//
// functii pentru punctul b)
//

// ================== scan and print files =================

void fileExists(LPSTR path, string &output) {
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    BOOL resultGetFileAttributes = GetFileAttributesEx(path, GetFileExInfoStandard, &fileData);
    if (resultGetFileAttributes == TRUE) {
        output += "File exists: YES\n";
        versionInfo(path);
        applySha256(path);
        if ( hasExecutableExtension(path) ) {
            number_executable_files +=1;
        }
    } else {
        output += "File exists: NO\n";
    }
}

string extractPathFromRegistryString(const char *registryString) {
    string path = registryString;
    size_t firstQuote = path.find('\"');
    size_t secondQuote = path.find('\"', firstQuote + 1);

    if (firstQuote != string::npos && secondQuote != string::npos) {
        return path.substr(firstQuote + 1, secondQuote - firstQuote - 1);
    }

    return path;
}

string convertLpftTimeToString(FILETIME *lpftLastWriteTime) {
    FILETIME ft;
    SYSTEMTIME st;

    if (FileTimeToLocalFileTime(lpftLastWriteTime, &ft) &&
        FileTimeToSystemTime(&ft, &st)) {
        return std::format("{:02}/{:02}/{:04} {:02}:{:02}:{:02}",
                           st.wDay, st.wMonth, st.wYear,
                           st.wHour, st.wMinute, st.wSecond);
    }

    return "";
}

void writeInRegisterFile(const char *input) {
    HANDLE fileHandler = CreateFile(
        FROM_REGISTER_PATH.c_str(),
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
         OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (fileHandler == INVALID_HANDLE_VALUE) {
        printf("Error opening file for writing registry data.\n");
        ErrorExit();
    }

    DWORD dwPtr = SetFilePointer(fileHandler, 0, NULL, FILE_END);
    if (dwPtr == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        printf("SetFilePointer failed: \n");
        CloseHandle(fileHandler);
        ErrorExit();
    }

    DWORD bytesWritten;
    WINBOOL rezult = WriteFile(fileHandler, input, strlen(input), &bytesWritten, NULL);
    if (rezult == FALSE) {
        printf("WriteFile failed: \n");
        CloseHandle(fileHandler);
        ErrorExit();
    }

    CloseHandle(fileHandler);
}

void writeMetadateKey(string hkey, const char *path, DWORD lpcSubKeys, DWORD lpcMaxSubKeyLen,
                      DWORD lpcValues, DWORD lpcMaxValueNameLen,
                      DWORD lpcMaxValueLen,
                      FILETIME *lpftLastWriteTime) {
    string output = "Key: ";
    output += hkey;
    output += "\n";
    output += "Path: ";
    output += path;
    output += "\n";
    output += "Number of subkeys: " + to_string(lpcSubKeys) + "\n";
    output += "Max subkey length: " + to_string(lpcMaxSubKeyLen) + "\n";
    output += "Number of values: " + to_string(lpcValues) + "\n";
    output += "Max value name length: " + to_string(lpcMaxValueNameLen) + "\n";
    output += "Max value length: " + to_string(lpcMaxValueLen) + "\n";
    output += "Last write time: " + convertLpftTimeToString(lpftLastWriteTime) + "\n";
    output += "----------------------------------------\n";
    printf("%s", output.c_str());

    writeInRegisterFile(output.c_str());
}

void writeValues(char *name, DWORD type, BYTE *valueBuffer, DWORD valueSize) {
    string output = "Value Name: ";
    output += name;
    output += "\n";
    output += "Type: " + to_string(type) + "\n";
    switch (type) {
        case REG_DWORD: {
            DWORD val = 0;
            memcpy(&val, valueBuffer, sizeof(DWORD));

            output += "Raw Data: " + to_string(val) + "\n";
            break;
        }
        case REG_QWORD: {
            unsigned long long val = 0;
            memcpy(&val, valueBuffer, sizeof(val));

            output += "Raw Data: " + to_string(val) + "\n";
            break;
        }
        case REG_BINARY: {
            output += "Raw Data : ";
            for (DWORD i = 0; i < valueSize; i++) {
                output += std::format("{:02X} ", valueBuffer[i]);
            }
            output += "\n";
            break;
        }
        case REG_SZ: {
            LPSTR outputValueBuffer = new char[valueSize + 1];
            LPCSTR lpValueBuffer = (LPCSTR) valueBuffer;

            strcpy_s(outputValueBuffer, valueSize + 1, lpValueBuffer);
            output += "Raw Data : ";
            output += outputValueBuffer;
            output += "\n";

            string path = extractPathFromRegistryString(outputValueBuffer);

            fileExists((LPSTR)path.c_str(), output);

            delete [] outputValueBuffer;
            break;
        }
        case REG_EXPAND_SZ: {
            LPSTR outputValueBuffer = new char[valueSize + 1];
            LPCSTR lpValueBuffer = (LPCSTR) valueBuffer;

            output += "Raw Data : ";
            output += lpValueBuffer;
            output += "\n";

            ExpandEnvironmentStrings(lpValueBuffer, outputValueBuffer, MAX_SIZE);
            output += "Expanded path : ";
            output += outputValueBuffer;
            output += "\n";

            fileExists(outputValueBuffer, output);

            delete[] outputValueBuffer;
            break;
        }
        default: {
        }
    }
    output += "----------------------------------------\n";
    printf("%s", output.c_str());

    writeInRegisterFile(output.c_str());
}
