#pragma once

void ToolsPanel_RegisterClass(HINSTANCE hInstance, const wchar_t *className);

typedef struct tagTOOLSWINDATA {
	HWND winHandle;
	HDC hdc;
	HDC backDC;
	RECT rcClientSize;
	HBRUSH bckgndBrush;
	HBRUSH greenPlayBrush;
	HBRUSH buttonFillBrush;
	HBRUSH buttonBckbndBrush;
	HPEN buttonsOutline;
	HBITMAP backBufBitmap;
	int totalButtonsCount;
	RECT rcPlayButtonPos;	// those RECTs store ABSOLUTE buttons coordinate in the parent (Tools) window
	RECT rcPauseButtonPos;	// so can be used to determine whether click was on them or not
	RECT rcStopButtonPos;
} TOOLSWINDATA, *PTOOLSWINDATA;
