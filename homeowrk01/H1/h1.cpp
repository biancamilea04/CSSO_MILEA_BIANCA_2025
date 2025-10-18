#include <stdio.h>
#include <map>
#include <string>
#include <vector>

#include "Windows.h"

using namespace std;

#define MAX_SIZE 1024

const char *sumarFilePath = ".\\Rezultate\\sumar.txt";
const char *inputFilePath = ".\\Input\\copy.ini";
const char *logFilePath = ".\\Rezultate\\fisiereCopiate.txt";
string copyDirPath;

void ErrorExit() {
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();

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
        ExitProcess(dw);
    }

    printf("Error: %s\n", lpMsgBuf);

    LocalFree(lpMsgBuf);
    ExitProcess(dw);
}

const char *readDirectoryPath() {
    const char *directoryPath = new char[MAX_SIZE];
    printf("Directory path: ");
    scanf("%[^\n]", directoryPath);


    int resultFileAttributes = GetFileAttributes(directoryPath);
    if (resultFileAttributes == INVALID_FILE_ATTRIBUTES) {
        ErrorExit();
    }

    return directoryPath;
}

void saveExtensionsInMap(string fileName, map<string, int> &fileExtensions) {
    int dotPos = fileName.find_last_of('.');

    if (dotPos != string::npos) {
        string extension = fileName.substr(dotPos);
        if (fileExtensions.find(extension) != fileExtensions.end()) {
            fileExtensions[extension]++;
        } else {
            fileExtensions[extension] = 1;
        }
    }
}

void extractDirPathAndExtensions(string input, map<string, int> &copyExtensions) {
    int pos = input.find_first_of('\n');
    copyDirPath = input.substr(0, pos - 1);

    input = input.substr(pos + 1);

    while ((pos = input.find_first_of('\n')) != string::npos) {
        string line = input.substr(0, pos - 1);
        if (line != "") {
            copyExtensions[line] = 1;
        }

        input = input.substr(pos + 1);
    }

    if (input != "") {
        copyExtensions[input] = 1;
    }
}

void writeLogs(string logMessage) {
    HANDLE logFileHandler = CreateFile(logFilePath,
                                       GENERIC_WRITE,
                                       FILE_SHARE_WRITE | FILE_SHARE_READ,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL);
    if (logFileHandler == INVALID_HANDLE_VALUE) {
        ErrorExit();
    }

    SetFilePointer(logFileHandler, 0, NULL, FILE_END);

    int resultWriteFile = WriteFile(logFileHandler, logMessage.c_str(), logMessage.size(), NULL, NULL);
    if (resultWriteFile == 0) {
        ErrorExit();
    }

    CloseHandle(logFileHandler);
}

void copyFileInDir(string dirName, map<string, int> &copyExtensions) {
    string extension = dirName.substr(dirName.find_last_of('.'));

    if (copyExtensions.find(extension) != copyExtensions.end()) {
        string logMessage;
        string fileName = dirName.substr(dirName.find_last_of('\\') + 1);
        string copyFile = copyDirPath + "\\" + fileName;

        if ( copyFile == dirName ) {
            logMessage = "Source and destination files are the same: " + dirName + "\n";
            writeLogs(logMessage);

            printf("%s", logMessage.c_str());
            return;
        }

        int resultCopyFile = CopyFile(dirName.c_str(), copyFile.c_str(), false);
        if (resultCopyFile) {
            logMessage = "File " + dirName + " copied successfully.\n";
            writeLogs(logMessage);

            printf("%s", logMessage.c_str());
        } else {
            logMessage = "File " + dirName + " copied failed." + " \n";
            writeLogs(logMessage);

            printf("%s Error:%d\n", logMessage.c_str(), GetLastError());
        }
    }
}

void recusiveDir(const char *dirPath, map<string, int> &mapExtensions,
                 void (*function)(string, map<string, int> &)) {
    WIN32_FIND_DATA findFileData;
    char dirMask[MAX_PATH];
    int retValue = sprintf(dirMask, "%s\\*", dirPath);
    if (retValue < 0) {
        ErrorExit();
    }

    HANDLE findHandle = FindFirstFile(dirMask, &findFileData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        ErrorExit();
    }

    do {
        char newPath[MAX_PATH];
        int retValue = sprintf(newPath, "%s\\%s", dirPath, findFileData.cFileName);
        if (retValue < 0) {
            ErrorExit();
        }

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (strcmp(findFileData.cFileName, ".") && strcmp(findFileData.cFileName, "..")) {
                recusiveDir(newPath, mapExtensions, function);
            }
        } else {
            function(string(newPath), mapExtensions);
        }
    } while (FindNextFile(findHandle, &findFileData) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        ErrorExit();
    }

    FindClose(findHandle);
}

void writeSumarFile(const char *directoryPath, map<string, int> fileExtensions) {
    HANDLE sumarFileHandler = CreateFile(sumarFilePath,
                                         GENERIC_WRITE,
                                         FILE_SHARE_WRITE | FILE_SHARE_READ,
                                         NULL,
                                         CREATE_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL);
    if (sumarFileHandler == INVALID_HANDLE_VALUE) {
        ErrorExit();
    }

    DWORD bytesWritten;
    string path = string(directoryPath) + "\n";

    int resultWriteFile = WriteFile(sumarFileHandler, path.c_str(), path.size(), &bytesWritten, NULL);
    if (resultWriteFile == 0) {
        ErrorExit();
    }

    for (auto &pair: fileExtensions) {
        string line = pair.first + ":" + to_string(pair.second) + "\n";
        printf(line.c_str());

        int resultWriteFile = WriteFile(sumarFileHandler, line.c_str(), line.length(), &bytesWritten, NULL);
        if (resultWriteFile == 0) {
            ErrorExit();
        }
    }

    CloseHandle(sumarFileHandler);
}

string readInputFile() {
    HANDLE inputFileHnadler = CreateFile(
        inputFilePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (inputFileHnadler == INVALID_HANDLE_VALUE) {
        ErrorExit();
    }

    BYTE buffer[MAX_SIZE];
    memset(buffer, 0, MAX_SIZE);
    DWORD bytesRead;

    if (ReadFile(inputFileHnadler, buffer, MAX_SIZE, &bytesRead, NULL)) {
        char *charBuffer = new char[bytesRead + 1];
        memcpy(charBuffer, buffer, bytesRead);
        charBuffer[bytesRead] = '\0';

        string outputFile = charBuffer;

        CloseHandle(inputFileHnadler);
        delete[] charBuffer;

        return outputFile;
    } else {
        ErrorExit();
    }
}

void homework01() {
    const char *dirPath = readDirectoryPath();

    map<string, int> fileExtensions;
    map<string, int> copyExtensions;

    recusiveDir(dirPath, fileExtensions, &saveExtensionsInMap);
    writeSumarFile(dirPath, fileExtensions);

    string result = readInputFile();
    extractDirPathAndExtensions(result, copyExtensions);
    recusiveDir(dirPath, copyExtensions, &copyFileInDir);

    delete[] dirPath;
}

int main() {
    int directoryHandler = CreateDirectory("Rezultate", NULL);
    if (directoryHandler != ERROR_ALREADY_EXISTS && directoryHandler != 0) {
        ErrorExit();
    }

    homework01();

    return 0;
}
