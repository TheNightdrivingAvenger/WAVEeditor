#pragma once
#include "headers\modeldata.h"

typedef struct tagDRAWINGWINDATA {
	HWND parentWindow;
	HWND winHandle;
	HDC hdc;
	HDC backDC;
	HBITMAP backBufBitmap;
	HPEN curPen;
	HPEN borderPen;
	HBRUSH highlightBrush;
	HBRUSH bckgndBrush;
	void *minMaxChunksCache;	// stores in int; [ [min][max]1st channel ... [min][max]nth channel ][ [min][max]1st channel ... [min][max]nth channel ]
	unsigned long cacheLength;	// length of the cache. min-max pairs (for every channel) = 1 length unit
	int samplesInBlock;			// number of samples used to calculate one cache block for one channel
	RANGE rgCurDisplayedRange;
	RECT rcClientSize;
	RECT rcSelectedRange;
	int stepX;					// how many pixels in one step
} DRAWINGWINDATA, *PDRAWINGWINDATA;

typedef struct tagUPDATEINFO {	// struct for holding a "cache update" message's info. Should be allocated in the heap by caller. Memory is freed by the message receiver
	void *soundData;
	int dataSize;
	PWAVEFORMATEX wfxFormat;
} UPDATEINFO, *PUPDATEINFO;

void DrawingArea_RegisterClass(HINSTANCE hInstance, const wchar_t *className);
