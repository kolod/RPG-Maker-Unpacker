#include "stdafx.h"
#include "RenPyReader.h"
#include "zlib.h"


RenPyReader::RenPyReader(LPWSTR path, ProgressCallback callback) : Reader(path, callback) {}

void RenPyReader::Extract() {
	if (ReserveMemory()) {
		if (Open()) switch (GetVersion()) {
		case 1: ExtractV1(); break;
		case 2: ExtractV2(); break;
		case 3: ExtractV3(); break;
		}

		CloseHandle(hFile);
		FreeMemory();
	}
}

int RenPyReader::GetVersion() {
	CHAR magic[8];

	try {
		// TODO: if file extantion ".rti" file version 1 and no magic

		// Reset file position
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		// Read magic
		if (ReadFile(hFile, magic, 8, nullptr, nullptr)) {
			readed.QuadPart = 8;
			if (strncmp(magic, "RPA-2.0 ", 8) == 0) return 2;
			if (strncmp(magic, "RPA-3.0 ", 8) == 0) return 3;
		}
	} catch (DWORD ex) {
		UNREFERENCED_PARAMETER(ex);
		ShowLastError();
	}

	return 0;
}

bool RenPyReader::ReadHeader(int64_t *offset, uint32_t *key) {
	CHAR header[64];

	try {		
		if (SetFilePointer(hFile, 8, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) return false;  // Set file position		
		if (!ReadFile(hFile, header, sizeof(header), nullptr, nullptr)) return false;              // Read  header
		if (_snscanf(header, sizeof(header), "%llX %lX", offset, key) != 2) return false;          // Get offset & key
	} catch (DWORD ex) {
		UNREFERENCED_PARAMETER(ex);
		ShowLastError();
		return false;
	}

	return true;
}

void RenPyReader::ExtractV1() {
	// Set decryption key
	key.uint32 = 0xDEADBEEF;
}

void RenPyReader::ExtractV2() {

}

void RenPyReader::ExtractV3() {
	LARGE_INTEGER indexPointer;
	LARGE_INTEGER fileSize; 
	LARGE_INTEGER compressedIndexSize;
	LARGE_INTEGER uncompressedIndexSize;
	LARGE_INTEGER filePointer;

	UCHAR *compressedIndex;
	UCHAR *uncompressedIndex;

	WCHAR szFileName[MAX_PATH];

	uint32_t offset;
	uint32_t length;

	// Get index offset & decryption key
	if (!ReadHeader(&indexPointer.QuadPart, &key.uint32)) return;

	// Move file pointer to begining of archive index
	if (!SetFilePointerEx(hFile, indexPointer, nullptr, FILE_BEGIN)) return;

	// Get size to read
	if (!GetFileSizeEx(hFile, &fileSize)) return;
	compressedIndexSize.QuadPart = fileSize.QuadPart - indexPointer.QuadPart + 1;
	uncompressedIndexSize.QuadPart = compressedIndexSize.QuadPart * 10;
	if (uncompressedIndexSize.HighPart != 0) return;

	// Commit memmory
	compressedIndex = (UCHAR*) VirtualAlloc(nullptr, compressedIndexSize.LowPart, MEM_COMMIT, PAGE_READWRITE);
	if (compressedIndex == nullptr) return;
	uncompressedIndex = (UCHAR*) VirtualAlloc(nullptr, uncompressedIndexSize.LowPart, MEM_COMMIT, PAGE_READWRITE);
	if (uncompressedIndex == nullptr) return;

	// Read archive index
	if (!ReadFile(hFile, compressedIndex, compressedIndexSize.LowPart, nullptr, nullptr)) return;

	// Decompress indexes
	uncompress(uncompressedIndex, (uLongf*) &uncompressedIndexSize.QuadPart, compressedIndex, compressedIndexSize.LowPart);

	SaveFile(L"index.bin", uncompressedIndex, uncompressedIndexSize.LowPart);

	LPSTR position = (LPSTR) uncompressedIndex;
	LPSTR end = position + uncompressedIndexSize.LowPart - 1;

	while (position < end) {
		if (!NextBinput(&position, end)) break;
		if (!NextString(&position, end, szFileName, MAX_PATH)) break;
		if (!NextBinput(&position, end)) break;
		if (!NextInteger(&position, end, &offset)) break;
		if (!NextInteger(&position, end, &length)) break;
		// TODO: Add check for prefix

		filePointer.QuadPart = offset;
				
		if (!CommitMemory(length)) break;                                        // Allocate memory		
		if (!SetFilePointerEx(hFile, filePointer, nullptr, FILE_BEGIN)) break;   // Set file pointer
		if (!ReadFile(hFile, buffer, length, nullptr, nullptr)) break;           // Read file		
		SaveFile(szFileName, buffer, length);                                    // Save file

		// Update progress
		readed.QuadPart += length;
		UpdateProgress();
	}
}

bool RenPyReader::NextBinput(LPSTR *index, LPSTR end) {
	while (*index <= end) {
		switch (**index) {
		case 'q':
			*index += 1 + 1;
			return true;
		case 'r':
			*index += 1 + 4;
			return true;
		}
		*index = *index + 1;
	}

	return false;
}

bool RenPyReader::NextString(LPSTR *index, LPSTR end, LPWSTR name, size_t size) {

	// Read utf-8 file name length
	if (*index + 5 > end) return false;
	*index = *index + 2;
	uint32_t encodedLength = *((uint32_t*) *index);
	*index = *index + 4;

	// Read utf-8 file name
	if (*index + encodedLength - 1 > end) return false;
	int charactersWritten = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) *index, encodedLength, name, (int) size);
	if (charactersWritten == 0) return false;

	// Move index pointer
	*index = *index + encodedLength - 1;

	// Find end of string
	// FIXME: I don't know why it works, but it works. Errors may occur, if the prefix is not empty.
	while (**index != ']') {
		*index = *index + 1;
		if (*index > end) return false;
	}

	// Append NULL
	name[charactersWritten] = 0;

	// Replase slashes
	for (int i = 0; i < charactersWritten; i++) {
		if (name[i] == '/') name[i] = '\\';
	}

	return true;
}

bool  RenPyReader::NextInteger(LPSTR *index, LPSTR end, uint32_t *value) {
	if (*index + 5 > end) return false;
	if (**index != 'J') return false;
	*index = *index + 1;
	*value = *((uint32_t*) *index) ^ key.uint32;
	*index = *index + 4;
	return true;
}
