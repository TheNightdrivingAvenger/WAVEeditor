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

void DrawingArea_DrawWave(PDRAWINGWINDATA pSelf);

BOOL DrawingArea_DrawNewFile(PDRAWINGWINDATA pSelf, PUPDATEINFO updateInfo)
{
	pSelf->soundMetadata = *(updateInfo->wfxFormat);
	pSelf->rgCurDisplayedRange.nFirstSample = 0;
	pSelf->rgCurDisplayedRange.nLastSample = 10;//updateInfo->dataSize / pSelf->soundMetadata.nBlockAlign - 1;

	pSelf->lastSample = updateInfo->dataSize / pSelf->soundMetadata.nBlockAlign - 1;//pSelf->rgCurDisplayedRange.nLastSample;

	BOOL res = recalcMinMax(pSelf, updateInfo->soundData, updateInfo->dataSize, updateInfo->wfxFormat, fitInWindow);
	DrawingArea_DrawWave(pSelf);

	SCROLLINFO scrollInfo = (SCROLLINFO){ sizeof(SCROLLINFO), SIF_ALL | SIF_DISABLENOSCROLL,
										0, pSelf->cacheLength, pSelf->cacheLength, 0};
	SetScrollInfo(pSelf->winHandle, SB_HORZ, &scrollInfo, TRUE);
	InvalidateRect(pSelf->winHandle, NULL, TRUE);
	//pSelf->blocksInScrollStep = 1;
	return res;
}

void DrawingArea_SampleRangeToSelection(PDRAWINGWINDATA pSelf, PSAMPLERANGE range)
{
	if (pSelf->minMaxChunksCache != NULL) {
		sampleIndex minEnd = min(range->nLastSample, pSelf->rgCurDisplayedRange.nLastSample);
		sampleIndex maxStart = max(range->nFirstSample, pSelf->rgCurDisplayedRange.nFirstSample);
		sampleIndex intersectedSegment = minEnd - maxStart;

		if (intersectedSegment >= 0) {
			pSelf->rcSelectedRange.left = maxStart / pSelf->samplesInBlock * pSelf->stepX -
				pSelf->rgCurDisplayedRange.nFirstSample / pSelf->samplesInBlock * pSelf->stepX;
			if (range->nFirstSample == range->nLastSample) {
				pSelf->rcSelectedRange.right = pSelf->rcSelectedRange.left + CURSOR_THICKNESS;
			} else {
				pSelf->rcSelectedRange.right = minEnd / pSelf->samplesInBlock * pSelf->stepX -
					pSelf->rgCurDisplayedRange.nFirstSample / pSelf->samplesInBlock * pSelf->stepX;
			}
		} else {
			pSelf->rcSelectedRange.left = pSelf->rcSelectedRange.right = -1;
		}
	}
}

// TODO: may consider to jump not straight to the action place, but a bit before it
BOOL DrawingArea_UpdateCache(PDRAWINGWINDATA pSelf, PUPDATEINFO updateInfo, PACTIONINFO aiAction)
{
	pSelf->soundMetadata = *(updateInfo->wfxFormat);
	pSelf->lastSample = max(0, updateInfo->dataSize / pSelf->soundMetadata.nBlockAlign - 1);

	sampleIndex curDisplayedRangeLength = pSelf->rgCurDisplayedRange.nLastSample - pSelf->rgCurDisplayedRange.nFirstSample + 1;
	// range where action occured starts outside of visible region, so we jump to it
	if ((aiAction->rgRange.nFirstSample < pSelf->rgCurDisplayedRange.nFirstSample)
		|| (aiAction->rgRange.nFirstSample > pSelf->rgCurDisplayedRange.nLastSample)) {

		sampleIndex samplesDelta = pSelf->lastSample - aiAction->rgRange.nFirstSample + 1;
		if (samplesDelta <= curDisplayedRangeLength - 1) {
			pSelf->rgCurDisplayedRange.nFirstSample = max(0, pSelf->lastSample - curDisplayedRangeLength + 1);
		} else {
			pSelf->rgCurDisplayedRange.nFirstSample = aiAction->rgRange.nFirstSample;
		}
	}
	// choose minimum between total samples count and previous range width
	pSelf->rgCurDisplayedRange.nLastSample = min(pSelf->lastSample,
		pSelf->rgCurDisplayedRange.nFirstSample + curDisplayedRangeLength);

	BOOL res = recalcMinMax(pSelf, updateInfo->soundData, updateInfo->dataSize, updateInfo->wfxFormat, zoomNone);
	DrawingArea_DrawWave(pSelf);
	InvalidateRect(pSelf->winHandle, NULL, TRUE);

	SCROLLINFO scrollInfo = (SCROLLINFO){ sizeof(SCROLLINFO), SIF_ALL | SIF_DISABLENOSCROLL,
										0, pSelf->cacheLength, 
										pSelf->rgCurDisplayedRange.nLastSample / pSelf->samplesInBlock -
											pSelf->rgCurDisplayedRange.nFirstSample / pSelf->samplesInBlock,
										pSelf->rgCurDisplayedRange.nFirstSample / pSelf->samplesInBlock, 0};
	// TODO: this triple-division is not good, consider saving another range (in blocks) ^^^
	SetScrollInfo(pSelf->winHandle, SB_HORZ, &scrollInfo, TRUE);
	//pSelf->blocksInScrollStep = 1;
	return res;
}

