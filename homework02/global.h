//
// global variables, functions, definitions and headers
//

#include <knownfolders.h>
#include <vector>
using namespace std;

#define MAX_SIZE 1024

const string REGISTRY_KEY = "Software\\CSSO\\Week2";

const string COPY_FILE_PATH = "C:\\Users\\Bianca Milea\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup";
const string EXECUTABLE_FILE_TO_COPY_PATH = "C:\\Windows\\System32\\Taskmgr.exe";

const string EXECUTABLE_PATH = "C:\\Windows\\System32\\wsl.exe";
const string RUNNING_SOFTWARE_PATH = "C:\\Facultate\\CSSO\\Laboratoare\\H2\\RunningSoftware";
const string FROM_REGISTER_PATH = "C:\\Facultate\\CSSO\\Laboratoare\\H2\\RunningSoftware\\fromRegistries.txt";
const string FROM_STARTUP_PATH = "C:\\Facultate\\CSSO\\Laboratoare\\H2\\RunningSoftware\\fromStartupFolders.txt";

const vector<string> HK = {
    "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
    "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
    "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
    "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"
};

const string CURRENT_VERSION_RUN = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

const vector<string> folders = {
    "%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup",
    "%ProgramData%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"
};

const vector<GUID> GUIDs = {
    FOLDERID_Startup,
    FOLDERID_ProgramData
};

DWORD number_executable_files = 0;

bool hasExecutableExtension(const char *fileName) {
    const char *dot = strrchr(fileName, '.');
    if (!dot || dot == fileName) return false;

    string extension = string(dot);
    return (extension == ".exe" || extension == ".bat" || extension == ".cmd" || extension == ".com" || extension == ".ps1" || extension == ".vbs" || extension == ".js" || extension == ".msc");
}

void splitHkeyPath(const string &fullPath, string &hkey, string &key) {
    size_t pos = fullPath.find('\\');
    if (pos == string::npos) {
        return;
    }
    hkey = fullPath.substr(0, pos);
    key = fullPath.substr(pos + 1);
}


