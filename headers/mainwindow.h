#pragma once

typedef struct tagMAINWINDOWDATA {
	HWND winHandle;
	HWND drawingAreaHandle;
	HWND toolsWinHandle;
	HANDLE savedFile;
} MAINWINDATA, *PMAINWINDATA;

void MainWindow_RegisterClass(HINSTANCE hInstance, const wchar_t *className);
