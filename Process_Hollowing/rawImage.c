#include <Windows.h>
#include <stdio.h>

int main(int argc, const char* argv[]) {
	WCHAR path[] = L"C:\\Windows\\System32\\notepad.exe";
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi = { 0 };

	if (!CreateProcessW(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		return 1; 
	}

	WaitForSingleObject(pi.hThread, INFINITE);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return 0;
}