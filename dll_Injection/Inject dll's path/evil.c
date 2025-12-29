#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        MessageBoxA(NULL, "DLL injected!", "Info", MB_OK);
        WinExec("calc.exe", SW_SHOW);
        break;
    }
    return TRUE;
}