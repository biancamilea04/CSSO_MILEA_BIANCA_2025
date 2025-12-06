#pragma once

void ErrorMessage() {
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
    }

    printf("Error: %s\n", lpMsgBuf);

    LocalFree(lpMsgBuf);
}

void write_file(std::string filename, std::string content) {
    HANDLE hFile = CreateFile(
        filename.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Could not open file for writing\n");
        printf("[FILE PATH]%S\n", filename.c_str());
        ErrorMessage();
        return;
    }

    SetFilePointer(hFile, 0, NULL, FILE_END);

    WINBOOL resultWriteFile = WriteFile(
        hFile,
        content.c_str(),
        content.size(),
        NULL,
        NULL
    );
    if (!resultWriteFile) {
        printf("[ERROR] Could not write to file %s\n", filename);
        ErrorMessage();
        CloseHandle(hFile);
        return;
    }

    CloseHandle(hFile);
}

void create_directory( const char *dirname ) {
    WINBOOL resultCreateDir = CreateDirectory(
        dirname,
        NULL
    );
    if (!resultCreateDir) {
        DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS) {
            printf("Could not create directory %s\n", dirname);
            ErrorMessage();
            return;
        }
    }
}

std::string read_file(const char *filename) {
    HANDLE hFile = CreateFile(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Could not open file for reading\n");
        ErrorMessage();
        return "";
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        printf("Could not get file size\n");
        ErrorMessage();
        CloseHandle(hFile);
        return "";
    }

    char *buffer = new char[fileSize + 1];
    DWORD bytesRead;
    WINBOOL resultReadFile = ReadFile(
        hFile,
        buffer,
        fileSize,
        &bytesRead,
        NULL
    );
    if (!resultReadFile || bytesRead != fileSize) {
        printf("Could not read file\n");
        ErrorMessage();
        delete[] buffer;
        CloseHandle(hFile);
        return "";
    }

    buffer[fileSize] = '\0';
    std::string content(buffer);
    delete[] buffer;
    CloseHandle(hFile);
    return content;
}

std::vector<std::string> split_lines(const std::string &content) {
    std::vector<std::string> lines;
    size_t start = 0;
    size_t end = content.find('\n');
    while (end != std::string::npos) {
        lines.push_back(content.substr(start, end - start));
        start = end + 1;
        end = content.find('\n', start);
    }

    if (start < content.size()) {
        lines.push_back(content.substr(start));
    }
    return lines;
}

std::tuple<std::string, std::string> get_method_link(std::string request) {
    std::string method = request.substr(0, request.find(':'));
    std::string url = request.substr(request.find(':') + 1);
    return {method, url};
}

std::string create_path(std::string base_dir, std::string endpoint, std::string prefix) {
    size_t pos = endpoint.find_last_of('/');
    if (pos != std::string::npos) {
        endpoint = endpoint.substr(pos + 1);
    }
    return base_dir + endpoint + prefix;
}

std::string extract_link(const std::string& url) {
    size_t pos = url.find("://");
    if (pos != std::string::npos) {
        pos = url.find('/', pos + 3);
    } else {
        pos = url.find('/');
    }

    if (pos == std::string::npos)
        return "/" + url;

    return url.substr(pos);
}

void update_file(std::string &file, const std::string &newValue) {
    size_t pos = file.find_last_of('\\');
    std::string folderPath = file.substr(0, pos + 1);
    std::string newFilename = newValue + "_aditional.txt";
    file = folderPath + newFilename;
}


int getDirectorySize(const std::string &dirPath) {
    WIN32_FIND_DATA findFileData;
    char dirMask[MAX_PATH];

    sprintf(dirMask, "%s\\*", dirPath.c_str());

    HANDLE hFind = FindFirstFile(dirMask, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;
    }

    int totalSize = 0;

    do {
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
            continue;

        char fullPath[MAX_PATH];
        sprintf(fullPath, "%s\\%s", dirPath.c_str(), findFileData.cFileName);

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            totalSize += getDirectorySize(fullPath);
        } else {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findFileData.nFileSizeLow;
            fileSize.HighPart = findFileData.nFileSizeHigh;
            totalSize += fileSize.QuadPart;
        }

    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

    return totalSize;
}
