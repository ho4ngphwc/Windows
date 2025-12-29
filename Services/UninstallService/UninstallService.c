#include <Windows.h>
#include <stdio.h>

#define BREAK_WITH_ERROR(m)                        \
    {                                              \
        printf("[-] %s! 0x%x", m, GetLastError()); \
        break;                                     \
    }

BOOL StopService1()
{
    BOOL returnStatus = FALSE;
    SC_HANDLE hSCM = NULL, hService = NULL;
    SERVICE_STATUS g_Status;

    do
    {
        if (!(hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT)))
            BREAK_WITH_ERROR("Failed to connect to SCM");

        if (!(hService = OpenServiceW(hSCM, L"ho4ngphwc", SERVICE_STOP)))
            BREAK_WITH_ERROR("Failed to open service");

        if (!ControlService(hService, SERVICE_CONTROL_STOP, &g_Status))
            BREAK_WITH_ERROR("Failed to stop service");

        printf("[+] Stop service success\n");
        returnStatus = TRUE;
    } while (0);

    if (hService)
        CloseServiceHandle(hService);

    if (hSCM)
        CloseServiceHandle(hSCM);

    return returnStatus;
}

BOOL UninstallService1()
{
    BOOL returnStatus = FALSE;
    SERVICE_STATUS status = {0};
    SC_HANDLE hSCM = NULL, hService = NULL;

    do
    {
        if (!(hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT)))
            BREAK_WITH_ERROR("Failed to connect to SCM");

        if (!(hService = OpenServiceW(hSCM, L"ho4ngphwc", DELETE | SERVICE_QUERY_STATUS)))
            BREAK_WITH_ERROR("Failed to open service");

        if (QueryServiceStatus(hService, &status) && status.dwCurrentState == SERVICE_RUNNING)
            StopService1();

        if (!DeleteService(hService))
            BREAK_WITH_ERROR("Failed to uninstall service");

        printf("[+] Uninstall service success\n");
        returnStatus = TRUE;
    } while (0);

    if (hService)
        CloseServiceHandle(hService);

    if (hSCM)
        CloseServiceHandle(hSCM);

    return returnStatus;
}

int main()
{
    if (UninstallService1())
    {
        printf("[+] Service uninstalled successfully\n");
    }
    else
    {
        printf("[-] Service uninstalled unsuccessfully\n");
    }
}