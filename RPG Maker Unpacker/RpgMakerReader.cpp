#include "stdafx.h"
#include "RpgMakerReader.h"


RpgMakerReader::RpgMakerReader(LPWSTR path) {
	lpInputPath = path;
	GetDir(lpInputPath, szDir);
}


RpgMakerReader::~RpgMakerReader() {}


bool RpgMakerReader::Open() {
	hFile = CreateFile(
		lpInputPath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	
	return hFile != INVALID_HANDLE_VALUE;
}

void RpgMakerReader::Extract() {
	if (Open()) switch (GetVersion()) {
	case 1: ExtractV1(); break;
	case 3: ExtractV3(); break;
	}
}

void RpgMakerReader::ExtractV1() {
	uint32_t nameLength, fileLength;
	uint8_t fileName[MAX_PATH*3];
	WCHAR szFileName[MAX_PATH];
	uint8_t *buffer;

	// Set file encryption key
	key.uint32 = 0xDEADCAFE;

	for (;;) {

		// Get file name length
		if (!ReadFile(hFile, &nameLength, 4, nullptr, nullptr)) goto Cleanup;
		DecryptIntV1(&nameLength);

		// Get file name
		if (nameLength > sizeof(fileName)) goto Cleanup;
		if (!ReadFile(hFile, &fileName, nameLength, nullptr, nullptr)) goto Cleanup;
		fileName[nameLength] = 0;
		DecryptNameV1(fileName, nameLength);

		// UTF-8 to UTF-16
		MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) fileName, nameLength + 1, szFileName, sizeof(szFileName));

		// Get file length
		if (!ReadFile(hFile, &fileLength, 4, nullptr, nullptr)) goto Cleanup;
		DecryptIntV1(&fileLength);

		// Allocate memory
		buffer = (uint8_t*) VirtualAlloc(nullptr, fileLength, MEM_COMMIT, PAGE_READWRITE);
		if (buffer == nullptr) goto Cleanup;

		// Read file
		if (!ReadFile(hFile, buffer, fileLength, nullptr, nullptr)) goto Cleanup;
		DecryptFileData(buffer, fileLength, key);

		// Save file
		SaveFile(szFileName, buffer, fileLength);

		// Free memory
		VirtualFree(buffer, NULL, MEM_RELEASE);
	}

	return;

Cleanup:
	;
}

void RpgMakerReader::ExtractV3() {
	LONG currentPointer;
	uint32_t nameLength, fileLength, fileOffset;
	uint8_t fileName[MAX_PATH * 3];
	WCHAR szFileName[MAX_PATH];
	uint8_t *buffer;
	key_t fileKey;

	// Get file encryption key
	if (!ReadFile(hFile, &key.uint32, 4, nullptr, nullptr)) goto Cleanup;
	key.int32 = key.int32 * 9 + 3;

	for (;;) {

		// Get file offset
		if (!ReadFile(hFile, &fileOffset, 4, nullptr, nullptr)) goto Cleanup;
		DecryptIntV3(&fileOffset);

		// Get file length
		if (!ReadFile(hFile, &fileLength, 4, nullptr, nullptr)) goto Cleanup;
		DecryptIntV3(&fileLength);

		// Get file key
		if (!ReadFile(hFile, &fileKey.uint32, 4, nullptr, nullptr)) goto Cleanup;
		DecryptIntV3(&fileKey.uint32);

		// Get file name length
		if (!ReadFile(hFile, &nameLength, 4, nullptr, nullptr)) goto Cleanup;
		DecryptIntV3(&nameLength);

		// Normal exit from loop
		if (fileOffset == 0) goto Cleanup;
		
		// Get file name
		if (nameLength > sizeof(fileName)) goto Cleanup;
		if (!ReadFile(hFile, &fileName, nameLength, nullptr, nullptr)) goto Cleanup;
		fileName[nameLength] = 0;
		DecryptNameV3(fileName, nameLength);

		// UTF-8 to UTF-16
		MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) fileName, nameLength + 1, szFileName, sizeof(szFileName));

		// Allocate memory
		buffer = (uint8_t*) VirtualAlloc(nullptr, fileLength, MEM_COMMIT, PAGE_READWRITE);
		if (buffer == nullptr) goto Cleanup;

		// Save file position pointer
		currentPointer = SetFilePointer(hFile, 0, 0, FILE_CURRENT);

		// Read file
		SetFilePointer(hFile, fileOffset, 0, FILE_BEGIN);
		if (!ReadFile(hFile, buffer, fileLength, nullptr, nullptr)) goto Cleanup;
		DecryptFileData(buffer, fileLength, fileKey);

		// Save file
		SaveFile(szFileName, buffer, fileLength);

		// Free memory
		VirtualFree(buffer, NULL, MEM_RELEASE);

		// Restore file position pointer
		SetFilePointer(hFile, currentPointer, 0, FILE_BEGIN);
	}

	return;

Cleanup:
	ShowLastError();
	;
}

int RpgMakerReader::GetVersion() {
	CHAR magic[8];

	try {
		// Reset file position
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		if (ReadFile(hFile, magic, 8, nullptr, nullptr)) {
			if (strncmp(magic, "RGSSAD", 6) == 0) {
				return magic[7];
			}
		}
	} catch (DWORD ex) {
		UNREFERENCED_PARAMETER(ex);
		ShowLastError();
	}

	return 0;
}

void RpgMakerReader::SaveFile(LPWSTR path, uint8_t *data, uint32_t length) {
	WCHAR szFullPath[MAX_PATH];
	WCHAR szDirectoryPath[MAX_PATH];

	_snwprintf(szFullPath, MAX_PATH, L"%s\\Extract\\%s", szDir, path);
	GetDir(szFullPath, szDirectoryPath);

	if (CreateDirectoryRecursively(szDirectoryPath)) {
		HANDLE hOutputFile = CreateFileW(szFullPath, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		WriteFile(hOutputFile, data, length, nullptr, nullptr);
		CloseHandle(hOutputFile);
	} else {
		ShowLastError();
	}
}

void RpgMakerReader::DecryptIntV1(uint32_t *value) {
	*value ^= key.uint32;
	key.int32 = key.int32 * 7 + 3;
}

void RpgMakerReader::DecryptIntV3(uint32_t *value) {
	*value = *value ^ key.uint32;
}

void RpgMakerReader::DecryptNameV1(uint8_t *string, uint32_t length) {
	for (uint32_t i = 0; i < length; i++) {
		string[i] ^= key.uint8[0]; // Little endian (LSB)
		key.int32 = key.int32 * 7 + 3;
	}
}

void RpgMakerReader::DecryptNameV3(uint8_t *string, uint32_t length) {
	for (uint32_t i = 0; i < length; i++) {
		string[i] ^= key.uint8[i % 4];
	}
}

void RpgMakerReader::DecryptFileData(uint8_t *data, uint32_t length, key_t fileKey) {
	uint32_t i;
	for (i = 0; i < length / 4; i++) {
		((uint32_t*) data)[i] ^= fileKey.uint32;
		fileKey.int32 = fileKey.int32 * 7 + 3;
	}

	for (i = i * 4; i < length; i++) {
		data[i] ^= fileKey.uint8[i % 4];
	}
}
