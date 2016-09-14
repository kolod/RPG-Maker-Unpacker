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
#include "Reader.h"

Reader::Reader(LPWSTR path, ProgressCallback callback) {
	buffer = nullptr;
	bufferSize = 0;
	reservedSpace = 0;
	pageSize = GetLargePageMinimum();
	pCallback = callback;
	lpInputPath = path;
	GetDir(lpInputPath, szDir);
}

bool Reader::Open() {
	hFile = CreateFile(
		lpInputPath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	// Get size of the rpg maker archive
	if (!GetFileSizeEx(hFile, &size)) size.QuadPart = 0;

	return hFile != INVALID_HANDLE_VALUE;
}

// Reserve 1G of virtual space
bool Reader::ReserveMemory() {
	FreeMemory();
	reservedSpace = (0x40000000 / pageSize) * pageSize;
	buffer = (uint8_t*) VirtualAlloc(nullptr, reservedSpace, MEM_RESERVE, PAGE_NOACCESS);
	return buffer != nullptr;
}

bool Reader::FreeMemory() {
	if (buffer != nullptr) {
		if (VirtualFree(buffer, NULL, MEM_RELEASE)) {
			buffer = nullptr;
		} else {
			return false;
		}
	}

	return true;
}

bool Reader::CommitMemory(size_t size) {

	if (size > bufferSize) {
		size_t commitedSpace = ((size + pageSize - 1) / pageSize) * pageSize;
		buffer = (uint8_t*) VirtualAlloc(buffer, commitedSpace, MEM_COMMIT, PAGE_READWRITE);
		if (buffer != nullptr) bufferSize = commitedSpace;
	}

	return buffer != nullptr;
}

void Reader::UpdateProgress() {
	if (pCallback != nullptr) {
		pCallback(readed.QuadPart, size.QuadPart);
	}
}

void Reader::SaveFile(LPWSTR path, uint8_t *data, uint32_t length) {
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