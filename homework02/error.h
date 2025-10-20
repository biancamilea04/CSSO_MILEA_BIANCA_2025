//
// functie de handle errors
//


void ErrorExit(bool exit = true, BOOL customErrorCode = FALSE, DWORD inputErrorCode = 0) {
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    DWORD dw;
    if (!customErrorCode) {
        dw = GetLastError();
    } else {
        dw = inputErrorCode;
    }
    if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL) == 0) {
        MessageBox(NULL, TEXT("FormatMessage failed"), TEXT("Error"), MB_OK);
        if (exit) {
            ExitProcess(dw);
        }
    }

    printf("[ERROR][%08X]: %s\n", dw, lpMsgBuf);

    LocalFree(lpMsgBuf);
    if (exit) {
        ExitProcess(dw);
    }
}