void DrawingArea_SetNewRange(PDRAWINGWINDATA pSelf, PACTIONINFO aiAction)
{
	pSelf->rgCurDisplayedRange.nFirstSample = aiAction->rgRange.nFirstSample;
	pSelf->rgCurDisplayedRange.nLastSample = aiAction->rgRange.nLastSample;
	SCROLLINFO scrollInfo = (SCROLLINFO){ sizeof(SCROLLINFO), SIF_POS | SIF_DISABLENOSCROLL,
										0, 0, 0,
										pSelf->rgCurDisplayedRange.nFirstSample / pSelf->samplesInBlock, 0};
	// TODO: this division is not good, consider saving another range (in blocks) ^^^
	SetScrollInfo(pSelf->winHandle, SB_HORZ, &scrollInfo, TRUE);
	DrawingArea_DrawWave(pSelf);
	// A bit laggy, can use update window or leave it as it is (it's OK)
	InvalidateRect(pSelf->winHandle, NULL, TRUE);
}

void DrawingArea_UpdateSelection(PDRAWINGWINDATA pSelf, PSAMPLERANGE newRange)
{
	DrawingArea_SampleRangeToSelection(pSelf, newRange);
	DrawingArea_DrawWave(pSelf);
	InvalidateRect(pSelf->winHandle, NULL, TRUE);
}

sampleIndex DrawingArea_CoordsToSample(PDRAWINGWINDATA pSelf, LPARAM clickCoords)
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
	if ((modff(blocksDelta, &intPart) > 0) && shrinked) {
		blocksDeltaInt = (int)truncf(intPart + 1);
	} else {
		blocksDeltaInt = (int)truncf(intPart);
	}

	if (shrinked) {
		pSelf->rgCurDisplayedRange.nLastSample -= blocksDeltaInt * pSelf->samplesInBlock;
	} else {
		pSelf->rgCurDisplayedRange.nLastSample = min(pSelf->lastSample,
			pSelf->rgCurDisplayedRange.nLastSample + blocksDeltaInt * pSelf->samplesInBlock);
	}
}

// Zooms in ~2x times
void DrawingArea_ZoomIn(PDRAWINGWINDATA pSelf, PUPDATEINFO updateInfo)
{
	if (pSelf->stepX <= MAX_STEPX) {
		sampleIndex totalSamples = pSelf->rgCurDisplayedRange.nLastSample - pSelf->rgCurDisplayedRange.nFirstSample;
		sampleIndex zoomDelta = totalSamples / 4;
		// TODO: fix
		pSelf->rgCurDisplayedRange.nFirstSample = max(0, pSelf->rgCurDisplayedRange.nFirstSample + zoomDelta);
		pSelf->rgCurDisplayedRange.nLastSample = min(pSelf->rgCurDisplayedRange.nLastSample - zoomDelta, pSelf->lastSample);
		recalcMinMax(pSelf, updateInfo->soundData, updateInfo->dataSize, updateInfo->wfxFormat, zoomIn);
		DrawingArea_DrawWave(pSelf);
		InvalidateRect(pSelf->winHandle, NULL, TRUE);
	}
}

