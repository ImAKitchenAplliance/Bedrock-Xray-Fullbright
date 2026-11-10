#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cwchar>
#include <stdexcept>
#include <vector>
#include "game.hpp"
namespace titanium {
namespace {
class Handle {
    HANDLE h_;
public:
    explicit Handle(HANDLE h):h_(h) {}
    ~Handle(){ if(h_ && h_!=INVALID_HANDLE_VALUE) CloseHandle(h_); }
    Handle(const Handle&)=delete;
    Handle& operator=(const Handle&)=delete;
    HANDLE get()const{return h_;}
};
[[noreturn]] void fail(const char* text) {
    throw std::runtime_error(std::string(text)+" (Windows error "+std::to_string(GetLastError())+")");
}
DWORD game_pid() {
    Handle s(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
    if(s.get()==INVALID_HANDLE_VALUE) fail("Process list unavailable");
    PROCESSENTRY32W p{};p.dwSize=sizeof(p);DWORD found=0;
    if(!Process32FirstW(s.get(),&p)) fail("Process enumeration failed");
    do { if(!_wcsicmp(p.szExeFile,L"Minecraft.Windows.exe")) {
        if(found)throw std::runtime_error("Multiple Minecraft instances found. Leave only one open.");
        found=p.th32ProcessID;
    }}while(Process32NextW(s.get(),&p));
    if(!found)throw std::runtime_error("Open Windows Bedrock Minecraft first, then try again.");
    return found;
}
std::wstring path(HMODULE m) {
    std::vector<wchar_t> b(32768);auto n=GetModuleFileNameW(m,b.data(),static_cast<DWORD>(b.size()));
    if(!n || n>=b.size())fail("Module path unavailable");
    return {b.data(),n};
}
std::wstring base_name(const std::wstring& p){return p.substr(p.find_last_of(L"\\/")+1);}
std::wstring target_path(HANDLE h) {
    std::vector<wchar_t>b(32768);DWORD n=static_cast<DWORD>(b.size());
    if(!QueryFullProcessImageNameW(h,0,b.data(),&n))fail("Minecraft identity unavailable");
    std::wstring p(b.data(),n);
    if(_wcsicmp(base_name(p).c_str(),L"Minecraft.Windows.exe"))throw std::runtime_error("Target identity changed; operation cancelled.");
    BOOL wow64{};SYSTEM_INFO info{};GetNativeSystemInfo(&info);
    if(!IsWow64Process(h,&wow64))fail("Architecture check failed");
    if(sizeof(void*)!=8 || wow64 || info.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_AMD64)
        throw std::runtime_error("This build requires native Windows x64 and x64 Minecraft.");
    if(WaitForSingleObject(h,0)!=WAIT_TIMEOUT)throw std::runtime_error("Minecraft has exited.");
    return p;
}
std::wstring version(const std::wstring& p){
    DWORD ignored{};DWORD n=GetFileVersionInfoSizeW(p.c_str(),&ignored);
    if(!n)fail("Game version unavailable");
    std::vector<unsigned char>b(n);
    if(!GetFileVersionInfoW(p.c_str(),0,n,b.data()))fail("Game version unavailable");
    VS_FIXEDFILEINFO*v{};UINT size{};
    if(!VerQueryValueW(b.data(),L"\\",reinterpret_cast<void**>(&v),&size) || size<sizeof(*v) || v->dwSignature!=0xfeef04bd)
        throw std::runtime_error("Invalid game version resource.");
    return std::to_wstring(HIWORD(v->dwFileVersionMS))+L"."+std::to_wstring(LOWORD(v->dwFileVersionMS))+L"."+
        std::to_wstring(HIWORD(v->dwFileVersionLS))+L"."+std::to_wstring(LOWORD(v->dwFileVersionLS));
}
struct Module{std::uintptr_t address{};std::wstring path;};
Module module(DWORD pid,const std::wstring& name){
    HANDLE raw=INVALID_HANDLE_VALUE;
    for(int i=0;i<4;++i){raw=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,pid);if(raw!=INVALID_HANDLE_VALUE || GetLastError()!=ERROR_BAD_LENGTH)break;}
    Handle s(raw);if(raw==INVALID_HANDLE_VALUE)fail("Module list unavailable");
    MODULEENTRY32W e{};e.dwSize=sizeof(e);
    if(!Module32FirstW(s.get(),&e))fail("Module enumeration failed");
    do{if(!_wcsicmp(e.szModule,name.c_str()))return {reinterpret_cast<std::uintptr_t>(e.modBaseAddr),e.szExePath};}
    while(Module32NextW(s.get(),&e));
    if(GetLastError()!=ERROR_NO_MORE_FILES)fail("Module enumeration failed");
    return {};
}
std::uintptr_t system_export(DWORD pid,const char* name){
    auto fn=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),name);if(!fn)fail("System function unavailable");
    HMODULE owner{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(fn),&owner))fail("Function owner unavailable");
    auto remote=module(pid,base_name(path(owner)));
    if(!remote.address)throw std::runtime_error("Matching system module missing in Minecraft.");
    return remote.address+reinterpret_cast<std::uintptr_t>(fn)-reinterpret_cast<std::uintptr_t>(owner);
}
DWORD call(HANDLE process,std::uintptr_t function,void* argument,bool& done){
    done=false;Handle t(CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(function),argument,0,nullptr));
    if(!t.get()){done=true;fail("Windows refused remote thread creation");}
    if(WaitForSingleObject(t.get(),30000)!=WAIT_OBJECT_0)
        throw std::runtime_error("Remote operation incomplete. Restart Minecraft before retrying; the thread may still finish.");
    done=true;DWORD code{};if(!GetExitCodeThread(t.get(),&code))fail("Thread status unavailable");return code;
}
std::wstring diagnostic(bool detach){
    DWORD pid=game_pid();
    Handle h(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE|PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid));
    if(!h.get())fail("Cannot open Minecraft");
    target_path(h.get());
    auto own=path(nullptr);auto dll=own.substr(0,own.find_last_of(L"\\/")+1)+L"titanium-client.dll";
    auto remote=module(pid,L"titanium-client.dll");bool done=true;
    if(remote.address && _wcsicmp(remote.path.c_str(),dll.c_str()))throw std::runtime_error("Loaded Titanium DLL is from another location.");
    if(detach){
        if(!remote.address)return L"No diagnostic DLL is attached.";
        if(!call(h.get(),system_export(pid,"FreeLibrary"),reinterpret_cast<void*>(remote.address),done))throw std::runtime_error("Windows refused to unload the DLL.");
        return L"Diagnostic DLL detached. No game hooks were installed.";
    }
    if(remote.address)throw std::runtime_error("Already attached. Detach before attaching again.");
    HMODULE local=LoadLibraryW(dll.c_str());if(!local)fail("Place titanium-client.dll beside Titanium-Emporer.exe");
    auto probe=GetProcAddress(local,"TitaniumProbe");
    const auto offset=reinterpret_cast<std::uintptr_t>(probe)-reinterpret_cast<std::uintptr_t>(local);FreeLibrary(local);
    if(!probe)throw std::runtime_error("Diagnostic export missing from DLL.");
    SIZE_T size=(dll.size()+1)*sizeof(wchar_t);
    void* arg=VirtualAllocEx(h.get(),nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!arg)fail("Cannot allocate DLL path");
    try{
        SIZE_T written{};
        if(!WriteProcessMemory(h.get(),arg,dll.c_str(),size,&written)||written!=size)fail("Cannot write DLL path");
        call(h.get(),system_export(pid,"LoadLibraryW"),arg,done);
    }catch(...){if(done)VirtualFreeEx(h.get(),arg,0,MEM_RELEASE);throw;}
    VirtualFreeEx(h.get(),arg,0,MEM_RELEASE);
    remote=module(pid,L"titanium-client.dll");
    if(!remote.address || _wcsicmp(remote.path.c_str(),dll.c_str()))throw std::runtime_error("Minecraft did not load the DLL. Loader policy/access may have rejected it; no bypass attempted.");
    if(call(h.get(),remote.address+offset,nullptr,done)!=2)throw std::runtime_error("Unexpected diagnostic response. No gameplay feature is enabled.");
    return L"Titanium Emporer DLL answered.\r\nDiagnostic attachment only. X-ray, No Fall and Fly have no game hooks yet.";
}
}
std::wstring inspect_game(){
    auto pid=game_pid();Handle h(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid));
    if(!h.get())fail("Cannot inspect Minecraft");auto p=target_path(h.get());
    return L"Minecraft detected  |  PID "+std::to_wstring(pid)+L"\r\nFile version: "+version(p)+
        L"\r\nSession: not classified. No build adapter is verified.\r\nX-ray / No Fall / Fly: unavailable.";
}
std::wstring attach_diagnostic(){return diagnostic(false);}
std::wstring detach_diagnostic(){return diagnostic(true);}
}
