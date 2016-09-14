//    RPG Maker Unpacker
//    Copyright (C) 2016  Alexandr Kolodkin <alexandr.kolodkin@gmail.com>
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "stdafx.h"
#include "RpgMakerReader.h"


RpgMakerReader::RpgMakerReader(LPWSTR path, ProgressCallback callback)
	: Reader(path, callback) {}

void RpgMakerReader::Extract() {
	if (ReserveMemory()) {
		if (Open()) switch (GetVersion()) {
		case 1: ExtractV1(); break;
		case 3: ExtractV3(); break;
		}

		CloseHandle(hFile);
		FreeMemory();
	}
}

void RpgMakerReader::ExtractV1() {
	uint32_t nameLength, fileLength;
	uint8_t fileName[MAX_PATH*3];
	WCHAR szFileName[MAX_PATH];

	// Set file encryption key
	key.uint32 = 0xDEADCAFE;

	for (;;) {

		// Get file name length
		if (!ReadFile(hFile, &nameLength, 4, nullptr, nullptr)) return;
		DecryptIntV1(&nameLength);

		// Get file name
		if (nameLength > sizeof(fileName)) return;
		if (!ReadFile(hFile, &fileName, nameLength, nullptr, nullptr)) return;
		fileName[nameLength] = 0;
		DecryptNameV1(fileName, nameLength);

		// UTF-8 to UTF-16
		MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) fileName, nameLength + 1, szFileName, sizeof(szFileName));

		// Get file length
		if (!ReadFile(hFile, &fileLength, 4, nullptr, nullptr)) return;
		DecryptIntV1(&fileLength);

		// Allocate memory
		CommitMemory(fileLength);

		// Read file
		if (!ReadFile(hFile, buffer, fileLength, nullptr, nullptr)) return;
		DecryptFileData(buffer, fileLength, key);

		// Save file
		SaveFile(szFileName, buffer, fileLength);

		// Update progress
		readed.QuadPart += 4 * 2 + fileLength + nameLength;
		UpdateProgress();
	}
}

void RpgMakerReader::ExtractV3() {
	LARGE_INTEGER currentPointer;
	uint32_t nameLength, fileLength, fileOffset;
	uint8_t fileName[MAX_PATH * 3];
	WCHAR szFileName[MAX_PATH];
	key_t fileKey;

	// Get file encryption key
	if (!ReadFile(hFile, &key.uint32, 4, nullptr, nullptr)) return;
	key.int32 = key.int32 * 9 + 3;

	for (;;) {

		// Get file offset
		if (!ReadFile(hFile, &fileOffset, 4, nullptr, nullptr)) return;
		DecryptIntV3(&fileOffset);

		// Get file length
		if (!ReadFile(hFile, &fileLength, 4, nullptr, nullptr)) return;
		DecryptIntV3(&fileLength);

		// Get file key
		if (!ReadFile(hFile, &fileKey.uint32, 4, nullptr, nullptr)) return;
		DecryptIntV3(&fileKey.uint32);

		// Get file name length
		if (!ReadFile(hFile, &nameLength, 4, nullptr, nullptr)) return;
		DecryptIntV3(&nameLength);

		// Normal exit from loop
		if (fileOffset == 0) return;
		
		// Get file name
		if (nameLength > sizeof(fileName)) return;
		if (!ReadFile(hFile, &fileName, nameLength, nullptr, nullptr)) return;
		fileName[nameLength] = 0;
		DecryptNameV3(fileName, nameLength);

		// UTF-8 to UTF-16
		MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) fileName, nameLength + 1, szFileName, sizeof(szFileName));

		// Allocate memory
		CommitMemory(fileLength);

		// Save file position pointer
		SetFilePointerEx(hFile, {0, 0}, &currentPointer, FILE_CURRENT);

		// Read file
		SetFilePointer(hFile, fileOffset, 0, FILE_BEGIN);
		if (!ReadFile(hFile, buffer, fileLength, nullptr, nullptr)) return;
		DecryptFileData(buffer, fileLength, fileKey);

		// Save file
		SaveFile(szFileName, buffer, fileLength);

		// Restore file position pointer
		SetFilePointerEx(hFile, currentPointer, nullptr, FILE_BEGIN);

		// Update progress
		readed.QuadPart += 4 * 4 + fileLength + nameLength;
		UpdateProgress();
	}
}

int RpgMakerReader::GetVersion() {
	CHAR magic[8];

	try {
		// Reset file position
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		// Read magic
		if (ReadFile(hFile, magic, 8, nullptr, nullptr)) {
			readed.QuadPart = 8;
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
