#define _WIN32_WINNT 0x0601
#include <Windows.h>
#include <stdio.h>
#include <winternl.h>

#define BREAK_WITH_STATUS(m, s)        \
    {                                  \
        printf("[-] %s 0x%x\n", m, s); \
        break;                         \
    }

VOID InitUnicodeString(PUNICODE_STRING destinationString, PCWSTR sourceString)
{
    destinationString->Buffer = (PWSTR)sourceString;
    destinationString->Length = (USHORT)(wcslen(sourceString) * sizeof(WCHAR));
    destinationString->MaximumLength = destinationString->Length + sizeof(WCHAR);
}

typedef NTSTATUS(NTAPI *pfnZwCreateKey)(
    PHANDLE KeyHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG TitleIndex,
    PUNICODE_STRING Class,
    ULONG CreateOptions,
    PULONG Disposition);

typedef NTSTATUS(NTAPI *pfnZwSetValueKey)(
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName,
    ULONG TitleIndex,
    ULONG Type,
    PVOID Data,
    ULONG DataSize);

BOOL InstallService3()
{
    BOOL returnStatus = FALSE;
    NTSTATUS status = 0;
    HMODULE hNtdll = NULL;

    pfnZwCreateKey pZwCreateKey = NULL;
    pfnZwSetValueKey pZwSetValueKey = NULL;

    HKEY hKey = NULL;
    UNICODE_STRING keyName = {0},
                   valueName = {0};
    OBJECT_ATTRIBUTES objectAttributes;

    WCHAR objectName[] = L"LocalSystem",
          imagePath[] = L"C:\\Users\\ho4ngphwc\\Desktop\\PoC\\Services.exe",
          keyPath[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ho4ngphwc";

    DWORD error = SERVICE_ERROR_NORMAL,
          startType = SERVICE_DEMAND_START,
          serviceType = SERVICE_WIN32_OWN_PROCESS;

    do
    {
        if (!(hNtdll = GetModuleHandleW(L"ntdll")))
            break;

        if (!(pZwCreateKey = GetProcAddress(hNtdll, "ZwCreateKey")))
            break;

        if (!(pZwSetValueKey = GetProcAddress(hNtdll, "ZwSetValueKey")))
            break;

        InitUnicodeString(&keyName, keyPath);
        InitializeObjectAttributes(&objectAttributes, &keyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

        if (!NT_SUCCESS(status = pZwCreateKey(&hKey, KEY_SET_VALUE, &objectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL)))
            BREAK_WITH_STATUS("Failed to create key", status);

        InitUnicodeString(&valueName, L"ErrorControl");
        if (!NT_SUCCESS(status = pZwSetValueKey(hKey, &valueName, 0, REG_DWORD, &error, sizeof(error))))
            BREAK_WITH_STATUS("Failed to set error control", status);

        InitUnicodeString(&valueName, L"ImagePath");
        if (!NT_SUCCESS(status = pZwSetValueKey(hKey, &valueName, 0, REG_EXPAND_SZ, imagePath, wcslen(imagePath) * sizeof(*imagePath))))
            BREAK_WITH_STATUS("Failed to set image path", status);

        InitUnicodeString(&valueName, L"ObjectName");
        if (!NT_SUCCESS(status = pZwSetValueKey(hKey, &valueName, 0, REG_SZ, objectName, wcslen(objectName) * sizeof(*objectName))))
            BREAK_WITH_STATUS("Failed to set object name", status);

        InitUnicodeString(&valueName, L"Start");
        if (!NT_SUCCESS(status = pZwSetValueKey(hKey, &valueName, 0, REG_DWORD, &startType, sizeof(startType))))
            BREAK_WITH_STATUS("Failed to set service type", status);

        printf("[+] Create service success!\n");
        returnStatus = TRUE;

    } while (0);

    /* if (hNtdll)
         CloseHandle(hNtdll);*/

    if (hKey)
        RegCloseKey(hKey);

    return returnStatus;
}

int main()
{
    if (InstallService3())
    {
        printf("[+] Service installed successfully\n");
    }
    else
    {
        printf("[-] Service installation failed\n");
    }
}