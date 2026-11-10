#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <string>
#include <cwchar>
#include "theme.hpp"

namespace {
HMODULE instance{};
SRWLOCK lifecycle=SRWLOCK_INIT;
HANDLE worker{},ready{};
DWORD worker_id{};
LONG startup_code=40;
HWND game{},overlay{};
bool visible=true,insert_down=false;
HFONT title_font{},subtitle_font{},body_font{},small_font{};
int dpi=96;
constexpr wchar_t class_name[]=L"TitaniumEmporerInjectedUI";
constexpr COLORREF transparent=RGB(1,2,3);
int px(int n){return MulDiv(n,dpi,96);}
void fill(HDC dc,RECT r,COLORREF c){auto b=CreateSolidBrush(c);FillRect(dc,&r,b);DeleteObject(b);}
void gradient(HDC dc,RECT r){
    for(int x=r.left;x<r.right;++x){auto c=titanium::theme::blend({122,47,13},{10,10,13},x-r.left,std::max(1L,r.right-r.left-1));
        fill(dc,{x,r.top,x+1,r.bottom},RGB(c.r,c.g,c.b));}
}
void label(HDC dc,const wchar_t* value,RECT r,HFONT font,COLORREF color,UINT flags=DT_LEFT|DT_TOP|DT_NOPREFIX){
    auto previous=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);
    DrawTextW(dc,value,-1,&r,flags);SelectObject(dc,previous);
}
void paint(HDC dc,RECT r){
    fill(dc,r,transparent);
    if(!visible)return;
    const int width=std::min(static_cast<int>(r.right)-px(24),px(580));
    const int x=(r.right-width)/2,y=px(18);
    gradient(dc,{x,y,x+width,y+px(132)});
    fill(dc,{x,y,x+width,y+px(3)},RGB(255,111,30));
    label(dc,L"MINECRAFT",{x,y+px(12),x+width,y+px(69)},title_font,RGB(255,247,237),DT_CENTER|DT_TOP|DT_SINGLELINE);
    label(dc,L"TITANIUM EDITION",{x,y+px(73),x+width,y+px(105)},subtitle_font,RGB(255,148,70),DT_CENTER|DT_TOP|DT_SINGLELINE);
    label(dc,L"Titanium Emporer  /  Windows",{x,y+px(108),x+width,y+px(130)},small_font,RGB(211,188,170),DT_CENTER|DT_TOP|DT_SINGLELINE);
    const int bottom=std::max(px(160),static_cast<int>(r.bottom)-px(230));
    gradient(dc,{px(18),bottom,px(342),bottom+px(206)});
    fill(dc,{px(18),bottom,px(21),bottom+px(206)},RGB(255,111,30));
    label(dc,L"TITANIUM EMPORER",{px(34),bottom+px(14),px(330),bottom+px(40)},subtitle_font,RGB(255,175,116));
    const wchar_t* names[]={L"X-RAY",L"NO FALL",L"FLY"};
    for(int i=0;i<3;++i){const int top=bottom+px(53+i*34);
        label(dc,names[i],{px(34),top,px(152),top+px(26)},body_font,RGB(244,233,222));
        label(dc,L"NO VERIFIED HOOK",{px(164),top+px(2),px(332),top+px(26)},small_font,RGB(218,157,111));}
    label(dc,L"Insert: show / hide this UI",{px(34),bottom+px(162),px(330),bottom+px(184)},small_font,RGB(211,188,170));
    label(dc,L"Session unclassified; gameplay features disabled",{px(34),bottom+px(184),px(330),bottom+px(204)},small_font,RGB(211,188,170));
}
bool foreground_game(){
    auto foreground=GetForegroundWindow();
    return foreground && game && (foreground==game || GetAncestor(foreground,GA_ROOT)==GetAncestor(game,GA_ROOT));
}
void track(){
    if(!IsWindow(game)){PostQuitMessage(0);return;}
    if(!foreground_game() || IsIconic(GetAncestor(game,GA_ROOT))){ShowWindow(overlay,SW_HIDE);insert_down=false;return;}
    const bool pressed=(GetAsyncKeyState(VK_INSERT)&0x8000)!=0;
    if(pressed&&!insert_down){visible=!visible;InvalidateRect(overlay,nullptr,FALSE);}insert_down=pressed;
    RECT r{};if(!GetClientRect(game,&r) || r.right<=0 || r.bottom<=0){ShowWindow(overlay,SW_HIDE);return;}
    POINT p{};if(!ClientToScreen(game,&p)){ShowWindow(overlay,SW_HIDE);return;}
    SetWindowPos(overlay,HWND_TOPMOST,p.x,p.y,r.right,r.bottom,SWP_NOACTIVATE|SWP_SHOWWINDOW);
}
LRESULT CALLBACK ui_proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    switch(message){
    case WM_TIMER:track();return 0;
    case WM_NCHITTEST:return HTTRANSPARENT;
    case WM_MOUSEACTIVATE:return MA_NOACTIVATE;
    case WM_ERASEBKGND:return 1;
    case WM_SIZE:InvalidateRect(w,nullptr,FALSE);return 0;
    case WM_PAINT:{PAINTSTRUCT ps{};HDC dc=BeginPaint(w,&ps);RECT r{};GetClientRect(w,&r);
        HDC buffer=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,std::max(1L,r.right),std::max(1L,r.bottom));
        auto old=SelectObject(buffer,bitmap);paint(buffer,r);BitBlt(dc,0,0,r.right,r.bottom,buffer,0,0,SRCCOPY);
        SelectObject(buffer,old);DeleteObject(bitmap);DeleteDC(buffer);EndPaint(w,&ps);return 0;}
    }return DefWindowProcW(w,message,wp,lp);
}
BOOL CALLBACK choose_game(HWND w,LPARAM parameter){
    auto best=reinterpret_cast<LONG64*>(parameter);DWORD pid{};GetWindowThreadProcessId(w,&pid);
    if(pid==GetCurrentProcessId() && IsWindowVisible(w)){
        wchar_t cls[128]{};GetClassNameW(w,cls,128);RECT r{};
        if(wcscmp(cls,class_name) && GetClientRect(w,&r) && r.right>=640 && r.bottom>=360){
            auto area=static_cast<LONG64>(r.right)*r.bottom;if(area>*best){*best=area;game=w;}}
    }return TRUE;
}
BOOL CALLBACK choose_tree(HWND w,LPARAM parameter){choose_game(w,parameter);EnumChildWindows(w,choose_game,parameter);return TRUE;}
DWORD WINAPI ui_thread(LPVOID){
    // Create a message queue before readiness so Stop can always post WM_QUIT.
    MSG message{};PeekMessageW(&message,nullptr,0,0,PM_NOREMOVE);
    game=nullptr;LONG64 largest{};EnumWindows(choose_tree,reinterpret_cast<LPARAM>(&largest));
    if(!game){InterlockedExchange(&startup_code,40);SetEvent(ready);return 0;}
    auto dc=GetDC(game);if(dc){dpi=GetDeviceCaps(dc,LOGPIXELSX);ReleaseDC(game,dc);}
    auto font=[](int size,int weight){return CreateFontW(-px(size),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");};
    title_font=font(42,FW_BOLD);subtitle_font=font(19,FW_SEMIBOLD);body_font=font(15,FW_SEMIBOLD);small_font=font(11,FW_NORMAL);
    WNDCLASSW cls{};cls.hInstance=instance;cls.lpfnWndProc=ui_proc;cls.lpszClassName=class_name;
    const auto registered=RegisterClassW(&cls);
    overlay=registered?CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,
        class_name,L"Titanium Emporer - Injected UI",WS_POPUP|WS_DISABLED,0,0,640,360,GetAncestor(game,GA_ROOT),nullptr,instance,nullptr):nullptr;
    const bool configured=overlay && SetLayeredWindowAttributes(overlay,transparent,255,LWA_COLORKEY) && SetTimer(overlay,1,50,nullptr);
    if(configured){visible=true;insert_down=false;track();}
    InterlockedExchange(&startup_code,configured?3:41);SetEvent(ready);
    if(configured){while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}}
    if(overlay){KillTimer(overlay,1);DestroyWindow(overlay);overlay=nullptr;}
    if(registered)UnregisterClassW(class_name,instance);
    for(auto f:{title_font,subtitle_font,body_font,small_font})if(f)DeleteObject(f);
    game=nullptr;return 0;
}
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH)instance=module;return TRUE;}
extern "C" __declspec(dllexport) DWORD WINAPI TitaniumStart(LPVOID){
    AcquireSRWLockExclusive(&lifecycle);
    if(worker && WaitForSingleObject(worker,0)==WAIT_TIMEOUT){ReleaseSRWLockExclusive(&lifecycle);return 44;}
    if(worker){CloseHandle(worker);worker=nullptr;}if(ready){CloseHandle(ready);ready=nullptr;}
    ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);InterlockedExchange(&startup_code,42);
    worker=ready?CreateThread(nullptr,0,ui_thread,nullptr,0,&worker_id):nullptr;
    DWORD result=42;
    if(worker && WaitForSingleObject(ready,5000)==WAIT_OBJECT_0)result=InterlockedCompareExchange(&startup_code,0,0);
    else if(worker)result=43;
    if(!worker && ready){CloseHandle(ready);ready=nullptr;}
    ReleaseSRWLockExclusive(&lifecycle);return result;
}
extern "C" __declspec(dllexport) DWORD WINAPI TitaniumStop(LPVOID){
    AcquireSRWLockExclusive(&lifecycle);
    DWORD result=0;
    if(worker){
        if(WaitForSingleObject(worker,0)==WAIT_TIMEOUT){
            if(!PostThreadMessageW(worker_id,WM_QUIT,0,0) || WaitForSingleObject(worker,5000)!=WAIT_OBJECT_0)result=45;
        }
        if(!result){CloseHandle(worker);worker=nullptr;if(ready){CloseHandle(ready);ready=nullptr;}}
    }
    ReleaseSRWLockExclusive(&lifecycle);return result;
}
