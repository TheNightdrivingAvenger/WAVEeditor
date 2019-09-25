#pragma once

void ToolsPanel_RegisterClass(HINSTANCE hInstance, const wchar_t *className);

#include "headers\modeldata.h"
#include "headers\buttoninfo.h"
#include "headers\list.h"

typedef struct tagTOOLSWINDATA {
	HWND winHandle;
	HDC hdc;
	HDC backDC;
	RECT rcClientSize;
	PMODELDATA modelData;
	HBRUSH bckgndBrush;
	HBRUSH buttonBckgndBrush;
	HBITMAP backBufBitmap;
	int totalButtonsCount;
	PLISTDATA buttonsContainer;	// memory is freed by the list when it's destroyed
} TOOLSWINDATA, *PTOOLSWINDATA;
