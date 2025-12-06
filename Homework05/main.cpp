#include <functional>
#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <wininet.h>
#include <commctrl.h>

#include "helpers.h"

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "comctl32.lib")

// ===[ CONSTANTE GLOBALE ]===
const char *CONFIG_FILE = "C:\\Facultate\\CSSO\\H5\\myconfig.txt";
const char *DOWNLOADS_DIR = "C:\\Facultate\\CSSO\\H5\\Downloads\\";
const char *HOST = "cssohw.herokuapp.com";

// ===[ VARIABILE GLOBALE ]===
std::string GET_LAST_RESPONSE;
std::string NR_MATRICOL = "310910401RSL231142";
std::string AGENT = "310910401RSL231142";

int COUNT_GET = 0;
int COUNT_POST = 0;

// ===[ CONTROALE GUI ]===
HWND hWndMain;
HWND hEditMatricol;
HWND hEditLink;
HWND hButtonRun;
HWND hListLog;

// ===[ ID-URI CONTROALE ]===
#define ID_EDIT_MATRICOL 101
#define ID_EDIT_LINK 102
#define ID_BUTTON_RUN 103
#define ID_LIST_LOG 104

// ===[ FUNCTII AUXILIARE GUI ]===
void AppendLog(const std::string& message) {
    SendMessage(hListLog, LB_ADDSTRING, 0, (LPARAM)message.c_str());
    int count = SendMessage(hListLog, LB_GETCOUNT, 0, 0);
    SendMessage(hListLog, LB_SETTOPINDEX, count - 1, 0);
    UpdateWindow(hListLog);
}

