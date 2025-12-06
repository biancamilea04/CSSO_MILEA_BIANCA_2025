//
// functii III a
//
#pragma once
#include "global.h"

HANDLE createMemoryMapper() {
    const char *mapName = "CSSOH3";

    HANDLE hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,  MAX_SIZE, mapName);
    if (hMap == NULL) {
        printf("CreateFileMapping failed: %d", GetLastError());
        ErrorExit();
    }

    LPVOID view = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, MAX_SIZE);
    if (view == NULL) {
        printf("MapViewOfFile failed: %d", GetLastError());
        CloseHandle(hMap);
        ErrorExit();
    }

    memcpy( view, "\0", 1);

    printf("Created memory mapping %s and set first byte to '\\0'.", mapName);

    UnmapViewOfFile(view);

    return hMap;
}

