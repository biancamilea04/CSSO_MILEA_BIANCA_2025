#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <commctrl.h>
#include <shlobj.h>
#include <thread>

#pragma comment(lib, "comctl32.lib")

constexpr auto MAX_ITEMS = 10000;
constexpr auto EMPTY = 0xFFFFFFFF;

static std::wstring ReportsRoot = L"C:\\Facultate\\CSSO\\H4\\Reports";
static std::wstring SummaryRoot = L"C:\\Facultate\\CSSO\\H4\\Reports\\Summary";
static std::wstring PathLogs = ReportsRoot + L"\\logs.txt";
static std::wstring PathSold = SummaryRoot + L"\\sold.txt";
static std::wstring PathDonations = SummaryRoot + L"\\donations.txt";
static std::wstring PathErrors = SummaryRoot + L"\\errors.txt";

HWND g_hDepositPath = NULL;
HWND g_hSellPath = NULL;
HWND g_hBtnBrowseDeposit = NULL;
HWND g_hBtnBrowseSell = NULL;
HWND g_hBtnRun = NULL;
HWND g_hBtnStop = NULL;
HWND g_hSoldText = NULL;
HWND g_hDonationsText = NULL;
HWND g_hLogsEdit = NULL;
HWND g_hErrorsEdit = NULL;

std::wstring g_depositDir = L"deposit";
std::wstring g_soldDir = L"sold";
bool g_isRunning = false;
HANDLE g_hStopEvent = NULL;
std::thread* g_workerThread = nullptr;

struct DayEntry {
	int year, month, day;
	std::wstring filename;
};

struct MonitorData {
	PROCESS_INFORMATION pi;
	HANDLE hErrorsMutex;
	std::wstring name;
	std::wstring errorsPath;
};

void CreateDirectory(const std::wstring& directoryName) {
	if (!CreateDirectoryW(directoryName.c_str(), NULL)) {
		if (GetLastError() != ERROR_ALREADY_EXISTS) {
			wprintf(L"Failed to create directory: %s.ErroCode:%lu\n", directoryName.c_str(), GetLastError());
		}
		else {
			wprintf(L"Directory already exists: %s\n", directoryName.c_str());
		}
	}
}

