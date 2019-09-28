#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include <math.h>
#include "headers\drawingarea.h"
#include "headers\mainwindow.h"
#include "headers\constants.h"
#include "headers\sounddrawer.h"

BOOL DrawingArea_DrawNewFile(PDRAWINGWINDATA pSelf, PUPDATEINFO updateInfo)
{
	pSelf->zoomLvl = 1;
	pSelf->soundMetadata = *(updateInfo->wfxFormat);
	pSelf->rgCurDisplayedRange.nFirstSample = 0;
	pSelf->rgCurDisplayedRange.nLastSample = updateInfo->dataSize / pSelf->soundMetadata.nBlockAlign - 1;
	pSelf->lastSample = pSelf->rgCurDisplayedRange.nLastSample;
	BOOL res = recalcMinMax(pSelf, updateInfo->soundData, updateInfo->dataSize, updateInfo->wfxFormat);
	InvalidateRect(pSelf->winHandle, NULL, TRUE);
	return res;
}

// TODO: may consider to jump not straight to the action place, but a bit before it
BOOL DrawingArea_UpdateCache(PDRAWINGWINDATA pSelf, PUPDATEINFO updateInfo, PACTIONINFO aiAction)
{
	pSelf->soundMetadata = *(updateInfo->wfxFormat);
	pSelf->lastSample = updateInfo->dataSize / pSelf->soundMetadata.nBlockAlign - 1;

	sampleIndex curDisplayedRangeLength = pSelf->rgCurDisplayedRange.nLastSample - pSelf->rgCurDisplayedRange.nFirstSample;
	// range where action occured starts outside of visible region, so we display it
	if ((aiAction->rgRange.nFirstSample < pSelf->rgCurDisplayedRange.nFirstSample)
		|| (aiAction->rgRange.nFirstSample > pSelf->rgCurDisplayedRange.nLastSample)) {

		pSelf->rgCurDisplayedRange.nFirstSample = aiAction->rgRange.nFirstSample;
	}
	// choose minimum between total samples count and previous range width
	pSelf->rgCurDisplayedRange.nLastSample = min(pSelf->lastSample,
		pSelf->rgCurDisplayedRange.nFirstSample + curDisplayedRangeLength);

	BOOL res = recalcMinMax(pSelf, updateInfo->soundData, updateInfo->dataSize, updateInfo->wfxFormat);
	InvalidateRect(pSelf->winHandle, NULL, TRUE);
	return res;
}

// probably will be not pos, but sample number, so calculactions and stuff
void DrawingArea_UpdateCursor(PDRAWINGWINDATA pSelf, int pos)
{
	pSelf->rcSelectedRange.left = pos;
	pSelf->rcSelectedRange.right = pos + CURSOR_THICKNESS;
	InvalidateRect(pSelf->winHandle, NULL, TRUE);
}

void DrawingArea_ResetCursor(PDRAWINGWINDATA pSelf)
{
	// TODO: not like this, if zoomed-in it will be in the left side of the screen, but needs to be off-screen (on the first sample)
	if (pSelf->rgCurDisplayedRange.nFirstSample > 0) {
	}
	pSelf->rcSelectedRange.left = 0;
	pSelf->rcSelectedRange.right = CURSOR_THICKNESS;
	InvalidateRect(pSelf->winHandle, NULL, TRUE);
}

unsigned long DrawingArea_CoordsToSample(PDRAWINGWINDATA pSelf, LPARAM clickCoords)
{
	// cache pointer can't be null, because when file changing occurs, cache gets updated with main window message
	// so it's used here as a marker if there's any file currently open
	if (pSelf->minMaxChunksCache != NULL) {
		if (pSelf->samplesInBlock == 1) {
			return min(pSelf->rgCurDisplayedRange.nLastSample, pSelf->rgCurDisplayedRange.nFirstSample
				+ (int)roundf(GET_X_LPARAM(clickCoords) / (float)pSelf->stepX));
		} else {
			return min(pSelf->rgCurDisplayedRange.nLastSample, pSelf->rgCurDisplayedRange.nFirstSample 
				+ pSelf->samplesInBlock * GET_X_LPARAM(clickCoords));
		}
	}
	return 0;
}

void DrawingArea_DrawWave(PDRAWINGWINDATA pSelf)
{
	SaveDC(pSelf->backDC);

	SelectObject(pSelf->backDC, pSelf->borderPen);
	FillRect(pSelf->backDC, &pSelf->rcClientSize, pSelf->bckgndBrush);

	if (pSelf->minMaxChunksCache != NULL) {
		drawRegion(pSelf, &pSelf->soundMetadata);
	}
	RestoreDC(pSelf->backDC, -1);
}

