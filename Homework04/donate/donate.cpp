#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>

constexpr auto MAX_ITEMS = 10000;
constexpr auto EMPTY = 0xFFFFFFFF;

static std::wstring ReportsRoot = L"C:\\Facultate\\CSSO\\H4\\Reports";
static std::wstring SummaryRoot = L"C:\\Facultate\\CSSO\\H4\\Reports\\Summary";
static std::wstring PathLogs = ReportsRoot + L"\\logs.txt";
static std::wstring PathSold = SummaryRoot + L"\\sold.txt";
static std::wstring PathDonations = SummaryRoot + L"\\donations.txt";
static std::wstring PathErrors = SummaryRoot + L"\\errors.txt";


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

int main()
{

	HANDLE hMapShelves = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MarketShelves");
	HANDLE hMapValability = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MarketValability");
	HANDLE hMapPrices = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MarketPrices");
	if (!hMapShelves || !hMapValability || !hMapPrices) {
		wprintf(L"Failed to open file mappings. ErrorCode: %lu\n", GetLastError());
		return 1;
	}

	DWORD* shelves = (DWORD*)MapViewOfFile(hMapShelves, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	DWORD* valability = (DWORD*)MapViewOfFile(hMapValability, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	DWORD* prices = (DWORD*)MapViewOfFile(hMapPrices, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (!shelves || !valability || !prices) {
		wprintf(L"Failed to map views of file mappings. ErrorCode: %lu\n", GetLastError());
		return 1;
	}

	HANDLE hShelvesMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, L"ShelvesMutex");
	HANDLE hValabilityMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, L"ValabilityMutex");
	HANDLE hPricesMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, L"PricesMutex");
	HANDLE hLogsMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, L"LogsMutex");
	HANDLE hErrorsMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, L"ErrorsMutex");
	HANDLE hDonationsMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, L"DonationsMutex");
	if (!hShelvesMutex || !hValabilityMutex || !hPricesMutex || !hLogsMutex || !hErrorsMutex || !hDonationsMutex) {
		wprintf(L"Failed to open mutexes. ErrorCode: %lu\n", GetLastError());
		return 1;
	}

	HANDLE hDonateSemaphore = OpenSemaphoreW(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, L"DonateSemaphore");
	if (!hDonateSemaphore) {
		wprintf(L"Failed to open donate semaphore. ErrorCode: %lu\n", GetLastError());
		return 1;
	}

	HANDLE evDonateDone = OpenEventW(EVENT_ALL_ACCESS, FALSE, L"DonateDoneEvent");
	HANDLE evNextDay = OpenEventW(EVENT_ALL_ACCESS, FALSE, L"NextDayEvent");
	if (!evDonateDone || !evNextDay) {
		wprintf(L"Failed to open events. ErrorCode: %lu\n", GetLastError());
		return 1;
	}

	while (true) {
		DWORD waitResult = WaitForSingleObject(hDonateSemaphore, INFINITE);
		if (waitResult != WAIT_OBJECT_0) {
			wprintf(L"Failed to wait for donate semaphore. ErrorCode: %lu\n", GetLastError());
			break;
		}
		for (int itemId = 0; itemId < MAX_ITEMS; itemId++) {
			WaitForSingleObject(hValabilityMutex, INFINITE);
			if (valability[itemId] == EMPTY) {
				ReleaseMutex(hValabilityMutex);
				continue;
			}
			else if (valability[itemId] == 0) {
				WaitForSingleObject(hPricesMutex, INFINITE);
				double itemPrice = prices[itemId];
				WaitForSingleObject(hDonationsMutex, INFINITE);
				double totalSold = ReadNumberFile(PathDonations);
				totalSold += itemPrice;
				WriteNumberFileAtomic(PathDonations, totalSold, hDonationsMutex);
				WaitForSingleObject(hLogsMutex, INFINITE);
				AppendFileAtomic(PathLogs, L"Produsul " + std::to_wstring(itemId) + L" a fost donat\n", hLogsMutex);
				ReleaseMutex(hLogsMutex);
				valability[itemId] = EMPTY;
				prices[itemId] = EMPTY;
				ReleaseMutex(hPricesMutex);
				ReleaseMutex(hDonationsMutex);
				ReleaseMutex(hValabilityMutex);
				WaitForSingleObject(hShelvesMutex, INFINITE);
				for (int s = 0; s < MAX_ITEMS; s++) {
					if (shelves[s] == (DWORD)itemId) {
						shelves[s] = EMPTY;
						break;
					}
				}
				ReleaseMutex(hShelvesMutex);
			}
			else {
				valability[itemId]--;
				ReleaseMutex(hValabilityMutex);
			}
		}
		SetEvent(evDonateDone);
		SetEvent(evNextDay);
	}
	if (!UnmapViewOfFile(shelves)) {
		wprintf(L"Failed to unmap shelves view. ErrorCode: %lu\n", GetLastError());
	}
	if (!UnmapViewOfFile(valability)) {
		wprintf(L"Failed to unmap valability view. ErrorCode: %lu\n", GetLastError());
	}
	if (!UnmapViewOfFile(prices)) {
		wprintf(L"Failed to unmap prices view. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hMapShelves)) {
		wprintf(L"Failed to close shelves mapping handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hMapValability)) {
		wprintf(L"Failed to close valability mapping handle. ErrorCode: %lu\n", GetLastError());
	}
	if (!CloseHandle(hMapPrices)) {
		wprintf(L"Failed to close prices mapping handle. ErrorCode: %lu\n", GetLastError());
	}
	return 0;
}