void AppendFileAtomic(const std::wstring& path, const std::wstring& data, HANDLE hmutex) {
	WaitForSingleObject(hmutex, INFINITE);

	HANDLE hFile = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		wprintf(L"Failed to open file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
		ReleaseMutex(hmutex);
		return;
	}

	DWORD bytesWritten;
	if (!WriteFile(hFile, data.c_str(), static_cast<DWORD>(data.size() * sizeof(wchar_t)), &bytesWritten, NULL)) {
		wprintf(L"Failed to write to file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
	}

	if (!CloseHandle(hFile)) {
		wprintf(L"Failed to close file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
	}
	if (!ReleaseMutex(hmutex)) {
		wprintf(L"Failed to release mutex. ErrorCode: %lu\n", GetLastError());
	}
}

void WriteNumberFileAtomic(const std::wstring& path, double value, HANDLE hmutex) {
	WaitForSingleObject(hmutex, INFINITE);

	HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		wprintf(L"Failed to open file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
		ReleaseMutex(hmutex);
		return;
	}

	std::wstring data = std::to_wstring(value);
	DWORD bytesWritten;
	if (!WriteFile(hFile, data.c_str(), static_cast<DWORD>(data.size() * sizeof(wchar_t)), &bytesWritten, NULL)) {
		wprintf(L"Failed to write to file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
	}

	if (!CloseHandle(hFile)) {
		wprintf(L"Failed to close file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
	}
	if (!ReleaseMutex(hmutex)) {
		wprintf(L"Failed to release mutex. ErrorCode: %lu\n", GetLastError());
	}
}

double ReadNumberFile(const std::wstring& path) {
	HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		wprintf(L"Failed to open file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
		return 0.0;
	}

	wchar_t buffer[64];
	DWORD bytesRead;
	if (!ReadFile(hFile, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, NULL)) {
		wprintf(L"Failed to read from file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
		CloseHandle(hFile);
		return 0.0;
	}
	buffer[bytesRead / sizeof(wchar_t)] = L'\0';

	if (!CloseHandle(hFile)) {
		wprintf(L"Failed to close file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
	}

	return std::stod(buffer);
}

bool ParseDateFile(const std::wstring& fileName, int& year, int& month, int& day) {
	std::wstring base = fileName;
	size_t dotPosition = base.rfind(L".txt");
	if (dotPosition != std::wstring::npos) {
		base = base.substr(0, dotPosition);
	}
	int yy, mm, dd;
	if (swscanf_s(base.c_str(), L"%d.%d.%d", &yy, &mm, &dd) == 3) {
		year = yy;
		month = mm;
		day = dd;
		return true;
	}
	return false;
}

std::vector<std::wstring> ListFilesSorted(const std::wstring& directory) {
	std::vector<DayEntry> tmp;
	std::wstring pattern = directory + L"\\*";

	WIN32_FIND_DATAW findFileData;
	HANDLE h = FindFirstFileW(pattern.c_str(), &findFileData);
	if (h == INVALID_HANDLE_VALUE) {
		wprintf(L"Failed to list files in directory: %s. ErrorCode: %lu\n", directory.c_str(), GetLastError());
		return {};
	}

	do {
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			int y, m, d;
			if (ParseDateFile(findFileData.cFileName, y, m, d)) {
				tmp.push_back({ y, m, d, findFileData.cFileName });
			}
		}
	} while (FindNextFileW(h, &findFileData));

	FindClose(h);

	std::sort(tmp.begin(), tmp.end(), [](const DayEntry& a, const DayEntry& b) {
		if (a.year != b.year) return a.year < b.year;
		if (a.month != b.month) return a.month < b.month;
		return a.day < b.day;
		});

	std::vector<std::wstring> result;
	for (auto& t : tmp) result.push_back(t.filename);
	return result;
}

DWORD WINAPI TimeoutMonitor(LPVOID param) {
	MonitorData* d = (MonitorData*)param;

	DWORD r = WaitForSingleObject(d->pi.hProcess, 60000);
	if (r == WAIT_TIMEOUT) {
		std::wstring msg = L"[TIMEOUT] Child process ";
		msg += d->name;
		msg += L" exceeded 60 seconds and was terminated.\r\n";

		AppendFileAtomic(d->errorsPath, msg, d->hErrorsMutex);
		std::wcout << msg<<std::endl;
		TerminateProcess(d->pi.hProcess, 1);
	}
	delete d;
	return 0;
}

void AppendToEdit(HWND hEdit, const std::wstring& text) {
	int len = GetWindowTextLengthW(hEdit);
	SendMessageW(hEdit, EM_SETSEL, len, len);
	SendMessageW(hEdit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
	SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
}

void UpdateSoldDisplay() {
	double sold = ReadNumberFile(PathSold);
	std::wstring soldStr = L"Sold: " + std::to_wstring(sold);
	SetWindowTextW(g_hSoldText, soldStr.c_str());
}

void UpdateDonationsDisplay() {
	double donations = ReadNumberFile(PathDonations);
	std::wstring donationsStr = L"Donations: " + std::to_wstring(donations);
	SetWindowTextW(g_hDonationsText, donationsStr.c_str());
}

void UpdateLogsDisplay() {
	HANDLE hFile = CreateFileW(PathLogs.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD fileSize = GetFileSize(hFile, NULL);
		if (fileSize != INVALID_FILE_SIZE && fileSize > 0) {
			std::vector<wchar_t> buffer(fileSize / sizeof(wchar_t) + 1);
			DWORD bytesRead;
			if (ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL)) {
				buffer[bytesRead / sizeof(wchar_t)] = L'\0';
				SetWindowTextW(g_hLogsEdit, buffer.data());

				SendMessageW(g_hLogsEdit, EM_SETSEL, 0, -1);
				SendMessageW(g_hLogsEdit, EM_SETSEL, -1, -1);
				SendMessageW(g_hLogsEdit, EM_SCROLLCARET, 0, 0);
			}
		}
		CloseHandle(hFile);
	}
}

void UpdateErrorsDisplay() {
	HANDLE hFile = CreateFileW(PathErrors.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD fileSize = GetFileSize(hFile, NULL);
		if (fileSize != INVALID_FILE_SIZE && fileSize > 0) {
			std::vector<wchar_t> buffer(fileSize / sizeof(wchar_t) + 1);
			DWORD bytesRead;
			if (ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL)) {
				buffer[bytesRead / sizeof(wchar_t)] = L'\0';
				SetWindowTextW(g_hErrorsEdit, buffer.data());

				SendMessageW(g_hErrorsEdit, EM_SETSEL, 0, -1);
				SendMessageW(g_hErrorsEdit, EM_SETSEL, -1, -1);
				SendMessageW(g_hErrorsEdit, EM_SCROLLCARET, 0, 0);
			}
		}
		CloseHandle(hFile);
	}
}

int RunManagementProcess() {
	CreateDirectory(ReportsRoot);
	CreateDirectory(SummaryRoot);

	HANDLE hSoldMutex = CreateMutexW(NULL, FALSE, L"SoldMutex");
	HANDLE hDonationsMutex = CreateMutexW(NULL, FALSE, L"DonationsMutex");
	HANDLE hShelvesMutex = CreateMutexW(NULL, FALSE, L"ShelvesMutex");
	HANDLE hValabilityMutex = CreateMutexW(NULL, FALSE, L"ValabilityMutex");
	HANDLE hPricesMutex = CreateMutexW(NULL, FALSE, L"PricesMutex");
	HANDLE hLogsMutex = CreateMutexW(NULL, FALSE, L"LogsMutex");
	HANDLE hErrorsMutex = CreateMutexW(NULL, FALSE, L"ErrorsMutex");
	if (hSoldMutex == NULL || hDonationsMutex == NULL || hShelvesMutex == NULL || hValabilityMutex == NULL || hPricesMutex == NULL || hLogsMutex == NULL || hErrorsMutex == NULL) {
		wprintf(L"Failed to create/open mutexes. ErrorCode: %lu\n", GetLastError());
		if (hSoldMutex) CloseHandle(hSoldMutex);
		if (hDonationsMutex) CloseHandle(hDonationsMutex);
		if (hShelvesMutex) CloseHandle(hShelvesMutex);
		if (hValabilityMutex) CloseHandle(hValabilityMutex);
		if (hPricesMutex) CloseHandle(hPricesMutex);
		if (hLogsMutex) CloseHandle(hLogsMutex);
		if (hErrorsMutex) CloseHandle(hErrorsMutex);
		return 1;
	}

	HANDLE hMapShelves = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MAX_ITEMS * sizeof(DWORD), L"MarketShelves");
	HANDLE hMapValability = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MAX_ITEMS * sizeof(DWORD), L"MarketValability");
	HANDLE hMapPrices = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MAX_ITEMS * sizeof(double), L"MarketPrices");
	if (hMapShelves == NULL || hMapValability == NULL || hMapPrices == NULL) {
		wprintf(L"Failed to create/open file mappings. ErrorCode: %lu\n", GetLastError());
		if (hMapShelves) CloseHandle(hMapShelves);
		if (hMapValability) CloseHandle(hMapValability);
		if (hMapPrices) CloseHandle(hMapPrices);
		CloseHandle(hSoldMutex);
		CloseHandle(hDonationsMutex);
		return 1;
	}
	DWORD* shelves = (DWORD*)MapViewOfFile(hMapShelves, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	DWORD* valability = (DWORD*)MapViewOfFile(hMapValability, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	DWORD* prices = (DWORD*)MapViewOfFile(hMapPrices, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (shelves == NULL || valability == NULL || prices == NULL) {
		wprintf(L"Failed to map views of file mappings. ErrorCode: %lu\n", GetLastError());
		if (shelves) UnmapViewOfFile(shelves);
		if (valability) UnmapViewOfFile(valability);
		if (prices) UnmapViewOfFile(prices);
		CloseHandle(hMapShelves);
		CloseHandle(hMapValability);
		CloseHandle(hMapPrices);
		CloseHandle(hSoldMutex);
		CloseHandle(hDonationsMutex);
		return 1;
	}
	for (size_t i = 0; i < MAX_ITEMS; ++i) {
		shelves[i] = EMPTY;
		valability[i] = EMPTY;
		prices[i] = EMPTY;
	}

	WriteNumberFileAtomic(PathSold, 0.0, hSoldMutex);
	WriteNumberFileAtomic(PathDonations, 0.0, hDonationsMutex);

	UpdateSoldDisplay();
	UpdateDonationsDisplay();

	HANDLE hDepositSemaphore = CreateSemaphoreW(NULL, MAX_ITEMS, MAX_ITEMS, L"DepositSemaphore");
	HANDLE hSellSemaphore = CreateSemaphoreW(NULL, 0, MAX_ITEMS, L"SellSemaphore");
	HANDLE hDonateSemaphore = CreateSemaphoreW(NULL, MAX_ITEMS, MAX_ITEMS, L"DonateSemaphore");
	if (hDepositSemaphore == NULL || hSellSemaphore == NULL || hDonateSemaphore == NULL) {
		wprintf(L"Failed to create/open semaphores. ErrorCode: %lu\n", GetLastError());
		if (hDepositSemaphore) CloseHandle(hDepositSemaphore);
		if (hSellSemaphore) CloseHandle(hSellSemaphore);
		if (hDonateSemaphore) CloseHandle(hDonateSemaphore);
		UnmapViewOfFile(shelves);
		UnmapViewOfFile(valability);
		UnmapViewOfFile(prices);
		CloseHandle(hMapShelves);
		CloseHandle(hMapValability);
		CloseHandle(hMapPrices);
		CloseHandle(hSoldMutex);
		CloseHandle(hDonationsMutex);
		return 1;
	}

	HANDLE evDepositDone = CreateEventW(NULL, FALSE, FALSE, L"DepositDoneEvent");
	HANDLE evSellDone = CreateEventW(NULL, FALSE, FALSE, L"SellDoneEvent");
	HANDLE evDonateDone = CreateEventW(NULL, FALSE, FALSE, L"DonateDoneEvent");
	HANDLE evNextDay = CreateEventW(NULL, FALSE, FALSE, L"NextDayEvent");
	if (evDepositDone == NULL || evSellDone == NULL || evDonateDone == NULL || evNextDay == NULL) {
		wprintf(L"Failed to create/open events. ErrorCode: %lu\n", GetLastError());
		if (evDepositDone) CloseHandle(evDepositDone);
		if (evSellDone) CloseHandle(evSellDone);
		if (evDonateDone) CloseHandle(evDonateDone);
		if (evNextDay) CloseHandle(evNextDay);
		CloseHandle(hDepositSemaphore);
		CloseHandle(hSellSemaphore);
		CloseHandle(hDonateSemaphore);
		UnmapViewOfFile(shelves);
		UnmapViewOfFile(valability);
		UnmapViewOfFile(prices);
		CloseHandle(hMapShelves);
		CloseHandle(hMapValability);
		CloseHandle(hMapPrices);
		CloseHandle(hSoldMutex);
		CloseHandle(hDonationsMutex);
		return 1;
	}

	WaitForSingleObject(hLogsMutex, INFINITE);
	HANDLE hLogsFile = CreateFileW(PathLogs.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hLogsFile == INVALID_HANDLE_VALUE) {
		wprintf(L"Failed to open logs file: %s. ErrorCode: %lu\n", PathLogs.c_str(), GetLastError());
		ReleaseMutex(hLogsMutex);
		return 1;
	}
	if (!CloseHandle(hLogsFile)) {
		wprintf(L"Failed to close logs file: %s. ErrorCode: %lu\n", PathLogs.c_str(), GetLastError());
	}
	if (!ReleaseMutex(hLogsMutex)) {
		wprintf(L"Failed to release LogsMutex. ErrorCode: %lu\n", GetLastError());
	}

	STARTUPINFO siDeposit = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION piDeposit = { 0 };
	if (!CreateProcessW(L".\\deposit.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &siDeposit, &piDeposit)) {
		wprintf(L"Failed to launch deposit.exe. ErrorCode: %lu\n", GetLastError());
	}

	STARTUPINFO siSell = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION piSell = { 0 };
	if (!CreateProcessW(L".\\sell.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &siSell, &piSell)) {
		wprintf(L"Failed to launch sell.exe. ErrorCode: %lu\n", GetLastError());
	}

	STARTUPINFO siDonate = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION piDonate = { 0 };
	if (!CreateProcessW(L".\\donate.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &siDonate, &piDonate)) {
		wprintf(L"Failed to launch donate.exe. ErrorCode: %lu\n", GetLastError());
	}

	auto StartMonitor = [&](PROCESS_INFORMATION pi, const wchar_t* name) {
		MonitorData* d = new MonitorData{ pi, hErrorsMutex, name, PathErrors };
		CreateThread(NULL, 0, TimeoutMonitor, d, 0, NULL);
		};

	StartMonitor(piDeposit, L"deposit.exe");
	StartMonitor(piSell, L"sell.exe");
	StartMonitor(piDonate, L"donate.exe");

	auto depositDays = ListFilesSorted(g_depositDir);
	for (auto& dayFile : depositDays) {
		if (WaitForSingleObject(g_hStopEvent, 0) == WAIT_OBJECT_0) {
			wprintf(L"Stop requested by user. Terminating child processes...\n");
			TerminateProcess(piDeposit.hProcess, 0);
			TerminateProcess(piSell.hProcess, 0);
			TerminateProcess(piDonate.hProcess, 0);
			break;
		}

		std::wprintf(L"Processing day file: %s\n", dayFile.c_str());
		ResetEvent(evNextDay);
		ResetEvent(evDepositDone);
		ResetEvent(evSellDone);
		ResetEvent(evDonateDone);

		ReleaseSemaphore(hDepositSemaphore, 1, NULL);
		WaitForSingleObject(evDepositDone, INFINITE);

		ReleaseSemaphore(hSellSemaphore, 1, NULL);
		WaitForSingleObject(evSellDone, INFINITE);

		ReleaseSemaphore(hDonateSemaphore, 1, NULL);
		WaitForSingleObject(evDonateDone, INFINITE);

		bool errorOccurred = false;
		std::wstring errorMsg = L"";
		WaitForSingleObject(hShelvesMutex, INFINITE);
		WaitForSingleObject(hValabilityMutex, INFINITE);
		WaitForSingleObject(hPricesMutex, INFINITE);
		for (int shelveId = 0; shelveId < MAX_ITEMS; shelveId++) {
			DWORD itemId = shelves[shelveId];
			if (itemId != EMPTY) {
				if (valability[itemId] == EMPTY || prices[itemId] == EMPTY) {
					errorOccurred = true;
					errorMsg += L"Raftul " + std::to_wstring(shelveId) + L" conține produsul " + std::to_wstring(itemId) + L" cu date invalide.\n";
				}
			}
		}
		for (int itemId = 0; itemId < MAX_ITEMS; itemId++) {
			if (valability[itemId] != EMPTY || prices[itemId] != EMPTY) {
				bool found = false;
				for (int shelveId = 0; shelveId < MAX_ITEMS; shelveId++) {
					if (shelves[shelveId] == itemId) {
						found = true;
						break;
					}
				}
				if (!found) {
					errorOccurred = true;
					errorMsg += L"Produsul " + std::to_wstring(itemId) + L" are valabilitate sau pret dar nu este pe niciun raft.\n";
				}
			}
		}
		ReleaseMutex(hPricesMutex);
		ReleaseMutex(hValabilityMutex);
		ReleaseMutex(hShelvesMutex);
		if (errorOccurred) {
			AppendFileAtomic(PathErrors, errorMsg, hErrorsMutex);
		}

		UpdateSoldDisplay();
		UpdateDonationsDisplay();
		UpdateLogsDisplay();
		UpdateErrorsDisplay();

		SetEvent(evNextDay);
	}

	std::wprintf(L"All day files processed. Waiting for child processes to finish...\n");

	WaitForSingleObject(piDeposit.hProcess, INFINITE);
	WaitForSingleObject(piSell.hProcess, INFINITE);
	WaitForSingleObject(piDonate.hProcess, INFINITE);

	UpdateSoldDisplay();
	UpdateDonationsDisplay();
	UpdateLogsDisplay();
	UpdateErrorsDisplay();

	HANDLE hErrFile = CreateFileW(PathErrors.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hErrFile != INVALID_HANDLE_VALUE) {
		DWORD fileSize = GetFileSize(hErrFile, NULL);
		if (fileSize == INVALID_FILE_SIZE) {
			wprintf(L"Failed to get errors file size: %s. ErrorCode: %lu\n", PathErrors.c_str(), GetLastError());
		}
		else if (fileSize > 0) {
			std::string content(fileSize, '\0');
			DWORD bytesRead;
			if (!ReadFile(hErrFile, &content[0], fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
				wprintf(L"Failed to read file:. ErrorCode: %lu\n", GetLastError());
				CloseHandle(hErrFile);
			}
			else {
				wprintf(L"Errors logged during processing:\n");
				printf("%s\n", content.c_str());
			}
		}
		else {
			wprintf(L"No errors logged during processing.\n");
		}
		if (!CloseHandle(hErrFile)) {
			wprintf(L"Failed to close errors file handle. ErrorCode: %lu\n", GetLastError());
		}
	}
	else {
		double sold = ReadNumberFile(PathSold);
		double donations = ReadNumberFile(PathDonations);
		wprintf(L"Final sold: %.2f\n", sold);
		wprintf(L"Final donations: %.2f\n", donations);
	}

	if (!CloseHandle(hSoldMutex)) {
		wprintf(L"Failed to close SoldMutex handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hDonationsMutex)) {
		wprintf(L"Failed to close DonationsMutex handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hShelvesMutex)) {
		wprintf(L"Failed to close ShelvesMutex handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hValabilityMutex)) {
		wprintf(L"Failed to close ValabilityMutex handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hPricesMutex)) {
		wprintf(L"Failed to close PricesMutex handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hLogsMutex)) {
		wprintf(L"Failed to close LogsMutex handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hErrorsMutex)) {
		wprintf(L"Failed to close ErrorsMutex handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!UnmapViewOfFile(shelves)) {
		wprintf(L"Failed to unmap Shelves view. ErrorCode: %lu\n", GetLastError());
	}
	if (!UnmapViewOfFile(valability)) {
		wprintf(L"Failed to unmap Valability view. ErrorCode: %lu\n", GetLastError());
	}
	if (!UnmapViewOfFile(prices)) {
		wprintf(L"Failed to unmap Prices view. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hMapShelves)) {
		wprintf(L"Failed to close MarketShelves mapping handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hMapValability)) {
		wprintf(L"Failed to close MarketValability mapping handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hMapPrices)) {
		wprintf(L"Failed to close MarketPrices mapping handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hDepositSemaphore)) {
		wprintf(L"Failed to close DepositSemaphore handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hSellSemaphore)) {
		wprintf(L"Failed to close SellSemaphore handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hDonateSemaphore)) {
		wprintf(L"Failed to close DonateSemaphore handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(piDeposit.hProcess)) {
		wprintf(L"Failed to close deposit.exe process handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(piDeposit.hThread)) {
		wprintf(L"Failed to close deposit.exe thread handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(piSell.hProcess)) {
		wprintf(L"Failed to close sell.exe process handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(piSell.hThread)) {
		wprintf(L"Failed to close sell.exe thread handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(piDonate.hProcess)) {
		wprintf(L"Failed to close donate.exe process handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(piDonate.hThread)) {
		wprintf(L"Failed to close donate.exe thread handle. ErrorCode: %lu\n", GetLastError());
	}

	g_isRunning = false;
	EnableWindow(g_hBtnRun, TRUE);
	EnableWindow(g_hBtnStop, FALSE);
	EnableWindow(g_hBtnBrowseDeposit, TRUE);
	EnableWindow(g_hBtnBrowseSell, TRUE);

	return 0;
}

void WorkerThreadProc() {
	RunManagementProcess();
}

int BrowseForFolder(HWND hwnd, std::wstring& selectedPath) {
	BROWSEINFOW bi = {0};
	bi.hwndOwner = hwnd;
	bi.lpszTitle = L"Select Folder";
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
	if (pidl != NULL) {
		wchar_t path[MAX_PATH];
		if (SHGetPathFromIDListW(pidl, path)) {
			selectedPath = path;
			CoTaskMemFree(pidl);
			return 1;
		}
		CoTaskMemFree(pidl);
	}
	return 0;
}

#define ID_BTN_BROWSE_DEPOSIT 1001
#define ID_BTN_BROWSE_SELL 1002
#define ID_BTN_RUN 1003
#define ID_BTN_STOP 1004
#define ID_TIMER_UPDATE 1005

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
	{
		CreateWindowW(L"STATIC", L"Deposit Directory:", WS_VISIBLE | WS_CHILD,
			10, 10, 150, 20, hwnd, NULL, NULL, NULL);
		g_hDepositPath = CreateWindowW(L"EDIT", g_depositDir.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY,
			10, 35, 500, 25, hwnd, NULL, NULL, NULL);
		g_hBtnBrowseDeposit = CreateWindowW(L"BUTTON", L"Browse...", WS_VISIBLE | WS_CHILD,
			520, 35, 80, 25, hwnd, (HMENU)ID_BTN_BROWSE_DEPOSIT, NULL, NULL);

		CreateWindowW(L"STATIC", L"Sold Directory:", WS_VISIBLE | WS_CHILD,
			10, 70, 150, 20, hwnd, NULL, NULL, NULL);
		g_hSellPath = CreateWindowW(L"EDIT", g_soldDir.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY,
			10, 95, 500, 25, hwnd, NULL, NULL, NULL);
		g_hBtnBrowseSell = CreateWindowW(L"BUTTON", L"Browse...", WS_VISIBLE | WS_CHILD,
			520, 95, 80, 25, hwnd, (HMENU)ID_BTN_BROWSE_SELL, NULL, NULL);

		g_hBtnRun = CreateWindowW(L"BUTTON", L"Run Management", WS_VISIBLE | WS_CHILD,
			10, 130, 150, 30, hwnd, (HMENU)ID_BTN_RUN, NULL, NULL);
		g_hBtnStop = CreateWindowW(L"BUTTON", L"Stop", WS_VISIBLE | WS_CHILD | WS_DISABLED,
			170, 130, 100, 30, hwnd, (HMENU)ID_BTN_STOP, NULL, NULL);

		g_hSoldText = CreateWindowW(L"STATIC", L"Sold: 0.00", WS_VISIBLE | WS_CHILD,
			10, 170, 250, 20, hwnd, NULL, NULL, NULL);
		g_hDonationsText = CreateWindowW(L"STATIC", L"Donations: 0.00", WS_VISIBLE | WS_CHILD,
			270, 170, 250, 20, hwnd, NULL, NULL, NULL);

		CreateWindowW(L"STATIC", L"Logs:", WS_VISIBLE | WS_CHILD,
			10, 200, 100, 20, hwnd, NULL, NULL, NULL);
		g_hLogsEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
			10, 225, 580, 150, hwnd, NULL, NULL, NULL);

		CreateWindowW(L"STATIC", L"Errors:", WS_VISIBLE | WS_CHILD,
			10, 385, 100, 20, hwnd, NULL, NULL, NULL);
		g_hErrorsEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
			10, 410, 580, 150, hwnd, NULL, NULL, NULL);

		g_hStopEvent = CreateEventW(NULL, TRUE, FALSE, L"ManagementStopEvent");

		SetTimer(hwnd, ID_TIMER_UPDATE, 500, NULL);
		break;
	}

	case WM_TIMER:
		if (wParam == ID_TIMER_UPDATE && g_isRunning) {
			UpdateSoldDisplay();
			UpdateDonationsDisplay();
			UpdateLogsDisplay();
			UpdateErrorsDisplay();
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_BTN_BROWSE_DEPOSIT:
			if (BrowseForFolder(hwnd, g_depositDir)) {
				SetWindowTextW(g_hDepositPath, g_depositDir.c_str());
			}
			break;

		case ID_BTN_BROWSE_SELL:
			if (BrowseForFolder(hwnd, g_soldDir)) {
				SetWindowTextW(g_hSellPath, g_soldDir.c_str());
			}
			break;

		case ID_BTN_RUN:
			if (!g_isRunning) {
				g_isRunning = true;
				ResetEvent(g_hStopEvent);
				EnableWindow(g_hBtnRun, FALSE);
				EnableWindow(g_hBtnStop, TRUE);
				EnableWindow(g_hBtnBrowseDeposit, FALSE);
				EnableWindow(g_hBtnBrowseSell, FALSE);

				SetWindowTextW(g_hLogsEdit, L"");
				SetWindowTextW(g_hErrorsEdit, L"");
				SetWindowTextW(g_hSoldText, L"Sold: 0.00");
				SetWindowTextW(g_hDonationsText, L"Donations: 0.00");

				if (g_workerThread != nullptr) {
					if (g_workerThread->joinable()) {
						g_workerThread->join();
					}
					delete g_workerThread;
				}
				g_workerThread = new std::thread(WorkerThreadProc);
			}
			break;

		case ID_BTN_STOP:
			if (g_isRunning) {
				SetEvent(g_hStopEvent);
				MessageBoxW(hwnd, L"Stop signal sent. Please wait for processes to terminate", L"Stop", MB_OK | MB_ICONINFORMATION);
			}
			break;
		}
		break;

	case WM_DESTROY:
		KillTimer(hwnd, ID_TIMER_UPDATE);
		if (g_isRunning) {
			SetEvent(g_hStopEvent);
		}
		if (g_workerThread != nullptr) {
			if (g_workerThread->joinable()) {
				g_workerThread->join();
			}
			delete g_workerThread;
		}
		if (g_hStopEvent) {
			CloseHandle(g_hStopEvent);
		}
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow
)
{
	CoInitialize(NULL);

    WNDCLASSW wcex;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"Management";

	if (!RegisterClassW(&wcex)) {
		MessageBoxW(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
		CoUninitialize();
		return 0;
	}

	HWND hwnd = CreateWindowW(
		L"Management",
		L"Management",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		620, 620,
		NULL, NULL, hInstance, NULL
	);

	if (hwnd == NULL) {
		MessageBoxW(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
		CoUninitialize();
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	CoUninitialize();
	return (int)msg.wParam;
}

