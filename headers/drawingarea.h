#pragma once
#include "headers\modeldata.h"

#define CURSOR_THICKNESS 1
#define SCREEN_DELTA_RIGHT 0	// how many pixels exclue on the right side from space for drawing

#define MAX_STEPX 40

typedef LONG XCoord;
typedef struct tagCOORDRANGE {
	XCoord xStart;
	XCoord xEnd;
} COORDRANGE, *PCOORDRANGE;

struct tagMODELDATA;

typedef struct tagDRAWINGWINDATA {
	HWND winHandle;
	struct tagMODELDATA *modelData;
	HDC hdc;
	HDC backDC;
	HBITMAP backBufBitmap;
	HPEN curPen;
	HPEN borderPen;
	HBRUSH highlightBrush;
	HBRUSH bckgndBrush;
	void *minMaxChunksCache;	// stores in int; [ [min][max]1st channel ... [min][max]nth channel ][ [min][max]1st channel ... [min][max]nth channel ]
	long cacheLength;	// length of the cache. min-max pairs (for every channel) = 1 length unit (1 block). The last block can contain less samples than others
	int samplesInBlock;			// number of samples used to calculate one cache block for one channel
	WAVEFORMATEX soundMetadata;	// stored to not disturb model on every redrawing, like a cache
	SAMPLERANGE rgCurDisplayedRange;
	sampleIndex lastSample;		// last sample number in sound data (for one channel)
	//int blocksInScrollStep;
	RECT rcClientSize;
	RECT rcSelectedRange;
	LONG lastUsedPixelX;
	LONG stepX;					// how many pixels in one step
} DRAWINGWINDATA, *PDRAWINGWINDATA;

typedef struct tagUPDATEINFO {	// struct for holding a "cache update" message's info. Should be allocated in the heap by caller. Memory is freed by the message receiver
	void *soundData;
	int dataSize;
	PWAVEFORMATEX wfxFormat;
	//SAMPLERANGE workingSet;
} UPDATEINFO, *PUPDATEINFO;

BOOL DrawingArea_UpdateCache(PDRAWINGWINDATA pSelf, PUPDATEINFO updateInfo, PACTIONINFO aiAction);
BOOL DrawingArea_DrawNewFile(PDRAWINGWINDATA pSelf, PUPDATEINFO updateInfo);
void DrawingArea_SetNewRange(PDRAWINGWINDATA pSelf, PACTIONINFO aiAction);
void DrawingArea_UpdateSelection(PDRAWINGWINDATA pSelf, PSAMPLERANGE newRange);
void DrawingArea_ZoomIn(PDRAWINGWINDATA pSelf, PUPDATEINFO updateInfo);
void DrawingArea_ZoomOut(PDRAWINGWINDATA pSelf, PUPDATEINFO updateInfo);
void DrawingArea_RegisterClass(HINSTANCE hInstance, const wchar_t *className);
