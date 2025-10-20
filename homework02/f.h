//
// functii pentru punctul f
//

void createRegistryKey() {
    HKEY hKey;
    DWORD keyResult;
    DWORD regErrorValue = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        REGISTRY_KEY.c_str(),
        0,
        0,
        0,
        KEY_WRITE,
        NULL,
        &hKey,
        &keyResult
    );
    if (regErrorValue != ERROR_SUCCESS) {
        printf("Error creating registry key: %s\n", REGISTRY_KEY.c_str());
        ErrorExit(TRUE, regErrorValue);
    }

    regErrorValue = RegSetValueEx(
        hKey,
        "StartupExecutables",
        0,
        REG_DWORD,
        (const BYTE *) &number_executable_files,
        sizeof(number_executable_files)
        );
    if (regErrorValue != ERROR_SUCCESS) {
        printf("Error setting registry value.\n");
        RegCloseKey(hKey);
        ErrorExit(TRUE, regErrorValue);
    }

    printf("Registry key is successful.\n");

    RegCloseKey(hKey);
}
