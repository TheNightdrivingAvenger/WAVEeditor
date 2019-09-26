#pragma once
#include "headers\modeldata.h"
#include "headers\player.h"

typedef struct tagMAINWINDOWDATA {
	HWND winHandle;
	HWND drawingAreaHandle;
	HWND toolsWinHandle;
	HANDLE savedFile;
	PMODELDATA modelData;
	PPLAYERDATA player;
} MAINWINDATA, *PMAINWINDATA;

void MainWindow_RegisterClass(HINSTANCE hInstance, const wchar_t *className);
