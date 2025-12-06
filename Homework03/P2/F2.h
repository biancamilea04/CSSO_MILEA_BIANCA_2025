//
// functii pentru punctul II
//
#pragma once
#include <set>
#include <string>

#include "global.h"
#include "a.h"
#include "b.h"

typedef struct MyData {
    std::string path;
    int PID;
} *PMYDATA;

DWORD WINAPI F2(LPVOID lpParam) {
    PMYDATA pData = (PMYDATA) lpParam;
    std::string path = pData->path;
    int PID = pData->PID;

    char buffer[MAX_SIZE];
    int len = snprintf(buffer, sizeof(buffer),
                       "[PID]: %lu\n", PID);
    if (len > 0) {
        printf(buffer);
        writeInFile(path.c_str(), buffer);
    }

    printf("===[SAVE THREADS]===\n");
    saveThreads(path.c_str(), PID);

    printf("===[SAVE MODULES]===\n");
    saveModules(path.c_str(), PID);

    return 0;
}

