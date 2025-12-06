//
// Created by Bianca Milea on 11/2/2025.
//
#pragma once

#include "d.h"
#include "global.h"

DWORD launchProcess(char cmd[], const char* TOKEN_INFORMATION_PATH) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD pid;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    printf("[START PROCESS] PID: %lu\n", pid);


    // Start the child process.
    if( !CreateProcess( NULL,   // No module name (use command line)
        cmd,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi )
    )
    {
        printf( "CreateProcess failed (%d).\n", GetLastError() );
        ErrorExit();
    }

    printf("[WAIT PROCESS] PID: %lu\n", pid);

    WaitForSingleObject( pi.hProcess, INFINITE );

    //=== punctul d ===
    pid = pi.dwProcessId;
    tokenInformation(pid, TOKEN_INFORMATION_PATH);

    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    printf("[FINISH PROCESS] PID: %lu\n", pid);

    return pid;
}
