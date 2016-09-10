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

#pragma once

#include "stdafx.h"

typedef union {
	int32_t int32;
	uint32_t uint32;
	uint8_t uint8[4];
} key_t;

typedef void(*ProgressCallback)(int64_t readed, int64_t all);

class RpgMakerReader {
public:
	RpgMakerReader(LPWSTR path, ProgressCallback callback = nullptr);
	~RpgMakerReader();

	bool Open();
	void Extract();

private:
	ProgressCallback pCallback;
	LPCWSTR lpInputPath;
	WCHAR szDir[MAX_PATH];
	HANDLE hFile;
	key_t key;

	LARGE_INTEGER size;
	LARGE_INTEGER readed;

	int GetVersion();

	void DecryptIntV1(uint32_t *value);
	void DecryptIntV3(uint32_t *value);

	void DecryptNameV1(uint8_t *string, uint32_t length);
	void DecryptNameV3(uint8_t *string, uint32_t length);

	void DecryptFileData(uint8_t *data, uint32_t length, key_t fileKey);

	void ExtractV1();
	void ExtractV3();

	void SaveFile(LPWSTR path, uint8_t *data, uint32_t length);

	void UpdateProgress();
};