void DrawingArea_DestroyBackBuffer(PDRAWINGWINDATA pSelf)
{
	RestoreDC(pSelf->backDC, -1);
	DeleteObject(pSelf->backBufBitmap);
	DeleteDC(pSelf->backDC);
}

void DrawingArea_RecalcCurDisplayedRange(PDRAWINGWINDATA pSelf)
{
	XCoord usedAreaSizeDelta = pSelf->lastUsedPixelX + SCREEN_DELTA_RIGHT - pSelf->rcClientSize.right;
	BOOL shrinked = FALSE;
	if (usedAreaSizeDelta > 0) {
		shrinked = TRUE;
	} else if (usedAreaSizeDelta < 0) {
		usedAreaSizeDelta = -usedAreaSizeDelta;
		shrinked = FALSE;
	}
	float blocksDelta = usedAreaSizeDelta / (float)pSelf->stepX;
	float intPart;
	sampleIndex blocksDeltaInt;
	if (modff(blocksDelta, &intPart) > 0) {
		blocksDeltaInt = (int)truncf(intPart + 1);
	} else {
		blocksDeltaInt = (int)roundf(intPart);
	}

	if (shrinked) {
		pSelf->rgCurDisplayedRange.nLastSample -= blocksDeltaInt * pSelf->samplesInBlock;
	} else {
		if (pSelf->rgCurDisplayedRange.nLastSample < pSelf->lastSample) {
			pSelf->rgCurDisplayedRange.nLastSample = min(pSelf->lastSample,
				pSelf->rgCurDisplayedRange.nLastSample + blocksDeltaInt * pSelf->samplesInBlock);
		}
	}
}

// if false, prev bitmap was not destroyed, but new wasn't created
BOOL DrawingArea_UpdateBackBuffer(PDRAWINGWINDATA pSelf)
{
	if ((pSelf->backBufBitmap = CreateCompatibleBitmap(pSelf->backDC, pSelf->rcClientSize.right, pSelf->rcClientSize.bottom)) != NULL) {
		HBITMAP prevBitmap = SelectObject(pSelf->backDC, pSelf->backBufBitmap);
		DeleteObject(prevBitmap); // previous bitmap was created by application, so it must be destroyed
		return TRUE;
	}
	return FALSE;
}

BOOL DrawingArea_CreateBackBuffer(PDRAWINGWINDATA pSelf)
{
	HDC temp = GetDC(pSelf->winHandle);
	if ((pSelf->backDC = CreateCompatibleDC(temp)) != NULL) {
		if ((pSelf->backBufBitmap = CreateCompatibleBitmap(temp, pSelf->rcClientSize.right, pSelf->rcClientSize.bottom)) != NULL) {
			ReleaseDC(pSelf->winHandle, temp);
			SaveDC(pSelf->backDC);
			SelectObject(pSelf->backDC, pSelf->backBufBitmap);
			return TRUE;
		}
	}
	ReleaseDC(pSelf->winHandle, temp);
	return FALSE;
}