// Zooms out ~2x times
void DrawingArea_ZoomOut(PDRAWINGWINDATA pSelf, PUPDATEINFO updateInfo)
{
	sampleIndex totalSamples = pSelf->rgCurDisplayedRange.nLastSample - pSelf->rgCurDisplayedRange.nFirstSample;
	sampleIndex zoomDelta = totalSamples / 4;
	pSelf->rgCurDisplayedRange.nFirstSample = max(0, pSelf->rgCurDisplayedRange.nFirstSample - zoomDelta);
	pSelf->rgCurDisplayedRange.nLastSample = min(pSelf->rgCurDisplayedRange.nLastSample + zoomDelta, pSelf->lastSample);
	recalcMinMax(pSelf, updateInfo->soundData, updateInfo->dataSize, updateInfo->wfxFormat, zoomOut);
	DrawingArea_DrawWave(pSelf);
	InvalidateRect(pSelf->winHandle, NULL, TRUE);
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
	SCROLLINFO scrollInfo;

	PSAMPLERANGE rgNewSelectedRange;

	if (uMsg == WM_CREATE) {
		pDrawSelf = (PDRAWINGWINDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DRAWINGWINDATA));
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)pDrawSelf);
		pDrawSelf->winHandle = hWnd;
		pDrawSelf->modelData = ((PCREATEINFO)((LPCREATESTRUCT)lParam)->lpCreateParams)->model;
		MainWindow_AttachDrawingArea(((PCREATEINFO)((LPCREATESTRUCT)lParam)->lpCreateParams)->parent, pDrawSelf);

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
		DrawingArea_DrawWave(pDrawSelf);

		scrollInfo.cbSize = sizeof(SCROLLINFO);
		scrollInfo.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
		scrollInfo.nMax = 0;
		scrollInfo.nMin = 0;
		scrollInfo.nPage = 0;
		scrollInfo.nPos = 0;
		SetScrollInfo(pDrawSelf->winHandle, SB_HORZ, &scrollInfo, TRUE);

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

			//pDrawSelf->rcSelectedRange.left = GET_X_LPARAM(lParam);
			//pDrawSelf->rcSelectedRange.right = pDrawSelf->rcSelectedRange.left + CURSOR_THICKNESS;
			
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
		case WM_HSCROLL:
			ZeroMemory(&scrollInfo, sizeof(SCROLLINFO));
			scrollInfo.cbSize = sizeof(SCROLLINFO);
			scrollInfo.fMask = SIF_ALL;
			GetScrollInfo(pDrawSelf->winHandle, SB_HORZ, &scrollInfo);
			switch (LOWORD(wParam)) {
			// User clicked the HOME keyboard key.
				case SB_TOP:
					scrollInfo.nPos = scrollInfo.nMin;
					break;
				// User clicked the END keyboard key.
				case SB_BOTTOM:
					scrollInfo.nPos = scrollInfo.nMax;
					break;
				// User clicked the top arrow.
				case SB_LINEUP:
					scrollInfo.nPos = max(scrollInfo.nMin, scrollInfo.nPos - 1);
					break;
				// User clicked the bottom arrow.
				case SB_LINEDOWN:
					scrollInfo.nPos = min(scrollInfo.nMax, scrollInfo.nPos + 1);
					break;
				// User clicked the scroll bar shaft above the scroll box.
				case SB_PAGEUP:
				// explicit int conversion because nPage is UINT, so nPos would be converted to UINT too
					scrollInfo.nPos = max(scrollInfo.nMin, scrollInfo.nPos - (int)scrollInfo.nPage);
					break;
				// User clicked the scroll bar shaft below the scroll box.
				case SB_PAGEDOWN:
					scrollInfo.nPos = min(scrollInfo.nMax - (scrollInfo.nPage - 1), scrollInfo.nPos + scrollInfo.nPage);
					break;
				// User dragged the scroll box.
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					scrollInfo.nPos = scrollInfo.nTrackPos;
					break;
			}
			sampleIndex curDisplayedRangeLength = pDrawSelf->rgCurDisplayedRange.nLastSample - pDrawSelf->rgCurDisplayedRange.nFirstSample + 1;
			// TODO: consider using separate SAMPLERANGE variable
			pDrawSelf->rgCurDisplayedRange.nFirstSample = scrollInfo.nPos * pDrawSelf->samplesInBlock;// * pDrawSelf->blocksInScrollStep;
			pDrawSelf->rgCurDisplayedRange.nLastSample = min(pDrawSelf->lastSample,
				pDrawSelf->rgCurDisplayedRange.nFirstSample + curDisplayedRangeLength - 1);

			Model_UpdateActiveRange(pDrawSelf->modelData, &pDrawSelf->rgCurDisplayedRange);
			return 0;
		case WM_PAINT:
			pDrawSelf->hdc = BeginPaint(pDrawSelf->winHandle, &ps);
			BitBlt(pDrawSelf->hdc, 0, 0, pDrawSelf->rcClientSize.right, pDrawSelf->rcClientSize.bottom, pDrawSelf->backDC, 0, 0, SRCCOPY);
			EndPaint(pDrawSelf->winHandle, &ps);
			return 0;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_SIZE:
			pDrawSelf->rcClientSize.bottom = HIWORD(lParam) + 1;
			pDrawSelf->rcClientSize.right = LOWORD(lParam) + 1;
			if (pDrawSelf->lastSample != 0) {
				DrawingArea_RecalcCurDisplayedRange(pDrawSelf);
				scrollInfo = (SCROLLINFO){ sizeof(SCROLLINFO), SIF_ALL | SIF_DISABLENOSCROLL,
										0, pDrawSelf->cacheLength, 
										pDrawSelf->rgCurDisplayedRange.nLastSample / pDrawSelf->samplesInBlock -
											pDrawSelf->rgCurDisplayedRange.nFirstSample / pDrawSelf->samplesInBlock,
										pDrawSelf->rgCurDisplayedRange.nFirstSample / pDrawSelf->samplesInBlock, 0};
				// TODO: this triple-division is not good, consider saving another range (in blocks) ^^^
				SetScrollInfo(pDrawSelf->winHandle, SB_HORZ, &scrollInfo, TRUE);
			}

			DrawingArea_UpdateBackBuffer(pDrawSelf); // ADD CHECKING!!
			DrawingArea_DrawWave(pDrawSelf);
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
