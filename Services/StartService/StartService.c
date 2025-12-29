#include <Windows.h>
#include <stdio.h>
#include <winternl.h>

#define BREAK_WITH_ERROR(m)                              \
    {                                                    \
        printf("[-] %s! Error 0x%x", m, GetLastError()); \
        break;                                           \
    }

BOOL StartService1()
{
    BOOL returnStatus = FALSE;
    SC_HANDLE hSCM = NULL, hService = NULL;

    do
    {
        if (!(hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT)))
            BREAK_WITH_ERROR("Failed to connect to SCM");

        if (!(hService = OpenServiceW(hSCM, L"ho4ngphwc", SERVICE_START)))
            BREAK_WITH_ERROR("Failed to open service");

        if (!StartService(hService, 0, NULL))
            BREAK_WITH_ERROR("Failed to start service");

        printf("[+] Start service success\n");
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
    if (StartService1())
    {
        printf("[+] Service started successfully\n");
    }
    else
    {
        printf("[-] Service didn't start\n");
    }
}