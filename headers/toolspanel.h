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
	PBUTTONINFO buttons; // TODO: obsolete?
	PLISTNODE buttonsContainer;
	/*RECT rcPlayButtonPos;	// those RECTs store ABSOLUTE buttons coordinate in the parent (Tools) window
	RECT rcPauseButtonPos;	// so can be used to determine whether click was on them or not
	RECT rcStopButtonPos;*/
} TOOLSWINDATA, *PTOOLSWINDATA;

typedef int(*ClickHandler)(void);
