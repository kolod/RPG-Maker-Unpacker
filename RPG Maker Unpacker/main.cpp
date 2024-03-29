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
#include "getopt.h"
#include "shell.h"
#include "resource.h"
#include "RpgMakerReader.h"
#include "RenPyReader.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // Current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // The main window class name
WCHAR szRegister[MAX_LOADSTRING];               // The register button text
WCHAR szUnRegister[MAX_LOADSTRING];             // The unregister button text
BOOL fIsElevated = true;                        //
HWND hWnd;                                      // The main window handle
HWND hProgressBar;                              // The progress bar

static const struct option_w longOptions[] {
	{L"rpgmaker", no_argument, nullptr, L'0'},
	{L"renpy", no_argument, nullptr, L'1'},
	{L"register", no_argument, nullptr, L'r'},
	{L"unregister", no_argument, nullptr, L'u'},
	{L"extract", required_argument, nullptr, L'e'},
	{L"help", no_argument, nullptr, L'h'}
};

enum class Engine {
	None,
	RpgMaker,
	RenPy
};

enum class Action {
	None,
	Register,
	UnRegister,
	Extract,
	Help
};

typedef struct {
	Action action;
	Engine engine;
	LPWSTR path;
} Options;

// Launch itself as administrator.
void Elevate(LPCWCHAR command) {
	if (!IsRunAsAdmin()) {
		wchar_t szPath[MAX_PATH];
		if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath))) {
			SHELLEXECUTEINFOW sei = {sizeof(sei)};
			sei.lpVerb = L"runas";
			sei.lpFile = szPath;
			sei.hwnd = hWnd;
			sei.nShow = SW_NORMAL;
			sei.lpParameters = command;

			if (!ShellExecuteExW(&sei)) {
				DWORD dwError = GetLastError();
				if (dwError != ERROR_CANCELLED) {
					ShowLastError();
				}
			} 
		}
	}
}

LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_COMMAND:
		if (lParam != 0) switch (wParam) {
		case ID_REGISTER:
			if (fIsElevated) {
				RegisterShellExtention();
			} else {
				Elevate(L"--register");
			}
			break;

		case ID_UNREGISTER:
			if (fIsElevated) {
				UnRegisterShellExtention();
			} else {
				Elevate(L"--unregister");
			}
		}
		break;

	case WM_DESTROY:
		PostQuitMessage (0);
		break;

	default:
		return DefWindowProc (hWnd, message, wParam, lParam);
	}

	return 0;
}

