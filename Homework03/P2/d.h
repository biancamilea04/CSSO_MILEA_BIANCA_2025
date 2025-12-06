#pragma once
#include <set>
#include <string>
#include <windows.h>
#include <cstring>
#include <cstdio>

//
// functii punctul d
//


void writePidsToMapping(std::set<int> &pids) {
    std::string pidData;
    for (int pid: pids)
        pidData += std::to_string(pid) += "\n";

    SIZE_T mapSize = pidData.size() + 1;

    HANDLE  hMap = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, "CSSOH3");
    if (hMap == NULL) {
        printf("OpenFileMapping failed: %lu\n", GetLastError());
        ErrorExit();
    }

    LPVOID pView = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, mapSize);
    if (pView == nullptr) {
        printf("MapViewOfFile failed: %lu\n", GetLastError());
        CloseHandle(hMap);
        ErrorExit();
    }

    char* pChar = (char*)pView;
    if (!pidData.empty()) {
        memcpy(pChar, pidData.c_str(), pidData.size());
    }

    if (pChar[0] == '\0') pChar[0] = '\1';

    UnmapViewOfFile(pView);
    CloseHandle(hMap);
}

