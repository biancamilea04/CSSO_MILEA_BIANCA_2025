//
//
//
#include <sddl.h>
#include <vector>

std::string getUserSid(HANDLE hToken) {
    DWORD dwSize = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
    std::vector<BYTE> buffer(dwSize);
    if (!GetTokenInformation(hToken, TokenUser, buffer.data(), dwSize, &dwSize)) {
        ErrorExit();
    }

    PTOKEN_USER pUser = (PTOKEN_USER)buffer.data();
    LPSTR StringSid;
    if (ConvertSidToStringSid(pUser->User.Sid, &StringSid)) {
        std::string sid(StringSid);
        LocalFree(StringSid);
        printf("[%s]\n", sid.c_str());
        return sid;
    }
    ErrorExit();
}

std::string getIntegrityLevel(HANDLE hToken) {
    DWORD dwLengthNeeded;
    GetTokenInformation(hToken, TokenIntegrityLevel, NULL, 0, &dwLengthNeeded);
    std::vector<BYTE> buffer(dwLengthNeeded);
    if (!GetTokenInformation(hToken, TokenIntegrityLevel, buffer.data(), dwLengthNeeded, &dwLengthNeeded))
        return "N/A";

    PTOKEN_MANDATORY_LABEL pTIL = (PTOKEN_MANDATORY_LABEL) buffer.data();
    DWORD dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid,
                                                 (DWORD) (*GetSidSubAuthorityCount(pTIL->Label.Sid) - 1));
    if (dwIntegrityLevel >= SECURITY_MANDATORY_SYSTEM_RID)
        return "System";
    if (dwIntegrityLevel >= SECURITY_MANDATORY_HIGH_RID)
        return "High";
    if (dwIntegrityLevel >= SECURITY_MANDATORY_MEDIUM_RID)
        return "Medium";

    return "Low";
}

std::string getElevationStatus(HANDLE hToken) {
    TOKEN_ELEVATION elevation;
    DWORD dwSize;
    if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
        return elevation.TokenIsElevated ? "Elevated" : "Not Elevated";
    }
    return "N/A";
}

std::string getPrivileges(HANDLE hToken) {
    DWORD dwSize = 0;

    GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwSize);
    DWORD err = GetLastError();
    if (err != ERROR_INSUFFICIENT_BUFFER) {
        printf("GetTokenInformation(TokenPrivileges) failed unexpectedly: %lu\n", err);
        return "";
    }

    std::vector<BYTE> buffer(dwSize);
    if (!GetTokenInformation(hToken, TokenPrivileges, buffer.data(), dwSize, &dwSize)) {
        printf("GetTokenInformation(TokenPrivileges) second call failed: %lu\n", GetLastError());
        return "";
    }

    PTOKEN_PRIVILEGES pPrivs = reinterpret_cast<PTOKEN_PRIVILEGES>(buffer.data());
    std::string privileges;

    for (DWORD i = 0; i < pPrivs->PrivilegeCount; i++) {
        WCHAR name[256];
        DWORD cchName = 0;

        if (LookupPrivilegeNameW(NULL, &pPrivs->Privileges[i].Luid, name, &cchName)) {
            char conv[256];
            WideCharToMultiByte(CP_UTF8, 0, name, -1, conv, sizeof(conv), NULL, NULL);

            privileges += conv;
            privileges += (pPrivs->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED)
                ? " (Enabled)\n"
                : " (Disabled)\n";
        } else {
            privileges += "[Unknown privilege]\n";
        }
    }

    return privileges;
}

void tokenInformation(DWORD pid, const char *TOKEN_INFORMATION_PATH) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        printf("OpenProcess failed for PID %lu: %lu\n", pid, GetLastError());
        ErrorExit();
    }

    HANDLE hToken;
    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
        printf("OpenProcessToken failed for PID %lu: %lu\n", pid, GetLastError());
        CloseHandle(hProcess);
        ErrorExit();
    }

    std::string sid = getUserSid(hToken);
    printf("[%s]\n", sid.c_str());
    std::string integrityLevel = getIntegrityLevel(hToken);
    printf("[%s]\n", integrityLevel.c_str());
    std::string elevationStatus = getElevationStatus(hToken);
    printf("[%s]\n", elevationStatus.c_str());
    std::string privileges = getPrivileges(hToken);
    printf("[%s]\n", privileges.c_str());

    std::string result = "Token Information for PID " + std::to_string(pid) + ":\n";
    result += "SID: " + sid + "\n";
    result += "Integrity Level: " + integrityLevel + "\n";
    result += "Elevation Status: " + elevationStatus + "\n";
    result += "Privileges:\n" + privileges + "\n";

    writeInFile(TOKEN_INFORMATION_PATH, result.c_str());

    CloseHandle(hToken);
    CloseHandle(hProcess);
}
