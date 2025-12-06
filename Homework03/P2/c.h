//
// functii II c
//

#pragma once

#include "d.h"

void launchThreads() {
    HANDLE hThreadArray[MAX_THREADS];
    int countThreads = 0;

    const char *input = readFromFile(PROCESE_FILE_PATH);
    std::set<int, std::greater<> > allPids = getPidValue(input);
    std::set<int> pids = selectPids(allPids);

    for (auto pid: pids) {
        std::string path = DETALII_FILE_PATH + std::to_string(pid) + ".txt";
        printf(" Path: %s\n", path.c_str());

        PMYDATA args = new MyData;
        args->path = path;
        args->PID = pid;

        HANDLE hThread = CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) F2,
            args,
            0,
            NULL
        );
        if (hThread == NULL) {
            printf("Error creating thread for PID %d: %d\n", pid, GetLastError());
            ErrorExit();
        } else {
            hThreadArray[countThreads++] = hThread;
            printf("Thread created for PID %d\n", pid);
        }
    }

    WaitForMultipleObjects(countThreads, hThreadArray, TRUE, INFINITE);

    for (int i = 0; i < countThreads; i++) {
        CloseHandle(hThreadArray[i]);
    }

    //punctul d apel
    writePidsToMapping(pids);
}
