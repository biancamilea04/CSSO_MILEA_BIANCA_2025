#pragma once
#include "error_handler.h"

const int MAX_SIZE = 1024;

const char* PROCESE_FILE_PATH = "C:\\Facultate\\CSSO\\Laboratoare\\H3\\procese.txt";
const char* DETALII_FILE_PATH = "C:\\Facultate\\CSSO\\Laboratoare\\H3\\Detalii\\";
const char* SUMAR_FILE_PATH = "C:\\Facultate\\CSSO\\Laboratoare\\H3\\Raport\\sumar.txt";

char P1_EXE_PATH[] = "\"C:\\Users\\Bianca Milea\\Desktop\\uni\\III\\csso\\Homework03\\P1\\P1.exe\"";
char P2_EXE_PATH[] = "\"C:\\Users\\Bianca Milea\\Desktop\\uni\\III\\csso\\Homework03\\P2\\P2.exe\"";

const char* TOKEN_P1_PATH = "C:\\Facultate\\CSSO\\Laboratoare\\H3\\token_P1.txt";
const char* TOKEN_P2_PATH = "C:\\Facultate\\CSSO\\Laboratoare\\H3\\token_P2.txt";

void writeInFile(const char *path, const char* input) {
    HANDLE fileHandler = CreateFile(
        path,
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

    DWORD res = SetFilePointer(fileHandler, 0, NULL, FILE_END);
    if (res == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        printf("SetFilePointer failed: %d\n", GetLastError());
        ErrorExit();
    }

    DWORD result = WriteFile(fileHandler, input, strlen(input), NULL, NULL);
    if ( !result ) {
        printf("Error writing to file: %d\n", GetLastError());
        ErrorExit();
    }

    CloseHandle(fileHandler);
}

const char* readFromFile( const char* path) {
    HANDLE fileHandler = CreateFile(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (fileHandler == INVALID_HANDLE_VALUE) {
        printf("Error opening file for reading: %d\n", GetLastError());
        ErrorExit();
    }

    BYTE buffer[MAX_SIZE];
    memset(buffer, 0, MAX_SIZE);
    DWORD bytesRead;

    if (!ReadFile(fileHandler, buffer, MAX_SIZE, &bytesRead, NULL)) {
        printf("ReadFile failed. Error:%d\n", GetLastError());
        CloseHandle(fileHandler);
        ErrorExit();
    }

    char *c_buffer = new char[bytesRead + 1];
    memcpy(c_buffer, buffer, bytesRead);
    c_buffer[bytesRead] = '\0';

    CloseHandle(fileHandler);

    return c_buffer;
}