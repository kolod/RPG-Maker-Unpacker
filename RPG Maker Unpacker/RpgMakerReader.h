#pragma once

#include "stdafx.h"

typedef union {
	int32_t int32;
	uint32_t uint32;
	uint8_t uint8[4];
} key_t;

class RpgMakerReader {
public:
	RpgMakerReader(LPWSTR path);
	~RpgMakerReader();

	bool Open();
	void Extract();

private:
	LPCWSTR lpInputPath;
	WCHAR szDir[MAX_PATH];
	HANDLE hFile;
	key_t key;

	int GetVersion();

	void DecryptIntV1(uint32_t *value);
	void DecryptIntV3(uint32_t *value);

	void DecryptNameV1(uint8_t *string, uint32_t length);
	void DecryptNameV3(uint8_t *string, uint32_t length);

	void DecryptFileData(uint8_t *data, uint32_t length, key_t fileKey);

	void ExtractV1();
	void ExtractV3();

	void SaveFile(LPWSTR path, uint8_t *data, uint32_t length);
};
