//
// functions for c)
//
#pragma once
#include "error_handler.h"

void waitAndPrintMemoryMapper() {
    SIZE_T mapSize = MAX_SIZE;
    HANDLE hMap = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, "CSSOH3");
    if (hMap == NULL) {
        printf("OpenFileMapping failed: %d\n", GetLastError());
        ErrorExit();
    }

    LPVOID pView = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, mapSize);
    if (pView == nullptr) {
        printf("MapViewOfFile failed: %lu\n", GetLastError());
        CloseHandle(hMap);
        return;
    }

    printf("[WAITING]\n");

    char* pChar = (char*)pView;
    while (*pChar == '\0') {
        Sleep(2000);
    }

    printf("==[FINISH]==\n");

    printf( "Memory mapping ready with data.\n");
    printf("Memory mapping:\n%s\n", pChar);

    UnmapViewOfFile(pView);
    CloseHandle(hMap);
}
