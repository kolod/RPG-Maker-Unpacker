#pragma once

#include "stdafx.h"

BOOL IsUserInAdminGroup();
BOOL IsRunAsAdmin();
BOOL IsProcessElevated();
DWORD GetProcessIntegrityLevel();

void Alert(LPWSTR text);
void ShowLastError();

bool FolderExists(LPCWSTR folderName);
void GetDir(LPCWSTR fullPath, LPWSTR dir);
bool CreateDirectoryRecursively(LPCWSTR dir);
