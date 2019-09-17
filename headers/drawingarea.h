#pragma once

#define DM_SETINPUTFILE (WM_USER + 0)
#define DM_SETCACHEFILE (WM_USER + 1)
#define DM_SETBUFFER	(WM_USER + 2)
#define DM_SAVEFILE		(WM_USER + 3)
#define DM_SAVEFILEAS 	(WM_USER + 4)

// ADM = Action Drawing Menu
#define ADM_COPY			(WM_USER + 10)
#define ADM_DELETE			(WM_USER + 11)
#define ADM_PASTE			(WM_USER + 12)
#define ADM_SELECTALL		(WM_USER + 13)
#define ADM_MAKESILENT		(WM_USER + 14)
#define ADM_SAVESELECTED	(WM_USER + 15)
#define ADM_REVERSESELECTED	(WM_USER + 16)
#define ADM_ISCHANGED		(WM_USER + 17)

#include "headers\modeldata.h"

typedef struct tagDRAWINWINDATA {
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
