//
// function i
//

bool KillProcessByPid(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION,
                                  FALSE, pid);
    if (!hProcess) {
        printf( "OpenProcess failed: %d\n", GetLastError());
        return false;
    }
    BOOL result = TerminateProcess(hProcess, 1);
    if (!result) {
        printf( "OpenProcess failed: %d\n", GetLastError());
    }

    CloseHandle(hProcess);
    return result == TRUE;
}