// ===[ FUNCTII NETWORK ]===
void send_get(std::string path, std::string method, std::string link,
              std::function<void(std::string, std::string)> func) {
    HINTERNET hInternet = InternetOpen(AGENT.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    HINTERNET hConnect = InternetConnect(hInternet, HOST, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL,
                                         INTERNET_SERVICE_HTTP, 0, 0);

    if (hConnect == NULL) {
        AppendLog("[ERROR] InternetConnect failed");
        ErrorMessage();
        InternetCloseHandle(hInternet);
        return;
    }

    HINTERNET hRequest = HttpOpenRequest(hConnect, method.c_str(), link.c_str(), NULL, NULL, NULL,
                                         INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);

    std::string header = "User-Agent: " + AGENT;

    AppendLog("[REQUEST] " + method + " " + link);

    BOOL hResponse = HttpSendRequest(hRequest, header.c_str(), -1, NULL, 0);
    if (!hResponse) {
        AppendLog("[ERROR] HttpSendRequest failed");
        ErrorMessage();
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    char buffer[2048];
    DWORD bytesRead;
    BOOL resultInternetReadFile = InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead);
    if (!resultInternetReadFile) {
        AppendLog("[ERROR] InternetReadFile failed");
        ErrorMessage();
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    buffer[bytesRead] = '\0';

    std::string response(buffer);
    AppendLog("[RESPONSE] " + response.substr(0, 100) + (response.length() > 100 ? "..." : ""));

    func(path, std::string(buffer));

    GET_LAST_RESPONSE = buffer;

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

void send_post(std::string link) {
    std::string body = "id=" + NR_MATRICOL + "&value=" + GET_LAST_RESPONSE;

    HINTERNET hInternet = InternetOpen(AGENT.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    HINTERNET hConnect = InternetConnect(hInternet, HOST, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL,
                                         INTERNET_SERVICE_HTTP, 0, 0);

    std::string path = extract_link(link);

    HINTERNET hRequest = HttpOpenRequest(hConnect, "POST", path.c_str(), NULL, NULL, NULL,
                                         INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);

    std::string headers = "Content-Type: application/x-www-form-urlencoded\r\nUser-Agent: " + AGENT;

    AppendLog("[REQUEST] POST " + path);

    BOOL res = HttpSendRequest(hRequest, headers.c_str(), headers.size(),
                               (LPVOID) body.c_str(), body.size());

    if (!res) {
        AppendLog("[ERROR] POST failed");
        ErrorMessage();
    } else {
        AppendLog("[RESPONSE] POST sent successfully");
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

void process_dohomework_endpoint(std::string method, std::string link) {
    std::string path = create_path(DOWNLOADS_DIR, link, ".txt");
    if (method == "GET") {
        send_get(path, method, extract_link(link), &write_file);
        COUNT_GET += 1;
    } else if (method == "POST") {
        send_post(link);
        COUNT_POST += 1;
    }
}

void process_additional_endpoint(std::string link) {
    std::string value = link.substr(link.find_last_of('/') + 1);
    std::string filename = std::string(DOWNLOADS_DIR) + value + "_aditional.txt";
    std::string currentValue = value;

    while (true) {
        std::string path = "/dohomework_additional/" + currentValue;

        std::string response = "";

        send_get(
            filename,
            "GET",
            path,
            [&](std::string file, std::string content) {
                write_file(file, "\n" + content);
                response = content;
            }
        );

        while (!response.empty() && isspace(response.back()))
            response.pop_back();

        if (response == "0")
            break;

        currentValue = response;
        update_file(filename, currentValue);
    }
}

void process_config_file(const char *filename) {
    std::string content = read_file(filename);
    std::vector<std::string> lines = split_lines(content);

    for (auto line : lines) {
        std::tuple<std::string, std::string> method_link = get_method_link(line);
        std::string method = std::get<0>(method_link);
        std::string link = std::get<1>(method_link);

        AppendLog("[PROCESSING] " + method + " " + link);

        if (link.find("dohomework_additional") != std::string::npos) {
            process_additional_endpoint(link);
        } else {
            COUNT_GET += 1;
            process_dohomework_endpoint(method, link);
        }
    }
}

void send_summary(const std::string &nrmatricol, int totalRequests, int getRequests, int postRequests, int totalSizeBytes) {
    std::string postData = "id=" + nrmatricol +
                           "&total=" + std::to_string(totalRequests) +
                           "&get=" + std::to_string(getRequests) +
                           "&post=" + std::to_string(postRequests) +
                           "&size=" + std::to_string(totalSizeBytes);

    HINTERNET hInternet = InternetOpen(AGENT.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        AppendLog("[ERROR] InternetOpen failed");
        return;
    }

    HINTERNET hConnect = InternetConnect(hInternet, HOST, INTERNET_DEFAULT_HTTP_PORT,
                                        NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        AppendLog("[ERROR] InternetConnect failed");
        InternetCloseHandle(hInternet);
        return;
    }

    const char* path = "/endhomework";
    HINTERNET hRequest = HttpOpenRequest(hConnect, "POST", path,
                                         NULL, NULL, NULL,
                                         INTERNET_FLAG_RELOAD, 0);
    if (!hRequest) {
        AppendLog("[ERROR] HttpOpenRequest failed");
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    const char* headers = "Content-Type: application/x-www-form-urlencoded";

    AppendLog("[REQUEST] POST /endhomework");
    AppendLog("[DATA] " + postData);

    BOOL res = HttpSendRequest(hRequest, headers, strlen(headers),
                              (LPVOID)postData.c_str(), postData.size());
    if (!res) {
        AppendLog("[ERROR] HttpSendRequest failed");
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    char buffer[2048];
    DWORD bytesRead;
    std::string fullResponse;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead) {
        buffer[bytesRead] = '\0';
        fullResponse += buffer;
    }

    AppendLog("[RESPONSE] " + fullResponse);

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    AppendLog("[DONE] Final POST completed");
}

// ===[ FUNCTIA DE RULARE A PROGRAMULUI ]===
DWORD WINAPI RunProgram(LPVOID lpParam) {
    COUNT_GET = 0;
    COUNT_POST = 0;
    GET_LAST_RESPONSE = "";

    char matricolBuffer[256];
    GetWindowText(hEditMatricol, matricolBuffer, 256);
    NR_MATRICOL = matricolBuffer;
    AGENT = matricolBuffer;

    AppendLog("========================================");
    AppendLog("[START] nr matricol: " + NR_MATRICOL);
    AppendLog("========================================");

    create_directory(DOWNLOADS_DIR);

    std::string assignPath = "assignhomework/" + NR_MATRICOL;
    send_get(CONFIG_FILE, "GET", assignPath.c_str(), &write_file);

    process_config_file(CONFIG_FILE);

    int total_size = getDirectorySize(DOWNLOADS_DIR);

    send_summary(NR_MATRICOL, COUNT_GET + COUNT_POST, COUNT_GET, COUNT_POST, total_size);

    AppendLog("========================================");
    AppendLog("[SUMMARY] Total GET: " + std::to_string(COUNT_GET));
    AppendLog("[SUMMARY] Total POST: " + std::to_string(COUNT_POST));
    AppendLog("[SUMMARY] Total Size: " + std::to_string(total_size) + " bytes");
    AppendLog("========================================");

    EnableWindow(hButtonRun, TRUE);
    return 0;
}

void OnRunButtonClick() {
    EnableWindow(hButtonRun, FALSE);

    SendMessage(hListLog, LB_RESETCONTENT, 0, 0);

    CreateThread(NULL, 0, RunProgram, NULL, 0, NULL);
}

// ===[ WINDOW PROCEDURE ]===
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindow("STATIC", "Numar Matricol:", WS_VISIBLE | WS_CHILD,
                        20, 20, 120, 20, hwnd, NULL, NULL, NULL);

            hEditMatricol = CreateWindow("EDIT", "310910401RSL231142",
                        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                        150, 18, 300, 24, hwnd, (HMENU)ID_EDIT_MATRICOL, NULL, NULL);

            CreateWindow("STATIC", "Link Server:", WS_VISIBLE | WS_CHILD,
                        20, 50, 120, 20, hwnd, NULL, NULL, NULL);

            hEditLink = CreateWindow("EDIT", "http://cssohw.herokuapp.com/",
                        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
                        150, 48, 300, 24, hwnd, (HMENU)ID_EDIT_LINK, NULL, NULL);

            hButtonRun = CreateWindow("BUTTON", "Ruleaza Program",
                        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        470, 18, 150, 54, hwnd, (HMENU)ID_BUTTON_RUN, NULL, NULL);

            CreateWindow("STATIC", "Log Requesturi si Raspunsuri:", WS_VISIBLE | WS_CHILD,
                        20, 115, 250, 20, hwnd, NULL, NULL, NULL);

            hListLog = CreateWindow("LISTBOX", NULL,
                        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
                        20, 140, 600, 350, hwnd, (HMENU)ID_LIST_LOG, NULL, NULL);

            break;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_BUTTON_RUN) {
                OnRunButtonClick();
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ===[ WINMAIN ]===
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "HomeworkClientClass";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hWndMain = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "HomeworkClientClass",
        "Homework5",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 660, 550,
        NULL, NULL, hInstance, NULL
    );

    if (hWndMain == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
