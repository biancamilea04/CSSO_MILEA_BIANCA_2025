//
// functii pentru punctul c)
//

#include <shlobj.h>

// ================== scan files =================

string getExtension(const char* fileName) {
    string s_fileName = string(fileName);
    string extension =  s_fileName.substr(s_fileName.find_last_of('.'));
    if (extension.empty() || extension == fileName) return "";
    return extension;
}

string timeToString(FILETIME ft) {
    FILETIME localFileTime;
    SYSTEMTIME systemTime;
    FileTimeToLocalFileTime(&ft, &localFileTime);
    FileTimeToSystemTime(&localFileTime, &systemTime);

    char buffer[20];
    sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d",
            systemTime.wDay, systemTime.wMonth, systemTime.wYear,
            systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
    return string(buffer);
}

void writeInStartupFile(string input) {
    HANDLE fileHandler = CreateFile(
        FROM_STARTUP_PATH.c_str(),
        GENERIC_WRITE | GENERIC_READ,
         FILE_SHARE_WRITE,
        NULL,
         OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (fileHandler == INVALID_HANDLE_VALUE) {
        printf("Error opening file for writing startup folder data.\n");
        ErrorExit();
    }

    DWORD dwPtr = SetFilePointer(fileHandler, 0, NULL, FILE_END);
    if (dwPtr == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        printf("SetFilePointer failed: %d\n", GetLastError());
        ErrorExit();
    }

    DWORD bytesWritten;
    BOOL writeResult = WriteFile(
        fileHandler,
        input.c_str(),
        (DWORD)input.size(),
        &bytesWritten,
        NULL
    );

    if (!writeResult || bytesWritten != input.size()) {
        printf("Error writing to file: %d\n", GetLastError());
        ErrorExit();
    }

    CloseHandle(fileHandler);
}

void writeElementInFile(WIN32_FIND_DATA myFindData, bool isExecutable) {
    string output = "Name : ";
    output += myFindData.cFileName;
    output += "\n";
    output += "Extension: ";
    output += getExtension(myFindData.cFileName);
    output += "\n";
    output+= "Creation Time: ";
    output += timeToString(myFindData.ftCreationTime);
    output += "\n";
    output += "Last Access Time: ";
    output += timeToString(myFindData.ftLastAccessTime);
    output += "\n";
    output += "Last Write Time: ";
    output += timeToString(myFindData.ftLastWriteTime);
    output += "\n";
    output += "Executable: ";
    output += (isExecutable ? "YES" : "NO");
    output += "\n";
    output += "----------------------------------------\n";
    printf("%s", output.c_str());

    writeInStartupFile(output);
}

void dirList(const char *dirName) {
    WIN32_FIND_DATA myFindData;
    char dirMask[MAX_PATH];
    int retValue = sprintf(dirMask, "%s\\*", dirName);

    HANDLE findHandle = FindFirstFile(dirMask, &myFindData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        printf("FindFirstFile failed with error: %d\n", GetLastError());
        ErrorExit();
    }

    do {
        if ( strcmp (myFindData.cFileName, ".") == 0 || strcmp(myFindData.cFileName, "..") == 0 ) {
            continue;
        }

        char* fullFilePath = new char[MAX_PATH];
        int retValuePath = sprintf(fullFilePath, "%s\\%s", dirName, myFindData.cFileName);

        bool isExecutable = hasExecutableExtension(myFindData.cFileName);
        if (isExecutable) {
            versionInfo( fullFilePath );
            applySha256( fullFilePath );
            number_executable_files+=1;
        }

        GetFileAttributesEx(myFindData.cFileName, GetFileExInfoStandard, &myFindData);
        writeElementInFile(myFindData, isExecutable);
    } while (FindNextFile(findHandle, &myFindData));

    FindClose(findHandle);
}

void scanFolder(string path) {
    char *outputPath = new char[MAX_SIZE];

    DWORD result = ExpandEnvironmentStrings(path.c_str(), outputPath, MAX_SIZE);
    if (result == 0 || result > MAX_SIZE) {
        ErrorExit();
    }

    printf("Scanning folder: %s\n", outputPath);
    dirList(outputPath);
}
