//    RPG Maker Unpacker
//    Copyright (C) 2016-...  Oleksandr Kolodkin <alexandr.kolodkin@gmail.com>
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

#define MAX_COMMAND (MAX_PATH + 50)

static LPCWCHAR gameEngine[] = {
	L"rpgmaker",
	L"renpy"
};

static LPCWCHAR fileExtentions[5][2] = {
	{L".rgssad", gameEngine[0]},
	{L".rgss2a", gameEngine[0]},
	{L".rgss3a", gameEngine[0]},
	{L".rpa", gameEngine[1]},
	{L".rpi", gameEngine[1]},
};

static const int fileExtentionsCount = sizeof(fileExtentions) / sizeof(fileExtentions[0]);


BOOL RegDeleteKeyRecurse(HKEY hKeyRoot, LPCWCHAR lpSubKey) {

	// First, see if we can delete the key without having to recurse.
	if (RegDeleteKey(hKeyRoot, lpSubKey) == ERROR_SUCCESS) return TRUE;

	// Open key
	HKEY hKey;
	LONG lResult = RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);
	if (lResult != ERROR_SUCCESS) {
		if (lResult == ERROR_FILE_NOT_FOUND) {
			printf("Key not found.\n");
			return TRUE;
		} else {
			printf("Error opening key.\n");
			return FALSE;
		}
	}

	// Enumerate the keys
	DWORD dwSize;
	WCHAR szName[MAX_PATH];
	WCHAR szSubKey[MAX_PATH];
	for (;;) {
		dwSize = MAX_PATH;
		lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, NULL, NULL, NULL);
		if (lResult != ERROR_SUCCESS) break;
		_snwprintf(szSubKey, MAX_PATH, L"%s\\%s", lpSubKey, szName);
		RegDeleteKeyRecurse(hKeyRoot, szSubKey);
	}

	// Close key
	RegCloseKey(hKey);

	// Try again to delete the key.
	return (RegDeleteKey(hKeyRoot, lpSubKey) == ERROR_SUCCESS) ? TRUE : FALSE;
}

int RegisterShellExtention() {
	WCHAR szExecutablePath[MAX_PATH];
	WCHAR szCommand[MAX_COMMAND];
	WCHAR szIconPath[MAX_COMMAND];
	WCHAR szFileTypeName[] = L"RPG Maker Archive";
	WCHAR szMenuText[] = L"Извлечь файлы";

	try {
		GetModuleFileNameW(GetModuleHandle(nullptr), szExecutablePath, sizeof(szExecutablePath));   // Get executaple path
		_snwprintf(szIconPath, MAX_COMMAND, L"\"%s\",0", szExecutablePath);                         // Get image path

		for (int i = 0; i < fileExtentionsCount; i++) {
			// Get extract command
			_snwprintf(szCommand, MAX_COMMAND, L"\"%s\" --%s --extract \"%%1\"", szExecutablePath, fileExtentions[i][1]);       

			// Create registry keys
			HKEY hKeyRoot, hKeyIcon, hKeyShell, hKeyUnpuck, hKeyCommand;
			RegCreateKeyExW(HKEY_CLASSES_ROOT, fileExtentions[i][0], 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKeyRoot, nullptr);
			RegSetKeyValueW(hKeyRoot, nullptr, nullptr, REG_SZ, szFileTypeName, sizeof(szFileTypeName));
			RegCreateKeyExW(hKeyRoot, L"DefaultIcon", 0, 0, 0, KEY_ALL_ACCESS, nullptr, &hKeyIcon, nullptr);
			RegSetKeyValueW(hKeyIcon, nullptr, nullptr, REG_SZ, szIconPath, sizeof(szIconPath));
			RegCreateKeyExW(hKeyRoot, L"shell", 0, 0, 0, KEY_ALL_ACCESS, nullptr, &hKeyShell, nullptr);
			RegCreateKeyExW(hKeyShell, L"Extract", 0, 0, 0, KEY_ALL_ACCESS, nullptr, &hKeyUnpuck, nullptr);
			RegSetKeyValueW(hKeyUnpuck, nullptr, nullptr, REG_SZ, szMenuText, sizeof(szMenuText));
			RegCreateKeyExW(hKeyUnpuck, L"command", 0, 0, 0, KEY_ALL_ACCESS, nullptr, &hKeyCommand, nullptr);
			RegSetKeyValueW(hKeyCommand, nullptr, nullptr, REG_SZ, szCommand, sizeof(szCommand));
			RegCloseKey(hKeyCommand);
			RegCloseKey(hKeyUnpuck);
			RegCloseKey(hKeyShell);
			RegCloseKey(hKeyIcon);
			RegCloseKey(hKeyRoot);	
		}

		Info(L"Программа добавлена в меню проводника.");
	} catch (DWORD ex) {
		ShowLastError();
		return ex;
	}

	return 0;
}

int UnRegisterShellExtention() {
	try {
		for (int i = 0; i < fileExtentionsCount; i++) {
			RegDeleteKeyRecurse(HKEY_CLASSES_ROOT, fileExtentions[i][0]);
		}
		Info(L"Программа удалена из меню проводника.");
	} catch (DWORD ex) {
		ShowLastError();
		return ex;
	}

	return 0;
}
