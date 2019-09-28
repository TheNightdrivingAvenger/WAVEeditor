#pragma once

#include "headers\buttoninfo.h"
#include "headers\list.h"

struct tagMODELDATA;

typedef struct tagTOOLSWINDATA {
	HWND winHandle;
	struct tagMODELDATA *modelData;
	HDC hdc;
	HDC backDC;
	RECT rcClientSize;
	HBRUSH bckgndBrush;
	HBRUSH buttonBckgndBrush;
	HBITMAP backBufBitmap;
	int totalButtonsCount;
	PLISTDATA buttonsContainer;	// memory is freed by the list when it's destroyed
} TOOLSWINDATA, *PTOOLSWINDATA;

void ToolsPanel_RegisterClass(HINSTANCE hInstance, const wchar_t *className);