ATOM MyRegisterClass (HINSTANCE hInstance) {
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof (WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon (hInstance, MAKEINTRESOURCE (IDI_RPGMAKERUNPACKER));
	wcex.hCursor = LoadCursor (nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = nullptr;
	return RegisterClassExW (&wcex);
}

int CreateMainWindow() {
	// Register main window class
	MyRegisterClass(hInst);

	// Calculate position and size of main window
	int width = 310;
	int height = 75;
	RECT rect;
	GetClientRect(GetDesktopWindow(), &rect);
	rect.left = rect.right / 2 - width / 2;
	rect.right = rect.left + width;
	rect.top = rect.bottom / 2 - height / 2;
	rect.bottom = rect.top + height;
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	hWnd = CreateWindowExW(
		0,
		szWindowClass,
		szTitle,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		rect.left, rect.top, width, height,
		nullptr, nullptr, hInst, nullptr);
	if (!hWnd) return -1;

	HWND hRegisterButton = CreateWindowExW(
		0,
		L"BUTTON",
		szRegister,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		5, 5, 300, 30, hWnd, (HMENU) ID_REGISTER, hInst, nullptr
	);

	HWND hUnRegisterButton = CreateWindowExW(
		0,
		L"BUTTON",
		szUnRegister,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		5, 40, 300, 30, hWnd, (HMENU) ID_UNREGISTER, hInst, nullptr
	);

	// Display the process elevation information.
	if (IsWindowsVistaOrGreater()) {
		Button_SetElevationRequiredState(hRegisterButton, !fIsElevated);
		Button_SetElevationRequiredState(hUnRegisterButton, !fIsElevated);
	}

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}

void UpdateProgress(int64_t readed, int64_t all) {
	SendMessage(hProgressBar, PBM_SETPOS, (WPARAM) readed * 1000 / all, 0);

	MSG msg;
	while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

int CreateProgressWindow(Options options) {

	// Register main window class
	MyRegisterClass(hInst);

	// Calculate position and size of main window
	int width = 310;
	int height = 40;
	RECT rect;
	GetClientRect(GetDesktopWindow(), &rect);
	rect.left = rect.right / 2 - width / 2;
	rect.right = rect.left + width;
	rect.top = rect.bottom / 2 - height / 2;
	rect.bottom = rect.top + height;
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	hWnd = CreateWindowExW(
		0,
		szWindowClass,
		szTitle,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		rect.left, rect.top, width, height,
		nullptr, nullptr, hInst, nullptr);
	if (!hWnd) return -1;

	hProgressBar = CreateWindowExW(
		0,
		PROGRESS_CLASS,
		NULL,
		WS_VISIBLE | WS_CHILD,
		5, 5, 300, 30, hWnd, (HMENU) ID_PROGRESS, hInst, nullptr
	);

	SendMessage(hProgressBar, PBM_SETRANGE, 0, (LPARAM) MAKELONG(0, 1000));
	SendMessage(hProgressBar, PBM_SETPOS, (WPARAM) 0, 0);
	UpdateWindow(hProgressBar);

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	
	Reader *reader = nullptr;
	switch (options.engine) {
	case Engine::None:
		return 1;

	case Engine::RpgMaker:
		reader = new RpgMakerReader(options.path, &UpdateProgress);
		break;

	case Engine::RenPy:
		reader = new RenPyReader(options.path, &UpdateProgress);
		break;
	}

	reader->Extract();
	delete reader;

	return 0;
}

int main(int argc, wchar_t *argv[]) {

	// Get the process elevation information.
	if (IsWindowsVistaOrGreater()) {
		fIsElevated = IsProcessElevated();
	}

	// Initialize global strings
	LoadStringW(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInst, IDS_WINDOW_CLASS, szWindowClass, MAX_LOADSTRING);
	LoadStringW(hInst, IDS_REGISTER, szRegister, MAX_LOADSTRING);
	LoadStringW(hInst, IDS_UNREGISTER, szUnRegister, MAX_LOADSTRING);

	INITCOMMONCONTROLSEX initControl;
	initControl.dwSize = sizeof(INITCOMMONCONTROLSEX);
	initControl.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&initControl);

	if (argc <= 1) return CreateMainWindow();

	int index = 0;
	Options options = {Action::None, Engine::None};

	for (;;) {
		int opt = getopt_long_w(argc, argv, L"rue:h", longOptions, &index);

		if (opt == 'r') {
			options.action = Action::Register;
			break;
		}

		else if (opt == 'u') {
			options.action = Action::UnRegister;
			break;
		}

		else if (opt == 'h') {
			options.action = Action::Help;
			break;
		}

		else if (opt == 'e') {
			options.action = Action::Extract;
			options.path = optarg_w;
		}

		else if (opt == L'0') {
			options.engine = Engine::RpgMaker;
		}

		else if (opt == L'1') {
			options.engine = Engine::RenPy;
		}

		else {
			break;
		}
	}

	switch (options.action) {
	case Action::Register:   return RegisterShellExtention();
	case Action::UnRegister: return UnRegisterShellExtention();
	case Action::Extract:    return CreateProgressWindow(options);

	default:
		break;
	}

	return 0;
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	UNREFERENCED_PARAMETER(nCmdShow);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(pCmdLine);

	hInst = hInstance;

	return main(__argc, __wargv);
}
