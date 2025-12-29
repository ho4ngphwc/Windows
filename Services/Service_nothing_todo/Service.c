#define _WIN32_WINTNT 0x0601
#include <Windows.h>
#include <stdio.h>

#define SERVICE_NAME L"ho4ngphwc"

// Global variables

SERVICE_STATUS g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE g_ServiceStopEvent = NULL;

// Service Control Handler

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

        SetEvent(g_ServiceStopEvent);
        break;
    default:
        break;
    }
}

// Service Main
VOID WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
    g_StatusHandle = RegisterServiceCtrlHandler(
        SERVICE_NAME,
        ServiceCtrlHandler);

    if (!g_StatusHandle)
        return;

    // Initial service status
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;
    g_ServiceStatus.dwWaitHint = 0;

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    // Create Stop event
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        g_ServiceStatus.dwCurrentState = SERVICE_STOP;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    // Service Running
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    // MAIN SERVICE LOOP
    while (WaitForSingleObject(g_ServiceStopEvent, 1000) != WAIT_OBJECT_0)
    {
        // Service is running
        // Payload có thể đặt ở sau này
        Sleep(1000);
    }

    // Service Stopped
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

int wmain(int argc, wchar_t *argv[])
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
        {
            {SERVICE_NAME, ServiceMain},
            {NULL, NULL}};

    StartServiceCtrlDispatcher(ServiceTable);
    return 0;
}