#include <Windows.h>
#include <stdio.h>

#define BREAK_WITH_ERROR(m)                              \
    {                                                    \
        printf("[-] %s! Error 0x%x", m, GetLastError()); \
        break;                                           \
    }
#define BREAK_WITH_STATUS(m, s)             \
    {                                       \
        printf("[-] %s! Error 0x%x", m, s); \
        break;                              \
    }

BOOL InstallService1()
{
    SC_HANDLE hSCM = NULL, hService = NULL;
    BOOL returnStatus = FALSE;
    WCHAR path[] = L"C:\\Users\\ho4ngphwc\\Desktop\\PoC\\Service.exe";
    SERVICE_DESCRIPTION description = {0};

    do
    {
        if (!(hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE)))
            BREAK_WITH_ERROR("Failed to open SCM");

        hService = CreateServiceW(hSCM,
                                  L"ho4ngphwc",
                                  NULL,
                                  SERVICE_CHANGE_CONFIG,
                                  SERVICE_WIN32_OWN_PROCESS,
                                  SERVICE_DEMAND_START,
                                  SERVICE_ERROR_NORMAL,
                                  path,
                                  NULL, NULL, NULL, NULL, NULL);
        if (!hService)
            BREAK_WITH_ERROR("Failed to create service");

        printf("[+] Create service success\n");

        description.lpDescription = L"Who am i? MRX";
        if (!ChangeServiceConfig2W(hService, SERVICE_CONFIG_DESCRIPTION, &description))
            BREAK_WITH_ERROR("Failed to set up description");

        printf("[+] Set description success\n");
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
    if (InstallService1())
    {
        printf("[+] Service installed successfully\n");
    }
    else
    {
        printf("[-] Service installation failed\n");
    }

    return 0;
}