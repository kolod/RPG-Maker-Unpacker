// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define STATIC_GETOPT
#define _CRT_SECURE_NO_WARNINGS

// Windows Header Files:
#include <windows.h>
#include <CommCtrl.h>
#include <VersionHelpers.h>
#include <Shellapi.h>

// C RunTime Header Files
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>

// User headers
#include "utils.h"
