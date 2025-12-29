#include <Windows.h>
#include <stdio.h>

BOOL InstallService2()
{
    BOOL returnStatus = FALSE;
    HKEY hKey = NULL;
    LSTATUS lResult = 0;
    DWORD disposition = 0,
          startType = SERVICE_DEMAND_START,
          serviceType = SERVICE_WIN32_OWN_PROCESS,
          errorControl = SERVICE_ERROR_NORMAL;

    CONST WCHAR subKey[] = L"SYSTEM\\CurrentControlSet\\Services\\ho4ngphwc",
                imagePath[] = L"C:\\Users\\ho4ngphwc\\Desktop\\PoC\\Service.exe",
                objectName[] = L"LocalSystem";

    do
    {
        if (ERROR_SUCCESS != RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &disposition))
            break;

        if (ERROR_SUCCESS != RegSetValueExW(hKey, L"ErrorControl", 0, REG_DWORD, &errorControl, sizeof(errorControl)))
            break;

        if (ERROR_SUCCESS != RegSetValueExW(hKey, L"ImagePath", 0, REG_EXPAND_SZ, imagePath, wcslen(imagePath) * sizeof(*imagePath)))
            break;

        if (ERROR_SUCCESS != RegSetValueExW(hKey, L"ObjectName", 0, REG_SZ, objectName, wcslen(objectName) * sizeof(*objectName)))
            break;

        if (ERROR_SUCCESS != RegSetValueExW(hKey, L"Start", 0, REG_DWORD, &startType, sizeof(startType)))
            break;

        if (ERROR_SUCCESS != RegSetValueExW(hKey, L"Type", 0, REG_DWORD, &serviceType, sizeof(serviceType)))
            break;

        returnStatus = TRUE;
    } while (0);

    RegCloseKey(hKey);
    return returnStatus;
}

int main()
{
    if (InstallService2())
    {
        printf("[+] Service installed successfully.\n");
    }
    else
    {
        printf("[-] Service installation failed.\n");
    }
}