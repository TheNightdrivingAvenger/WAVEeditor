#pragma once
#include "headers\modeldata.h"

typedef struct tagDRAWINGWINDATA {
	HWND winHandle;
	HDC hdc;
	HDC backDC;
	HBITMAP backBufBitmap;
	HPEN curPen;
	HPEN borderPen;
	HBRUSH highlightBrush;
	HBRUSH bckgndBrush;
	PMODELDATA modelData;
	RECT rcClientSize;
	RECT rcSelectedRange;
	int stepX;					// how many pixels in one step
} DRAWINGWINDATA, *PDRAWINGWINDATA;

void DrawingArea_RegisterClass(HINSTANCE hInstance, const wchar_t *className);
