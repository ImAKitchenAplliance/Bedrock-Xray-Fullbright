#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cstring>
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
struct Module{std::uintptr_t address{};std::wstring path;DWORD size{};};
Module module(DWORD pid,const std::wstring& name){
    HANDLE raw=INVALID_HANDLE_VALUE;
    for(int i=0;i<4;++i){raw=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,pid);if(raw!=INVALID_HANDLE_VALUE || GetLastError()!=ERROR_BAD_LENGTH)break;}
    Handle s(raw);if(raw==INVALID_HANDLE_VALUE)fail("Module list unavailable");
    MODULEENTRY32W e{};e.dwSize=sizeof(e);
    if(!Module32FirstW(s.get(),&e))fail("Module enumeration failed");
    do{if(!_wcsicmp(e.szModule,name.c_str()))return {reinterpret_cast<std::uintptr_t>(e.modBaseAddr),e.szExePath,e.modBaseSize};}
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
// Resolve from the loaded image so old DLLs cannot receive newer entry-point RVAs.
std::uintptr_t client_export(HANDLE process,const Module& image,const char* wanted){
    auto read=[&](DWORD rva,void* buffer,SIZE_T size){
        if(rva>image.size || size>image.size-rva)throw std::runtime_error("Invalid client export range.");
        SIZE_T received{};
        if(!ReadProcessMemory(process,reinterpret_cast<void*>(image.address+rva),buffer,size,&received)||received!=size)fail("Cannot read loaded client exports");
    };
    IMAGE_DOS_HEADER dos{};read(0,&dos,sizeof(dos));
    if(dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<0)throw std::runtime_error("Invalid client DLL header.");
    IMAGE_NT_HEADERS64 nt{};read(static_cast<DWORD>(dos.e_lfanew),&nt,sizeof(nt));
    if(nt.Signature!=IMAGE_NT_SIGNATURE || nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
       nt.OptionalHeader.NumberOfRvaAndSizes<=IMAGE_DIRECTORY_ENTRY_EXPORT)throw std::runtime_error("Invalid client DLL image.");
    auto directory=nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if(!directory.VirtualAddress || directory.Size<sizeof(IMAGE_EXPORT_DIRECTORY))throw std::runtime_error("Client export directory missing.");
    IMAGE_EXPORT_DIRECTORY exports{};read(directory.VirtualAddress,&exports,sizeof(exports));
    if(exports.NumberOfNames>4096 || exports.NumberOfFunctions>4096)throw std::runtime_error("Unexpected client export table size.");
    std::vector<DWORD> names(exports.NumberOfNames),functions(exports.NumberOfFunctions);
    std::vector<WORD> ordinals(exports.NumberOfNames);
    read(exports.AddressOfNames,names.data(),names.size()*sizeof(DWORD));
    read(exports.AddressOfNameOrdinals,ordinals.data(),ordinals.size()*sizeof(WORD));
    read(exports.AddressOfFunctions,functions.data(),functions.size()*sizeof(DWORD));
    for(std::size_t i=0;i<names.size();++i){
        char name[64]{};
        for(DWORD j=0;j<sizeof(name)-1;++j){
            if(names[i]>image.size || j>=image.size-names[i])throw std::runtime_error("Invalid export name range.");
            read(names[i]+j,name+j,1);if(!name[j])break;
        }
        if(std::strcmp(name,wanted))continue;
        if(ordinals[i]>=functions.size())throw std::runtime_error("Invalid client export ordinal.");
        auto rva=functions[ordinals[i]];
        if(!rva || rva>=image.size || (rva>=directory.VirtualAddress && rva-directory.VirtualAddress<directory.Size))
            throw std::runtime_error("Invalid or forwarded client entry point.");
        return image.address+rva;
    }
    throw std::runtime_error("Loaded DLL is an older/incompatible build. Restart Minecraft, extract the entire new ZIP, then attach again.");
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
        if(!remote.address)return L"No Titanium DLL is attached.";
        if(call(h.get(),client_export(h.get(),remote,"TitaniumStop"),nullptr,done)!=0)
            throw std::runtime_error("Injected UI did not stop. DLL was left loaded; restart Minecraft before retrying.");
        if(!call(h.get(),system_export(pid,"FreeLibrary"),reinterpret_cast<void*>(remote.address),done))throw std::runtime_error("Windows refused to unload the DLL.");
        return L"Injected UI stopped and Titanium DLL detached. No game hooks were installed.";
    }
    if(remote.address)throw std::runtime_error("Already attached. Detach before attaching again.");
    if(GetFileAttributesW(dll.c_str())==INVALID_FILE_ATTRIBUTES)throw std::runtime_error("Place the new titanium-client.dll beside Titanium-Emporer.exe.");
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
    const auto code=call(h.get(),client_export(h.get(),remote,"TitaniumStart"),nullptr,done);
    if(code!=3)throw std::runtime_error("DLL loaded, but injected UI startup returned code "+std::to_string(code)+
        ". 40=no game window; 41=window setup failed; 42=worker failed; 43=timeout; 44=already running. Detach or restart Minecraft before retrying.");
    return L"Injected UI started. Return to Minecraft in windowed/borderless mode.\r\nMINECRAFT / TITANIUM EDITION branding is visible when the game has focus.\r\nInsert hides/shows the UI. X-ray, No Fall and Fly remain unavailable.";
}
}
std::wstring inspect_game(){
    auto pid=game_pid();Handle h(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid));
    if(!h.get())fail("Cannot inspect Minecraft");auto p=target_path(h.get());
    return L"Minecraft detected  |  PID "+std::to_wstring(pid)+L"\r\nFile version: "+version(p)+
        L"\r\nSession: not classified. No gameplay adapter is verified. UI attachment is available separately.\r\nX-ray / No Fall / Fly: unavailable.";
}
std::wstring attach_diagnostic(){return diagnostic(false);}
std::wstring detach_diagnostic(){return diagnostic(true);}
}
