//
// functii pentru punctul d)
//

#include <winver.h>
#include <bcrypt.h>

// ================== version info =================

struct LANGANDCODEPAGE {
    WORD wLanguage;
    WORD wCodePage;
} *lpTranslate;

void getInfo(const char *field, BYTE *buffer, UINT &len) {
    char subBlock[256];
    char *pValue = NULL;

    char format[] = "\\StringFileInfo\\%04x%04x\\";
    strcat(format, field);

    sprintf(subBlock, format,
            lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);

    if (VerQueryValue(buffer, subBlock, (LPVOID *) &pValue, &len) && pValue) {
        printf("%s: %s\n", field, pValue);
    } else {
        printf("%s: N/A\n", field);
    }
}

void versionInfo(const char *filePath) {
    printf("================== d =================\n");

    DWORD handle;
    DWORD size = GetFileVersionInfoSize(filePath, &handle);
    if (size == 0) {
        printf("GetFileVersionInfoSize failed. Error:%d\n", GetLastError());
        ErrorExit(false);
        return;
    }

    BYTE *buffer = new BYTE[size];
    if (!GetFileVersionInfo(filePath, handle, size, buffer)) {
        printf("GetFileVersionInfo failed. Error:%d\n", GetLastError());
        ErrorExit(false);
        delete[] buffer;
        return;
    }

    UINT len;
    if (!VerQueryValue(buffer, "\\VarFileInfo\\Translation", (LPVOID *) &lpTranslate, &len)) {
        printf("VerQueryValue for translation failed. Error:%d\n", GetLastError());
        ErrorExit(false);
        delete[] buffer;
        return;
    }

    printf("File Version Information:\n");

    vector<string> infoFields = {
        "CompanyName",
        "ProductName",
        "FileVersion"
    };

    for (const auto &field: infoFields) {
        getInfo(field.c_str(), buffer, len);
    }
    printf("----------------------------------------\n");

    delete[] buffer;
}

// ================== SHA =================

string readFile(const char *filePath) {
    HANDLE fileHandler = CreateFile(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (fileHandler == INVALID_HANDLE_VALUE) {
        printf("Error opening file for SHA256 hashing: %d\n", GetLastError());
        if ( GetLastError() == 1920 ) {
            bool exit = false;
            ErrorExit(exit);
            return "";
        }
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
    string input = string(c_buffer);

    delete[] c_buffer;
    CloseHandle(fileHandler);

    return input;
}

void sha256(string input) {
    BCRYPT_ALG_HANDLE hashAlgorithm;
    BCRYPT_HASH_HANDLE hashHandle;
    DWORD hashObjectSize, dataSize;

    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &hashAlgorithm,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        printf("BCryptOpenAlgorithmProvider failed. Status: 0x%08X\n", status);
        ErrorExit();
    }

    status = BCryptGetProperty(
        hashAlgorithm,
        BCRYPT_OBJECT_LENGTH,
        (PUCHAR) &hashObjectSize,
        sizeof(DWORD),
        &dataSize,
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        BCryptCloseAlgorithmProvider(hashAlgorithm, 0);
        printf("BCryptGetProperty failed. Status: 0x%08X\n", status);
        ErrorExit();
    }

    PUCHAR hashObject = new UCHAR[hashObjectSize];
    status = BCryptCreateHash(
        hashAlgorithm,
        &hashHandle,
        hashObject,
        hashObjectSize,
        NULL,
        0,
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        delete[] hashObject;
        BCryptCloseAlgorithmProvider(hashAlgorithm, 0);
        printf("BCryptCreateHash failed. Status: 0x%08X\n", status);
        ErrorExit();
    }

    status = BCryptHashData(
        hashHandle,
        (PUCHAR) input.c_str(),
        (ULONG) input.size(),
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        BCryptDestroyHash(hashHandle);
        delete[] hashObject;
        BCryptCloseAlgorithmProvider(hashAlgorithm, 0);
        printf("BCryptHashData failed. Status: 0x%08X\n", status);
        ErrorExit();
    }

    UCHAR hash[32];
    status = BCryptFinishHash(
        hashHandle,
        hash,
        sizeof(hash),
        0
    );
    if (!BCRYPT_SUCCESS(status)){
        delete [] hashObject;
        BCryptCloseAlgorithmProvider(hashAlgorithm, 0);
        printf("BCryptFinishHash failed. Status: 0x%08X\n", status);
        ErrorExit();
    }

    BCryptDestroyHash(hashHandle);
    delete[] hashObject;
    BCryptCloseAlgorithmProvider(hashAlgorithm, 0);

    printf("SHA-256: ");
    for (int i = 0; i < sizeof(hash); i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");

}

void applySha256(const char *filePath) {
    printf("================== d =================\n");

    string fileContent = readFile(filePath);
    if (fileContent.empty()) {
        printf("File is empty or cannot be read.\n");
        return;
    }
    sha256(fileContent);

    printf("----------------------------------------\n");
}
