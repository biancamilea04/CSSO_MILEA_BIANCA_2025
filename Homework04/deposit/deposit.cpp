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

struct DayEntry {
	int year, month, day;
	std::wstring filename;
};

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

std::string ReadAllFileA(const std::wstring& path) {
	HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		wprintf(L"Failed to open file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
		return "";
	}

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE) {
		wprintf(L"Failed to get file size: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
		CloseHandle(hFile);
		return "";
	}

	std::string content(fileSize, '\0');
	DWORD bytesRead;
	if (!ReadFile(hFile, &content[0], fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
		wprintf(L"Failed to read file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
		CloseHandle(hFile);
		return "";
	}

	if (!CloseHandle(hFile)) {
		wprintf(L"Failed to close file: %s. ErrorCode: %lu\n", path.c_str(), GetLastError());
	}
	return content;
}

bool ParseDateFile(const std::wstring& fileName, int& year, int& month, int& day) {
	std::wstring base = fileName;
	size_t dotPosition = base.rfind(L'.txt');
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

//sorteaza fisierele dupa data
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

int main()
{
	//deschide „MarketShelves”, „MarketValability”, „ProductPrices” și obține pointerul lor
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
	if (!hShelvesMutex || !hValabilityMutex || !hPricesMutex || !hLogsMutex || !hErrorsMutex) {
		wprintf(L"Failed to open mutexes. ErrorCode: %lu\n", GetLastError());
		return 1;
	}

	HANDLE hDepositSemaphore = OpenSemaphoreW(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, L"DepositSemaphore");
	if (!hDepositSemaphore) {
		wprintf(L"Failed to open deposit semaphore. ErrorCode: %lu\n", GetLastError());
		return 1;
	}

	HANDLE evDepositDone = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, L"DepositDoneEvent");
	HANDLE evNextDay = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, L"NextDayEvent");
	if (!evDepositDone || !evNextDay) {
		wprintf(L"Failed to open events. ErrorCode: %lu\n", GetLastError());
		return 1;
	}


    //sorteaza fisierul deposit dupa data
	auto dayFiles = ListFilesSorted(L"deposit");

	for (auto& df : dayFiles) {
		WaitForSingleObject(hDepositSemaphore, INFINITE);
		std::wstring filePath = L"deposit\\" + df;
		std::string content = ReadAllFileA(filePath);
		if (content.empty()) {
			SetEvent(evDepositDone);
			continue;
		}
		size_t pos = 0;
		while (pos < content.size()) {
			size_t nextPos = content.find('\n', pos);
			if (nextPos == std::string::npos) nextPos = content.size();
			std::string line = content.substr(pos, nextPos - pos);
			pos = nextPos + 1;

			DWORD itemId;
			DWORD expireInDays;
			DWORD shelveId;
			DWORD price;

			//extragem informatiile din linia fisierului (id_produs, expires_in, shelve_id, product_price)
			if (sscanf_s(line.c_str(), "%lu,%lu,%lu,%lu", &itemId, &expireInDays, &shelveId, &price) != 4) {
				wprintf(L"Invalid line format in file %s: %S\n", filePath.c_str(), line.c_str());
				std::wstring errorMsg = L"Invalid line format in file " + df + L": " + std::wstring(line.begin(), line.end()) + L"\r\n";
				AppendFileAtomic(PathErrors, errorMsg, hErrorsMutex);
				continue;
			}

			WaitForSingleObject(hShelvesMutex, INFINITE);
			if (shelves[shelveId] != EMPTY) { // shelves[shelve_id] ≠ 0xFFFFFFFF
				std::wstring errorMsg = L"S-a încercat adăugarea produsului " + std::to_wstring(itemId) + L" pe raftul " + std::to_wstring(shelveId) + L" care este deja ocupat de " + std::to_wstring(shelves[shelveId]) + L"\r\n";
				AppendFileAtomic(PathErrors, errorMsg, hErrorsMutex);
				ReleaseMutex(hShelvesMutex);
				continue;
			}
			else if (shelves[shelveId] == EMPTY) { // [shelve_id] == 0xFFFFFFFF
				WaitForSingleObject(hValabilityMutex, INFINITE);
				WaitForSingleObject(hPricesMutex, INFINITE);
				shelves[shelveId] = itemId;
				valability[itemId] = expireInDays;
				prices[itemId] = price;
				ReleaseMutex(hPricesMutex);
				ReleaseMutex(hValabilityMutex);
				ReleaseMutex(hShelvesMutex);

				std::wstring logMsg = L"Am adăugat pe raftul " + std::to_wstring(shelveId) + L" produsul " + std::to_wstring(itemId) \
					+ L" cu valabilitate " + std::to_wstring(expireInDays) + L" zile și preț " + std::to_wstring(price) + L"\r\n";
				AppendFileAtomic(PathLogs, logMsg, hLogsMutex);

			}
		}
		SetEvent(evDepositDone);
		WaitForSingleObject(evNextDay, INFINITE);
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

