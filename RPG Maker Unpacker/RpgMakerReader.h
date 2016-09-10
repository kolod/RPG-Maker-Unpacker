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