LRESULT CALLBACK DrawingArea_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res = 0;
	PDRAWINGWINDATA pDrawSelf;
	PAINTSTRUCT ps;

	PSAMPLERANGE rgNewSelectedRange;

	if (uMsg == WM_CREATE) {
		pDrawSelf = (PDRAWINGWINDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DRAWINGWINDATA));
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)pDrawSelf);
		pDrawSelf->winHandle = hWnd;
		pDrawSelf->modelData = ((PCREATEINFO)((LPCREATESTRUCT)lParam)->lpCreateParams)->model;
		MainWindow_AttachDrawingArea(((PCREATEINFO)((LPCREATESTRUCT)lParam)->lpCreateParams)->parent, pDrawSelf);
		pDrawSelf->zoomLvl = 1;

		pDrawSelf->curPen = CreatePen(PS_SOLID, 1, RGB(255,128,0));
		pDrawSelf->borderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		pDrawSelf->bckgndBrush = CreateSolidBrush(RGB(72,72,72));
		pDrawSelf->highlightBrush = CreateSolidBrush(RGB(192,192,192));

		GetClientRect(pDrawSelf->winHandle, &pDrawSelf->rcClientSize);
		pDrawSelf->rcSelectedRange.top = pDrawSelf->rcClientSize.top;
		pDrawSelf->rcSelectedRange.bottom = pDrawSelf->rcClientSize.bottom;
		pDrawSelf->rcSelectedRange.left = 0;
		pDrawSelf->rcSelectedRange.right = pDrawSelf->rcSelectedRange.left + CURSOR_THICKNESS;

		DrawingArea_CreateBackBuffer(pDrawSelf); // ADD CHECKING!!!

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	pDrawSelf = (PDRAWINGWINDATA)GetWindowLong(hWnd, 0);
	if (pDrawSelf == NULL) return DefWindowProc(hWnd, uMsg, wParam, lParam);

	switch (uMsg)
	{
		case WM_LBUTTONDOWN:
			// send message to the model and tell about the selection changes
			rgNewSelectedRange = HeapAlloc(GetProcessHeap(), 0, sizeof(SAMPLERANGE));
			rgNewSelectedRange->nFirstSample = DrawingArea_CoordsToSample(pDrawSelf, lParam);
			rgNewSelectedRange->nLastSample = rgNewSelectedRange->nFirstSample;
			Model_UpdateSelection(pDrawSelf->modelData, FALSE, rgNewSelectedRange);
			HeapFree(GetProcessHeap(), 0, rgNewSelectedRange);

			pDrawSelf->rcSelectedRange.left = GET_X_LPARAM(lParam);
			pDrawSelf->rcSelectedRange.right = pDrawSelf->rcSelectedRange.left + CURSOR_THICKNESS;
			
			InvalidateRect(pDrawSelf->winHandle, NULL, TRUE);
			return 0;
		case WM_RBUTTONDOWN:
			rgNewSelectedRange = HeapAlloc(GetProcessHeap(), 0, sizeof(SAMPLERANGE));
			rgNewSelectedRange->nLastSample = DrawingArea_CoordsToSample(pDrawSelf, lParam);
			Model_UpdateSelection(pDrawSelf->modelData, TRUE, rgNewSelectedRange);
			HeapFree(GetProcessHeap(), 0, rgNewSelectedRange);

			pDrawSelf->rcSelectedRange.right = GET_X_LPARAM(lParam);
			if (pDrawSelf->rcSelectedRange.right < pDrawSelf->rcSelectedRange.left) {
				long cTmp = pDrawSelf->rcSelectedRange.left;
				pDrawSelf->rcSelectedRange.left = GET_X_LPARAM(lParam);
				pDrawSelf->rcSelectedRange.right = cTmp;
			}
			InvalidateRect(pDrawSelf->winHandle, NULL, TRUE);
			return 0;
		case WM_PAINT:
			pDrawSelf->hdc = BeginPaint(pDrawSelf->winHandle, &ps);
			DrawingArea_DrawWave(pDrawSelf);
			BitBlt(pDrawSelf->hdc, 0, 0, pDrawSelf->rcClientSize.right, pDrawSelf->rcClientSize.bottom, pDrawSelf->backDC, 0, 0, SRCCOPY);
			EndPaint(pDrawSelf->winHandle, &ps);
			return 0;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_SIZE:
			GetClientRect(pDrawSelf->winHandle, &(pDrawSelf->rcClientSize));
			if (pDrawSelf->lastSample != 0) DrawingArea_RecalcCurDisplayedRange(pDrawSelf);

			DrawingArea_UpdateBackBuffer(pDrawSelf); // ADD CHECKING!!
			pDrawSelf->rcSelectedRange.top = pDrawSelf->rcClientSize.top;
			pDrawSelf->rcSelectedRange.bottom = pDrawSelf->rcClientSize.bottom;
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		case WM_DESTROY:
			DrawingArea_DestroyBackBuffer(pDrawSelf);
			//delete objects. // TODO: Delete inner objects?
			HeapFree(GetProcessHeap(), 0, pDrawSelf);
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);			
	}
}


void DrawingArea_RegisterClass(HINSTANCE hInstance, const wchar_t *className)
{
	WNDCLASSEXW drawingAreaWinClass = {0};
	drawingAreaWinClass.cbSize = sizeof(drawingAreaWinClass);
	drawingAreaWinClass.style = CS_GLOBALCLASS;
	drawingAreaWinClass.lpszClassName = className;
	drawingAreaWinClass.hInstance = hInstance;
	drawingAreaWinClass.hbrBackground = 0;
	drawingAreaWinClass.lpfnWndProc = DrawingArea_WindowProc;
	drawingAreaWinClass.hCursor = LoadCursor(0, IDC_HAND);
	drawingAreaWinClass.cbWndExtra = sizeof(LONG_PTR);

	RegisterClassExW(&drawingAreaWinClass);
}
