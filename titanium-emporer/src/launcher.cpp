#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <exception>
#include <algorithm>
#include <memory>
#include <string>
#include <thread>
#include "theme.hpp"
#include "game.hpp"

namespace {
constexpr UINT result_message=WM_APP+1;
constexpr int inspect_id=101,attach_id=102,copy_id=104;
HWND output{},buttons[4]{};
HFONT body{},heading{},small{},mono{},title{};
HBRUSH field_brush{};
bool busy=false;
int scale=96;
int px(int v){return MulDiv(v,scale,96);}
COLORREF rgb(titanium::theme::Color c){return RGB(c.r,c.g,c.b);}
void box(HDC dc,RECT r,COLORREF color){auto b=CreateSolidBrush(color);FillRect(dc,&r,b);DeleteObject(b);}
void text(HDC dc,const std::wstring& value,RECT r,HFONT font,COLORREF color,UINT flags=DT_LEFT|DT_TOP|DT_NOPREFIX){
    auto old=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);
    DrawTextW(dc,value.c_str(),static_cast<int>(value.size()),&r,flags);SelectObject(dc,old);
}
void line(HDC dc,int x,int y,int w,COLORREF color){box(dc,{x,y,x+w,y+px(1)},color);}
void gradient(HDC dc,RECT r,titanium::theme::Color from,titanium::theme::Color to){
    for(int x=r.left;x<r.right;++x)
        box(dc,{x,r.top,x+1,r.bottom},rgb(titanium::theme::blend(from,to,static_cast<unsigned>(x-r.left),static_cast<unsigned>(r.right-r.left-1))));
}
void layout(HWND w){
    RECT r{};GetClientRect(w,&r);int x=px(230),width=r.right-x-px(34),y=px(388);
    MoveWindow(output,x,y,width,std::max<LONG>(px(120),r.bottom-y-px(110)),TRUE);
    const int by=r.bottom-px(72),gap=px(10),bw=(width-gap*3)/4;
    for(int i=0;i<4;++i)MoveWindow(buttons[i],x+i*(bw+gap),by,bw,px(40),TRUE);
}
void paint(HWND window,HDC dc){
    RECT r{};GetClientRect(window,&r);
    box(dc,r,rgb(titanium::theme::black));
    gradient(dc,{px(196),0,r.right,px(245)},{73,32,15},{9,10,13});
    box(dc,{0,0,px(196),r.bottom},RGB(15,16,20));
    box(dc,{px(24),px(28),px(72),px(76)},rgb(titanium::theme::orange));
    text(dc,L"TE",{px(24),px(36),px(72),px(72)},heading,RGB(16,12,9),DT_CENTER|DT_TOP);
    text(dc,L"TITANIUM",{px(24),px(98),px(192),px(125)},heading,RGB(245,242,239));
    text(dc,L"EMPORER",{px(24),px(127),px(192),px(150)},small,RGB(255,140,66));
    line(dc,px(24),px(178),px(148),RGB(48,42,35));
    box(dc,{px(16),px(204),px(180),px(248)},RGB(47,29,19));
    box(dc,{px(16),px(204),px(19),px(248)},rgb(titanium::theme::orange));
    text(dc,L"Overview",{px(32),px(215),px(176),px(240)},body,RGB(255,177,124));
    text(dc,L"WINDOWS x64",{px(24),r.bottom-px(94),px(190),r.bottom-px(70)},small,RGB(153,148,145));
    text(dc,L"Local worlds + Realms",{px(24),r.bottom-px(64),px(190),r.bottom-px(30)},small,RGB(188,178,169));
    const int left=px(230),right=r.right-px(34),width=right-left;
    text(dc,L"WINDOWS CLIENT / 0.3",{left,px(31),right,px(52)},small,RGB(255,157,92));
    text(dc,L"Titanium Emporer",{left,px(65),right,px(116)},title,RGB(255,246,235));
    text(dc,L"Your world. Your edge.",{left,px(127),right,px(154)},body,RGB(205,184,168));
    text(dc,L"INJECTED UI BUILD  /  GAME FEATURES NOT IMPLEMENTED",{left,px(177),right,px(208)},small,RGB(255,170,104));
    const wchar_t* names[]={L"X-RAY",L"NO FALL",L"FLY"};
    const wchar_t* descriptions[]={L"Terrain visibility",L"Fall-damage handling",L"Flight controls"};
    int gap=px(12),card=(width-gap*2)/3;
    for(int i=0;i<3;++i){int x=left+i*(card+gap);
        box(dc,{x,px(239),x+card,px(336)},RGB(22,22,26));
        gradient(dc,{x,px(239),x+card,px(242)},{255,111,30},{44,28,21});
        text(dc,names[i],{x+px(16),px(254),x+card-px(12),px(278)},heading,RGB(249,234,221));
        text(dc,descriptions[i],{x+px(16),px(283),x+card-px(12),px(304)},small,RGB(164,157,151));
        text(dc,L"UNAVAILABLE",{x+px(16),px(308),x+card-px(12),px(330)},small,RGB(200,128,80));
    }
    text(dc,L"CONNECTION & DIAGNOSTICS",{left,px(356),right,px(380)},small,RGB(190,180,172));
    text(dc,busy?L"Working...":L"Read build is read-only. Inject UI adds the Titanium branding layer.",
        {left,r.bottom-px(25),right,r.bottom-px(3)},small,RGB(150,142,135));
}
void fonts(){
    auto make=[](int size,int weight,const wchar_t* face){return CreateFontW(-px(size),0,0,0,weight,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,face);};
    body=make(15,FW_NORMAL,L"Segoe UI");heading=make(18,FW_SEMIBOLD,L"Segoe UI");
    small=make(12,FW_NORMAL,L"Segoe UI");mono=make(13,FW_NORMAL,L"Consolas");title=make(38,FW_BOLD,L"Segoe UI");
}
void clipboard(HWND w){
    int length=GetWindowTextLengthW(output);std::wstring value(length+1,L'\0');
    GetWindowTextW(output,value.data(),length+1);
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,(length+1)*sizeof(wchar_t));if(!memory)return;
    auto data=GlobalLock(memory);if(!data){GlobalFree(memory);return;}
    CopyMemory(data,value.c_str(),(length+1)*sizeof(wchar_t));GlobalUnlock(memory);
    if(!OpenClipboard(w)){GlobalFree(memory);return;}
    EmptyClipboard();if(!SetClipboardData(CF_UNICODETEXT,memory))GlobalFree(memory);CloseClipboard();
}
void work(HWND w,int id){
    if(busy)return;
    if(id==copy_id){clipboard(w);return;}
    if(id==attach_id && MessageBoxW(w,L"This loads the Titanium UI DLL into Minecraft. It draws orange/black branding over the game window; native game buttons are unchanged.\n\nX-ray, No Fall and Fly are not implemented. Use windowed/borderless mode and stay at the title screen for this untested build. Continue?",L"Titanium Emporer - injected UI",MB_OKCANCEL|MB_ICONINFORMATION)!=IDOK)return;
    busy=true;for(auto b:buttons)EnableWindow(b,FALSE);InvalidateRect(w,nullptr,FALSE);
    try{
        std::thread([w,id]{
            auto result=std::make_unique<std::wstring>();
            try{*result=id==inspect_id?titanium::inspect_game():id==attach_id?titanium::attach_diagnostic():titanium::detach_diagnostic();}
            catch(const std::exception&e){std::string error=e.what();*result=L"Operation failed:\r\n"+std::wstring(error.begin(),error.end());}
            if(PostMessageW(w,result_message,0,reinterpret_cast<LPARAM>(result.get())))result.release();
        }).detach();
    }catch(...){busy=false;for(auto b:buttons)EnableWindow(b,TRUE);SetWindowTextW(output,L"Could not start diagnostic worker.");InvalidateRect(w,nullptr,FALSE);}
}
LRESULT CALLBACK window_proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    switch(message){
    case WM_CREATE:{
        auto dc=GetDC(w);scale=GetDeviceCaps(dc,LOGPIXELSX);ReleaseDC(w,dc);fonts();
        field_brush=CreateSolidBrush(RGB(17,18,22));
        output=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"Titanium Emporer is ready.\r\n\r\nOpen Windows Bedrock, then select Read build.\r\nNo game adapter is verified. Gameplay features remain unavailable.",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,0,0,0,0,w,nullptr,nullptr,nullptr);
        SendMessageW(output,WM_SETFONT,reinterpret_cast<WPARAM>(mono),TRUE);
        const wchar_t* captions[]={L"Read build",L"Inject UI",L"Detach",L"Copy report"};
        for(int i=0;i<4;++i){buttons[i]=CreateWindowExW(0,L"BUTTON",captions[i],WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
            0,0,0,0,w,reinterpret_cast<HMENU>(static_cast<INT_PTR>(inspect_id+i)),nullptr,nullptr);}
        layout(w);return 0;}
    case WM_GETMINMAXINFO:{auto m=reinterpret_cast<MINMAXINFO*>(lp);m->ptMinTrackSize={px(1020),px(740)};return 0;}
    case WM_SIZE:if(output){layout(w);InvalidateRect(w,nullptr,FALSE);}return 0;
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT ps{};auto dc=BeginPaint(w,&ps);RECT r{};GetClientRect(w,&r);
        auto buffer=CreateCompatibleDC(dc);auto bitmap=CreateCompatibleBitmap(dc,std::max(1L,r.right),std::max(1L,r.bottom));
        auto old=SelectObject(buffer,bitmap);paint(w,buffer);BitBlt(dc,0,0,r.right,r.bottom,buffer,0,0,SRCCOPY);
        SelectObject(buffer,old);DeleteObject(bitmap);DeleteDC(buffer);EndPaint(w,&ps);return 0;}
    case WM_DRAWITEM:{auto d=reinterpret_cast<DRAWITEMSTRUCT*>(lp);bool enabled=!(d->itemState&ODS_DISABLED);
        bool primary=d->CtlID==inspect_id;auto r=d->rcItem;
        if(primary&&enabled)gradient(d->hDC,r,{255,133,48},{219,78,16});else box(d->hDC,r,enabled?RGB(38,30,25):RGB(27,26,26));
        if(d->itemState&ODS_SELECTED){r.left+=px(1);r.top+=px(1);}
        wchar_t label[80]{};GetWindowTextW(d->hwndItem,label,80);
        text(d->hDC,label,r,small,!enabled?RGB(111,103,98):primary?RGB(22,14,9):RGB(255,181,129),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        if(d->itemState&ODS_FOCUS){InflateRect(&r,-px(4),-px(4));DrawFocusRect(d->hDC,&r);}return TRUE;}
    case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:{auto dc=reinterpret_cast<HDC>(wp);SetTextColor(dc,RGB(225,210,196));SetBkColor(dc,RGB(17,18,22));return reinterpret_cast<LRESULT>(field_brush);}
    case WM_COMMAND:if(HIWORD(wp)==BN_CLICKED && LOWORD(wp)>=inspect_id && LOWORD(wp)<=copy_id)work(w,LOWORD(wp));return 0;
    case result_message:{std::unique_ptr<std::wstring> result(reinterpret_cast<std::wstring*>(lp));SetWindowTextW(output,result->c_str());
        busy=false;for(auto b:buttons)EnableWindow(b,TRUE);InvalidateRect(w,nullptr,FALSE);return 0;}
    case WM_CLOSE:if(busy){MessageBoxW(w,L"Wait for the diagnostic operation to finish before closing.",L"Titanium Emporer",MB_OK|MB_ICONINFORMATION);return 0;}DestroyWindow(w);return 0;
    case WM_DESTROY:for(auto f:{body,heading,small,mono,title})DeleteObject(f);DeleteObject(field_brush);PostQuitMessage(0);return 0;
    }return DefWindowProcW(w,message,wp,lp);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show){
    SetProcessDPIAware();WNDCLASSW cls{};cls.hInstance=instance;cls.lpfnWndProc=window_proc;
    cls.lpszClassName=L"TitaniumEmporerLauncher";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    cls.hIcon=LoadIconW(nullptr,IDI_APPLICATION);RegisterClassW(&cls);
    auto dc=GetDC(nullptr);scale=GetDeviceCaps(dc,LOGPIXELSX);ReleaseDC(nullptr,dc);
    auto w=CreateWindowExW(0,cls.lpszClassName,L"Titanium Emporer",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        CW_USEDEFAULT,CW_USEDEFAULT,px(1080),px(780),nullptr,nullptr,instance,nullptr);
    if(!w)return 1;
    BOOL dark=TRUE;COLORREF caption=RGB(15,16,20),caption_text=RGB(255,177,124);
    // Unsupported title-bar attributes are harmlessly ignored on older Windows.
    DwmSetWindowAttribute(w,20,&dark,sizeof(dark));
    DwmSetWindowAttribute(w,35,&caption,sizeof(caption));
    DwmSetWindowAttribute(w,36,&caption_text,sizeof(caption_text));
    ShowWindow(w,show);MSG msg{};
    while(GetMessageW(&msg,nullptr,0,0)>0){if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
    return static_cast<int>(msg.wParam);
}
