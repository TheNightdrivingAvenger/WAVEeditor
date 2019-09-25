/*ДОЧЕРНЕЕ ОКНО ДЛЯ РАБОТЫ СО ЗВУКОМ. 
* ЧИТАЕТ ИЗ ФАЙЛА (ПРОВЕРЯЕТ КОРРЕКТНОСТЬ).
* ПЕРЕСЧИТЫВАЕТ (вызывает процедуру из модуля soundworker.h для работы со звуком) ВСЕ ДИАПАЗОНЫ (видимый, выделенный).
* РИСУЕТ (вызывает процедуры из модуля для рисования).
* СОХРЯНЕТ ФАЙЛЫ.
* ВЫЗЫВАЕТ ПРОЦЕДУРЫ ДЛЯ РАБОТЫ С ФАЙЛОМ (редактирование).
*/

#define UNICODE
#define _UNICODE
#include <Windows.h>
#include <windowsx.h>
#include <string.h>
#include <math.h>
#include "headers\drawingarea.h"
#include "headers\constants.h"
#include "headers\sounddrawer.h"

BOOL DrawingArea_UpdateCache(PDRAWINGWINDATA pSelf, PUPDATEINFO updateInfo)
{
	// TODO: updating the cache means there has been some changes in the sound data. But what I do here is just set view from the
	// first to the last sample. Not good for planned scaling
	pSelf->rgCurDisplayedRange.nLastSample = updateInfo->dataSize / updateInfo->wfxFormat->nBlockAlign;
	pSelf->soundMetadata = *(updateInfo->wfxFormat);
	BOOL res = recalcMinMax(pSelf, updateInfo->soundData, updateInfo->dataSize, updateInfo->wfxFormat);
	return res;
}

unsigned long DrawingArea_CoordsToSample(PDRAWINGWINDATA pSelf, LPARAM clickCoords)
{
	// cache pointer can't be null, because when file changing occurs, cache gets updated with main window message
	// so it's used here as a marker if there's any file currently open
	if (pSelf->minMaxChunksCache != NULL) {
		if (pSelf->samplesInBlock == 1) {
			return min(pSelf->rgCurDisplayedRange.nLastSample, pSelf->rgCurDisplayedRange.nFirstSample
				+ (int)roundf(LOWORD(clickCoords) / (float)pSelf->stepX));
		} else {
			return min(pSelf->rgCurDisplayedRange.nLastSample, pSelf->rgCurDisplayedRange.nFirstSample 
				+ pSelf->samplesInBlock * LOWORD(clickCoords));
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

	PRANGE rgNewSelectedRange;

	if (uMsg == WM_CREATE) {
		pDrawSelf = (PDRAWINGWINDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DRAWINGWINDATA));
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)pDrawSelf);
		pDrawSelf->winHandle = hWnd;
		pDrawSelf->parentWindow = ((LPCREATESTRUCT)lParam)->hwndParent;

		pDrawSelf->curPen = CreatePen(PS_SOLID, 1, RGB(255,128,0));
		pDrawSelf->borderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		pDrawSelf->bckgndBrush = CreateSolidBrush(RGB(72,72,72));
		pDrawSelf->highlightBrush = CreateSolidBrush(RGB(192,192,192));

		GetClientRect(pDrawSelf->winHandle, &pDrawSelf->rcClientSize);
		pDrawSelf->rcSelectedRange.top = pDrawSelf->rcClientSize.top;
		pDrawSelf->rcSelectedRange.bottom = pDrawSelf->rcClientSize.bottom;
		pDrawSelf->rcSelectedRange.left = 0;
		pDrawSelf->rcSelectedRange.right = pDrawSelf->rcSelectedRange.left + 2;

		DrawingArea_CreateBackBuffer(pDrawSelf); // ADD CHECKING!!!

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	pDrawSelf = (PDRAWINGWINDATA)GetWindowLong(hWnd, 0);
	if (pDrawSelf == NULL) return DefWindowProc(hWnd, uMsg, wParam, lParam);

	switch (uMsg)
	{
		case UPD_CURSOR:
			pDrawSelf->rcSelectedRange.left = lParam;
			pDrawSelf->rcSelectedRange.right = pDrawSelf->rcSelectedRange.left + 2;
			InvalidateRect(pDrawSelf->winHandle, NULL, TRUE);
			return 0;
		case UPD_SELECTION:
			if (wParam) {
				pDrawSelf->rcSelectedRange.left = 0;
				pDrawSelf->rcSelectedRange.right = pDrawSelf->rcClientSize.right - 1;
			} else {
				pDrawSelf->rcSelectedRange.left = GET_X_LPARAM(lParam);
				pDrawSelf->rcSelectedRange.right = GET_Y_LPARAM(lParam);
			}
			InvalidateRect(pDrawSelf->winHandle, NULL, TRUE);
			return 0;
		case UPD_CACHE:
			// TRUE if everything update OK, FALSE otherwise
			res = DrawingArea_UpdateCache(pDrawSelf, (PUPDATEINFO)lParam);
			HeapFree(GetProcessHeap(), 0, (PUPDATEINFO)lParam);
			InvalidateRect(pDrawSelf->winHandle, NULL, TRUE);
			return !res;
		case UPD_DISPLAYEDRANGE:
			pDrawSelf->rgCurDisplayedRange.nFirstSample = ((PRANGE)lParam)->nFirstSample;
			pDrawSelf->rgCurDisplayedRange.nLastSample = ((PRANGE)lParam)->nLastSample;
			HeapFree(GetProcessHeap(), 0, (PRANGE)lParam);
			return 0;
		case WM_LBUTTONDOWN:
			// send message to main window and tell about the selection changes
			// Send and not Post because model must be updated immediately
			rgNewSelectedRange = HeapAlloc(GetProcessHeap(), 0, sizeof(RANGE));
			rgNewSelectedRange->nFirstSample = DrawingArea_CoordsToSample(pDrawSelf, lParam);
			rgNewSelectedRange->nLastSample = rgNewSelectedRange->nFirstSample;
			SendMessage(pDrawSelf->parentWindow, UPD_SELECTEDRANGE, FALSE, (LPARAM)rgNewSelectedRange);

			pDrawSelf->rcSelectedRange.left = GET_X_LPARAM(lParam);
			pDrawSelf->rcSelectedRange.right = pDrawSelf->rcSelectedRange.left + 2;
			
			InvalidateRect(pDrawSelf->winHandle, NULL, TRUE);
			return 0;
		case WM_RBUTTONDOWN:
			rgNewSelectedRange = HeapAlloc(GetProcessHeap(), 0, sizeof(RANGE));
			rgNewSelectedRange->nLastSample = DrawingArea_CoordsToSample(pDrawSelf, lParam);
			SendMessage(pDrawSelf->parentWindow, UPD_SELECTEDRANGE, TRUE, (LPARAM)rgNewSelectedRange);

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
