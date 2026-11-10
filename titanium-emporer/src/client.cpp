#define WIN32_LEAN_AND_MEAN
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
extern "C" __declspec(dllexport) DWORD WINAPI TitaniumProbe(LPVOID) {
    OutputDebugStringW(L"[Titanium Emporer] Diagnostic DLL loaded. No game adapter; Xray, NoFall and Fly unavailable.\n");
    return 2;
}
