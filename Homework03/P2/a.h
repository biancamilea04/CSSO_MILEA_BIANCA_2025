//
// functii punctul II a
//
#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>

BOOL EnableDebugPrivilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;

    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
        CloseHandle(hToken);
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(hToken);
    return GetLastError() == ERROR_SUCCESS;
}

void saveModules(const char *path, int PID) {
    HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
    MODULEENTRY32 me32;

    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32 , PID);
    if (hModuleSnap == INVALID_HANDLE_VALUE) {
        printf("Module snapshot failed for pid %d\n", PID);
        if (GetLastError() == ERROR_ACCESS_DENIED) {
            EnableDebugPrivilege();
            hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, PID);
            if (hModuleSnap == INVALID_HANDLE_VALUE) {
                printf("[ACCESS DENIED] PID: %d\n", PID);
                return;
            }
        } else {
            ErrorExit(false);
            return;
        }

    }

    me32.dwSize = sizeof(MODULEENTRY32);

    if (!Module32First(hModuleSnap, &me32)) {
        CloseHandle(hModuleSnap);
        ErrorExit();
    }

    do {
        char buffer[MAX_SIZE];
        int len = snprintf(buffer, sizeof(buffer),
                           "ModuleId: %lu, ProcessId: %lu, szModule: %lu, szExePath: %s\n",
                           me32.th32ModuleID,
                           me32.th32ProcessID,
                           me32.szModule,
                           me32.szExePath);
        if (len > 0) {
            printf(buffer);
            writeInFile(path, buffer);
        }
    } while (Module32Next(hModuleSnap, &me32));

    CloseHandle(hModuleSnap);

    printf("Modules saved for PID %d\n", PID);
}

void saveThreads(const char *path, int PID) {
    HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 te32;

    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE) {
        printf("Thread snapshot failed\n");
        ErrorExit();
    }

    te32.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(hThreadSnap, &te32)) {
        printf("Thread32First failed with error: %d\n", GetLastError());
        CloseHandle(hThreadSnap);
        ErrorExit();
    }

    do {
        if (te32.th32OwnerProcessID == PID) {
            char buffer[MAX_SIZE];
            int len = snprintf(buffer, sizeof(buffer),
                               "ThreadId: %lu, OwnerProcessId: %lu\n",
                               te32.th32ThreadID,
                               te32.th32OwnerProcessID);
            if (len > 0) {
                printf(buffer);
                writeInFile(path, buffer);
            }
        }
    } while (Thread32Next(hThreadSnap, &te32));

    CloseHandle(hThreadSnap);
    printf("Threads saved for PID %d\n", PID);
}