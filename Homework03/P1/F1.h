#pragma once
#include "error_handler.h"
#include "global.h"

void F1(const char* path) {
    printf("=== P1 ===\n\n ");

    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if(hProcessSnap == INVALID_HANDLE_VALUE)
    {
        printf("CreateToolhelp32Snapshot failed.err = %d \n",GetLastError());
        ErrorExit();
    }

    pe32.dwSize = sizeof( PROCESSENTRY32 );

    if( !Process32First( hProcessSnap, &pe32 ) )
    {
        printf("ErrorProcess32First : %d \n", GetLastError() );
        CloseHandle( hProcessSnap );
        ErrorExit();
    }
    do
    {
        printf("Proces [%d]: %s \n",pe32.th32ProcessID,pe32.szExeFile );
        char buffer[MAX_SIZE];
        int len = snprintf(buffer, sizeof(buffer), "Proces: %lu, ParentPID: %lu, Name: %s, cntThreads: %lu\n",
                           pe32.th32ProcessID,
                           pe32.th32ParentProcessID,
                           pe32.szExeFile,
                           pe32.cntThreads);
        if (len > 0) {
            printf(buffer);
            writeInFile(path, buffer);
        }
    }
    while(Process32Next(hProcessSnap, &pe32 ));

    CloseHandle(hProcessSnap);
}
