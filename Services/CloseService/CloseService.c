#include <Windows.h>
#include <stdio.h>

#define BREAK_WITH_ERROR(m)                               \
    {                                                     \
        printf("[-] %s ! Error 0x%x", m, GetLastError()); \
        break;                                            \
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

int main()
{
    if (StopService1())
    {
        printf("[+] Service stopped successfully.\n");
    }
    else
    {
        printf("[-] Service stopped unsuccessfully.\n");
    }
}