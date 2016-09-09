#include "stdafx.h"
#include "getopt.h"
#include "shell.h"
#include "resource.h"
#include "RpgMakerReader.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // Current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // The main window class name
WCHAR szRegister[MAX_LOADSTRING];               // The register button text
WCHAR szUnRegister[MAX_LOADSTRING];             // The unregister button text
BOOL fIsElevated = true;                        //
HWND hWnd;                                      // The main window handle

static const struct option_w longOptions[] {
	{L"register", no_argument, nullptr, L'r'},
	{L"unregister", no_argument, nullptr, L'u'},
	{L"extract", required_argument, nullptr, L'e'},
	{L"help", no_argument, nullptr, L'h'}
};

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
	initControl.dwICC = ICC_STANDARD_CLASSES;
	InitCommonControlsEx(&initControl);
	
	int index = 0;
	int opt = getopt_long_w(argc, argv, L"rue:h", longOptions, &index);

	if (opt == 'r') {
		RegisterShellExtention();
		return 0;
	}

	if (opt == 'u') {
		UnRegisterShellExtention();
		return 0;
	}

	if (opt == 'h') {
		return 0;
	}

	if (opt == 'e') {
		RpgMakerReader *reader = new RpgMakerReader(optarg_w);
		reader->Open();
		reader->Extract();
		return 0;
	}

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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	UNREFERENCED_PARAMETER(nCmdShow);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(pCmdLine);

	hInst = hInstance;

	return main(__argc, __wargv);
}
