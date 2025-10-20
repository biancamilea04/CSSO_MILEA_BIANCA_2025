//
// functii pentru punctul e
//


void createValue() {
    HKEY hKey;
    int result = RegOpenKeyExA(
        HKEY_CURRENT_USER,
        CURRENT_VERSION_RUN.c_str(),
        0,
        KEY_WRITE,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        printf("RegOpenKeyExA failed with error: %d\n", result);
        ErrorExit();
    } else {
        printf("Registry key opened successfully.\n");
    }

    LSTATUS status = RegSetValueExA(
        hKey,
        "calculator",
        0,
        REG_SZ,
        (const BYTE *) EXECUTABLE_PATH.c_str(),
        EXECUTABLE_PATH.length()
    );
    if (status != ERROR_SUCCESS) {
        printf("RegSetValueExA failed with error: %d\n", status);
        RegCloseKey(hKey);
        ErrorExit();
    } else {
        printf("Registry value created successfully.\n");
    }

    RegCloseKey(hKey);
}

void copyExecutableFile() {
    WINBOOL result = CopyFile(
        EXECUTABLE_PATH.c_str(),
        (COPY_FILE_PATH + "\\wsl.exe").c_str(),
        FALSE
    );

    if (!result) {
        printf("CopyFile failed with error: %d\n", GetLastError());
        ErrorExit();
    } else {
        printf("File copied successfully to startup folder.\n");
    }
}
