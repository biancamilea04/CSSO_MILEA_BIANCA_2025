#include <iostream>
#include <Windows.h>
#include <format>


#include "global.h"
#include "error.h"
#include "d.h"
#include "b.h"
#include "c.h"
#include "e.h"
#include "f.h"

using namespace std;

HKEY toHkey(string hkey) {
    if (hkey.empty()) {
        printf("Empty HKEY string\n");
        exit(-1);
    }

    if (hkey == "HKLM") {
        return HKEY_LOCAL_MACHINE;
    }
    if (hkey == "HKCU") {
        return HKEY_CURRENT_USER;
    }
    if (hkey == "HKCR") {
        return HKEY_CLASSES_ROOT;
    }
    if (hkey == "HKU") {
        return HKEY_USERS;
    }
    if (hkey == "HKCC") {
        return HKEY_CURRENT_CONFIG;
    }
    if (hkey == "HKPD") {
        return HKEY_PERFORMANCE_DATA;
    }

    printf("Unknown HKEY: %s\n", hkey.c_str());
    exit(-1);
}

void handleHkey(string hkeyName, const char *key) {
    DWORD keyResult;
    HKEY resultedKeyHandler;
    HKEY hKeyRoot = toHkey(hkeyName);

    DWORD regErrorValue = RegOpenKeyEx(hKeyRoot, key, 0, KEY_READ, &resultedKeyHandler);

    if (regErrorValue != ERROR_SUCCESS) {
        printf("Error key opening: %s\n", key);
        ErrorExit(TRUE, regErrorValue);
    }


    DWORD nSubKeys;
    DWORD nSubKeysMaxLen;
    DWORD valuesC;
    DWORD maxValueNameLen;
    DWORD maxValueLen;
    FILETIME fileTime;

    regErrorValue = RegQueryInfoKey(resultedKeyHandler, NULL, NULL, NULL, &nSubKeys, &nSubKeysMaxLen, NULL, &valuesC,
                                    &maxValueNameLen, &maxValueLen, NULL, &fileTime);
    if (regErrorValue != ERROR_SUCCESS) {
        ErrorExit(TRUE, regErrorValue);
    }

    writeMetadateKey(hkeyName, key, nSubKeys, nSubKeysMaxLen, valuesC, maxValueNameLen, maxValueLen, &fileTime);

    DWORD actualNameSize = maxValueNameLen + 1;
    DWORD actualValueSize = maxValueLen + 1;
    char *nameBuffer = new char[actualNameSize];
    BYTE *valueBuffer = new BYTE[actualValueSize];

    DWORD tempNameSize;
    DWORD tempValueSize;

    DWORD type;

    DWORD errorRegEnumValue;

    for (int i = 0; i < valuesC; i++) {
        memset(nameBuffer, 0, actualNameSize);
        memset(valueBuffer, 0, actualValueSize);

        tempNameSize = actualNameSize;
        tempValueSize = actualValueSize;

        errorRegEnumValue = RegEnumValue(resultedKeyHandler, i, nameBuffer, &tempNameSize, 0, &type, valueBuffer,
                                         &tempValueSize);
        if (errorRegEnumValue != ERROR_SUCCESS) {
            ErrorExit(TRUE, errorRegEnumValue);
        }
        writeValues(nameBuffer, type, valueBuffer, tempValueSize);
    }

    RegCloseKey(resultedKeyHandler);
}


void homework03() {
    BOOL result = CreateDirectory(RUNNING_SOFTWARE_PATH.c_str(), NULL);
    if (result != 0 || GetLastError() == ERROR_ALREADY_EXISTS) {
        printf("Directory created or already exists: %s\n", RUNNING_SOFTWARE_PATH.c_str());
    } else {
        ErrorExit();
    }

    printf("================== b =================\n");
    for ( auto &hk : HK) {
        string hkey;
        string key;
        splitHkeyPath(hk, hkey, key);
        handleHkey(hkey, key.c_str());
    }


    printf("================== c =================\n");
    for ( auto path : folders) {
        scanFolder(path);
    }

    printf("================== e =================\n");
    createValue();
    copyExecutableFile();
    printf("\n");

    printf("================== f =================\n");
    printf("Executablen files: %d\n", number_executable_files);
    createRegistryKey();


}

int main() {
    homework03();

    return 0;
}
