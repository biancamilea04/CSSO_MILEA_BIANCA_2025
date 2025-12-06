//
// functions for e
//

#include <set>

std::vector<int> getPidValue(const char *input) {
    std::string inputStr(input);
    std::vector<int> pidValue;
    const std::string delimiter = "\n";
    size_t pos = 0;

    while ((pos = inputStr.find(delimiter)) != std::string::npos) {
        std::string token = inputStr.substr(0, pos);

        int pos1 = token.find(": ");
        int pos2 = token.find(",");

        std::string key = token.substr(pos1 + 2, pos2 - pos1 - 2);

        int pid = atoi(key.c_str());
        pidValue.push_back(pid);

        inputStr.erase(0, pos + delimiter.length());
    }

    return pidValue;
}

int countSubstrOccurrences(const std::string &text, const std::string &needle) {
    if (needle.empty()) return 0;
    int count = 0;
    size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

// PUNCTUL F
std::string canOpenProcess(int pid) {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProc != NULL) {
        CloseHandle(hProc);
        return "DA";
    } else {
        return "NU";
    }
}

std::string formatPids(std::vector<int> pidValue) {
    std::string result;
    for (auto pid: pidValue) {
        //APLICAM PUNCTUL F
        result += std::to_string(pid) + " - Se poate deschide: " + canOpenProcess(pid) + "\n";
    }

    return result;
}

std::string processPidFile(const char *input) {
    std::string str_input = input;
    std::string pid = str_input.substr(str_input.find("[PID]: "), str_input.find("\n"));

    int nrModules = countSubstrOccurrences(str_input, "ModuleId:");
    int nrThreads = countSubstrOccurrences(str_input, "ThreadId:");

    return "Programul: " + pid + "are " + std::to_string(nrModules) + " parti incarcate si " + std::to_string(nrThreads)
           + " fire de lucru\n";
}

void iterDirectory(const char *path) {
    WIN32_FIND_DATA findFileData;
    char dirMask[MAX_PATH];
    int retValue = sprintf(dirMask, "%s\\*", path);
    if (retValue < 0) {
        ErrorExit();
    }

    HANDLE findHandle = FindFirstFile(dirMask, &findFileData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        ErrorExit();
    }

    do {
        char newPath[MAX_PATH];
        int retValue = sprintf(newPath, "%s\\%s", path, findFileData.cFileName);
        if (retValue < 0) {
            ErrorExit();
        }

        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            const char *outputProceseTxt = readFromFile(newPath);
            std::string processedData = processPidFile(outputProceseTxt);
            writeInFile(SUMAR_FILE_PATH, processedData.c_str());
        }
    } while (FindNextFile(findHandle, &findFileData) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        ErrorExit();
    }

    FindClose(findHandle);
}

std::string formatTokenInformation(const char *input, const char *name) {
    std::string str_input = input;
    std::string utilizator = str_input.substr(str_input.find("SID: "), str_input.find("\n"));
    std::string integritate = str_input.substr(str_input.find("Integrity Level: "), str_input.find("\n"));
    std::string elevare = str_input.substr(str_input.find("Elevation Status: "), str_input.find("\n"));
    std::string privilegii = str_input.substr(str_input.find("Privileges:"));

    return std::string(name) + ": ruleaza ca utlizator" + utilizator + ", are nivel de integritate " + integritate +
           ", este " + elevare + " si are urmatoarele privilegii:\n" + privilegii + "\n";
}

std::string getTokenInformationFormated(const char *path, const char *name) {
    const char *outputTokenInfo = readFromFile(path);
    std::string formattedInfo = formatTokenInformation(outputTokenInfo, "P1");

    return formattedInfo;
}

void writeInSumar() {
    const char *outputProceseTxt = readFromFile(PROCESE_FILE_PATH);
    std::vector<int> pids = getPidValue(outputProceseTxt);

    std::string formattedPids = formatPids(pids);

    iterDirectory(DETALII_FILE_PATH);

    std::string processP1TokenInfo = getTokenInformationFormated(TOKEN_P1_PATH, "P1.exe");
    std::string processP2TokenInfo = getTokenInformationFormated(TOKEN_P2_PATH, "P2.exe");

    std::string inputData = "=== RAPORT SUMAR ===\n\n";
    inputData += "Procese:\n";
    inputData += formattedPids + "\n";
    inputData += "Informatii token pentru P1.exe:\n";
    inputData += processP1TokenInfo + "\n";
    inputData += "Informatii token pentru P2.exe:\n";
    inputData += processP2TokenInfo + "\n";

    writeInFile(SUMAR_FILE_PATH, inputData.c_str());
}
