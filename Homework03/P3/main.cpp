#include <iostream>
#include <windows.h>
#include <tlhelp32.h>

#include "a.h"
#include "b.h"
#include "c.h"
#include "e.h"
#include "i.h"
#include "h.h"
#include "global.h"
#include "error_handler.h"

int main() {

    //=== punctul a ===
    HANDLE hMap = createMemoryMapper();

    //=== punctul b ===
    DWORD PID_P1 = launchProcess(P1_EXE_PATH, TOKEN_P1_PATH);
    DWORD PID_P2 = launchProcess(P2_EXE_PATH, TOKEN_P2_PATH);

    //=== punctul d ===
    // COMENTAT PENTRU CA II APELAT IN LAUNCHPROCESS
    //tokenInformation(PID_P1, TOKEN_P1_PATH);
    //tokenInformation(PID_P2, TOKEN_P2_PATH);

    //=== punctul c ===
    waitAndPrintMemoryMapper();

    //=== punctul e f ===
    writeInSumar();

    // === punctul i ===
    KillProcessByPid(PID_P1);

    // === punctul h ===
    verifyPidStillExist(PID_P1);

    CloseHandle(hMap);
    return 0;
}