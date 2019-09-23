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
#include "headers\fileworker.h"
#include "headers\soundworker.h"

BOOL DrawingArea_UpdateCache(PDRAWINGWINDATA pSelf)
{
	pSelf->modelData->rgCurRange.nLastSample = pSelf->modelData->dataSize / pSelf->modelData->wfxFormat.nBlockAlign;
	BOOL res = recalcMinMax(pSelf);
	InvalidateRect(pSelf->winHandle, NULL, TRUE);
	return res;
}

BOOL checkFormat(PWAVEFORMATEX pWfx)
{
	return (pWfx->wBitsPerSample >= 8 && pWfx->wBitsPerSample <= 16)
		&& (pWfx->nChannels > 0 && pWfx->nChannels <= 2) && (pWfx->nSamplesPerSec > 0 && pWfx->nSamplesPerSec <= 44100) 
		&& (pWfx->nAvgBytesPerSec == (pWfx->nSamplesPerSec * pWfx->nChannels * (pWfx->wBitsPerSample / 8))) 
		&& (pWfx->nBlockAlign == (pWfx->nChannels * (pWfx->wBitsPerSample / 8)));
}

/*
* Return codes: 0 OK
*				1 not RIFF
* 				2 not WAVE
*				3 fmt not found
*				4 not supported format
*				5 data not found
*				6 failed reading from file
*				7 not enough memory
*/
LRESULT DrawingArea_FileChange(PDRAWINGWINDATA pSelf, HANDLE hNewFile)
{
	//working with the file: reading chunks and settings structure's fields
	if (pSelf->modelData->curFile != INVALID_HANDLE_VALUE) {
		CloseHandle(pSelf->modelData->curFile);
		pSelf->modelData->curFile = INVALID_HANDLE_VALUE;
	}

	BOOL heapRes;
	if (pSelf->modelData->soundData != NULL) {
		heapRes = HeapFree(GetProcessHeap(), 0, pSelf->modelData->soundData);
		memset(&pSelf->modelData->wfxFormat, 0, sizeof(pSelf->modelData->wfxFormat));
		pSelf->modelData->soundData = NULL;
	}

	DWORD readres;
	CHUNK curChunk = getChunk(hNewFile);
	if (strncmp(curChunk.chunkID, "RIFF", CHUNKIDLENGTH) != 0) {
		return 1;
	} else {
		ReadFile(hNewFile, curChunk.chunkID, CHUNKIDLENGTH, &readres, NULL);
		curChunk.chunkID[CHUNKIDLENGTH] = '\0';
		if (strncmp(curChunk.chunkID, "WAVE", 4) != 0) {
			return 2;
		}
	}

	BOOL chunkNotFound;
	curChunk.chunkSize = 0;
	do {
		SetFilePointer(hNewFile, curChunk.chunkSize, NULL, FILE_CURRENT);
		curChunk = getChunk(hNewFile);
		chunkNotFound = (curChunk.chunkID[0] == '\0');
	} while (!chunkNotFound && (strncmp(curChunk.chunkID, "fmt ", CHUNKIDLENGTH) != 0));

	if (!chunkNotFound) {
		ReadFile(hNewFile, &(pSelf->modelData->wfxFormat), sizeof(pSelf->modelData->wfxFormat) - sizeof(WORD), &readres, NULL);
		
		if (checkFormat(&pSelf->modelData->wfxFormat)) {
			if (curChunk.chunkSize > sizeof(WAVEFORMATEX)) {
				return 4; // extensible WAV
			}
		} else {
			return 4;
		}
	} else {
		return 3;
	}

	curChunk.chunkSize = 0;
	do {
		SetFilePointer(hNewFile, curChunk.chunkSize, NULL, FILE_CURRENT);
		curChunk = getChunk(hNewFile);
		chunkNotFound = (curChunk.chunkID[0] == '\0');
	} while (!chunkNotFound && (strncmp(curChunk.chunkID, "data", CHUNKIDLENGTH) != 0));

	if (!chunkNotFound) {
		if ((pSelf->modelData->soundData = HeapAlloc(GetProcessHeap(), 0, curChunk.chunkSize)) != NULL) {		
			pSelf->modelData->dataSize = curChunk.chunkSize;
			if (ReadFile(hNewFile, pSelf->modelData->soundData, curChunk.chunkSize, &readres, NULL)) {
				pSelf->modelData->rgCurRange.nFirstSample = 0;
				pSelf->modelData->rgCurRange.nLastSample = pSelf->modelData->dataSize / pSelf->modelData->wfxFormat.nBlockAlign; //same range for every channel
				//pSelf->modelData->rgCurRange.nLastSample = 3600;

				pSelf->rcSelectedRange.left = 0;
				pSelf->rcSelectedRange.right = 0;
				if (!DrawingArea_UpdateCache(pSelf)) {
					return 7;
				}
				pSelf->modelData->curFile = hNewFile;
				return 0;
			} else {
				HeapFree(GetProcessHeap(), 0, pSelf->modelData->soundData);
				return 6;
			}
		} else {
			return 7;
		}
	} else {
		return 5;
	}
}

// returns zero if saved successfully
LRESULT DrawingArea_SaveFile(PDRAWINGWINDATA pSelf, HANDLE hFile, BOOL saveSelected)
{
	if (pSelf->modelData->curFile == INVALID_HANDLE_VALUE || pSelf->modelData->dataSize == 0) return -2;

	DWORD writeRes;
	struct writingDataStart {
		char riffTag[CHUNKIDLENGTH];
		unsigned long fileLen;
		char waveTag[CHUNKIDLENGTH];
		char fmtTag[CHUNKIDLENGTH];
		unsigned long fmtLen;
	} dataStart;
	struct writingDataEnd {
		char dataTag[CHUNKIDLENGTH];
		unsigned long dataLen;
	} dataEnd;
	memcpy(&dataStart.riffTag, "RIFF", CHUNKIDLENGTH);
	dataStart.fileLen = 4 + 8 + pSelf->modelData->dataSize + 8 + sizeof(pSelf->modelData->wfxFormat) - sizeof(WORD);
	memcpy(&dataStart.waveTag, "WAVE", CHUNKIDLENGTH);
	memcpy(&dataStart.fmtTag, "fmt ", CHUNKIDLENGTH);
	dataStart.fmtLen = sizeof(pSelf->modelData->wfxFormat) - sizeof(WORD);
	
	memcpy(&dataEnd.dataTag, "data", CHUNKIDLENGTH);

	unsigned long amountToWrite, bufferStart;
	if (saveSelected) {
		amountToWrite = (pSelf->modelData->rgSelectedRange.nLastSample - pSelf->modelData->rgSelectedRange.nFirstSample + 1) 
																							* pSelf->modelData->wfxFormat.nBlockAlign;
		bufferStart = (pSelf->modelData->rgSelectedRange.nFirstSample - 1) * pSelf->modelData->wfxFormat.nBlockAlign;
	} else {
		amountToWrite = pSelf->modelData->dataSize;
		bufferStart = 0;
	}
	dataEnd.dataLen = amountToWrite;

	if (WriteFile(hFile, &dataStart, sizeof(struct writingDataStart), &writeRes, NULL)) {
		DWORD chunkSize = sizeof(pSelf->modelData->wfxFormat) - sizeof(WORD);
		if (writeRes == sizeof(struct writingDataStart) && WriteFile(hFile, &pSelf->modelData->wfxFormat, chunkSize, &writeRes, NULL)) {
			if (writeRes == chunkSize && WriteFile(hFile, &dataEnd, sizeof(struct writingDataEnd), &writeRes, NULL)) {
				if (writeRes == sizeof(struct writingDataEnd) 			// NOT DATA SIZE
						&& WriteFile(hFile, (char *)pSelf->modelData->soundData + bufferStart, amountToWrite, &writeRes, NULL)) {
					if (writeRes == amountToWrite) {
						return 0; //write OK
					}
				}
			}
		}
	}

	DWORD err;
	if ((err = GetLastError()) != 0) { //writing error
		return (LRESULT)err;
	} else {
		return -1; // no writing error, but probably written less than should be
	}
}

void DrawingArea_MakeSilent(PDRAWINGWINDATA pSelf)
{
	if (pSelf->modelData->rgSelectedRange.nFirstSample != pSelf->modelData->rgSelectedRange.nLastSample)
	{
		makeSilent(pSelf->modelData);
		pSelf->modelData->rgCopyRange.nFirstSample = pSelf->modelData->rgCopyRange.nLastSample = 0;
		DrawingArea_UpdateCache(pSelf);
	}
}

void DrawingArea_PastePiece(PDRAWINGWINDATA pSelf)
{
	if (pSelf->modelData->rgCopyRange.nFirstSample != pSelf->modelData->rgCopyRange.nLastSample) {
		pastePiece(pSelf->modelData);
		pSelf->modelData->rgSelectedRange.nLastSample = pSelf->modelData->rgSelectedRange.nFirstSample;
		pSelf->rcSelectedRange.right = pSelf->rcSelectedRange.left + 2;
		DrawingArea_UpdateCache(pSelf);
	}
}

void DrawingArea_DeletePiece(PDRAWINGWINDATA pSelf)
{
	if (pSelf->modelData->rgSelectedRange.nFirstSample != pSelf->modelData->rgSelectedRange.nLastSample) {
		deletePiece(pSelf->modelData);
		pSelf->modelData->rgCopyRange.nFirstSample = pSelf->modelData->rgCopyRange.nLastSample = 0;
		pSelf->rcSelectedRange.left = 0;
		pSelf->rcSelectedRange.right = pSelf->rcSelectedRange.left + 2;
		DrawingArea_UpdateCache(pSelf);
	}
}

void DrawingArea_ReverseSelected(PDRAWINGWINDATA pSelf)
{
	if (pSelf->modelData->rgSelectedRange.nFirstSample != pSelf->modelData->rgSelectedRange.nLastSample) {
		reversePiece(pSelf->modelData);
		DrawingArea_UpdateCache(pSelf);
	}
}

unsigned long DrawingArea_CoordsToSample(PDRAWINGWINDATA pSelf, LPARAM clickCoords)
{
	if (pSelf->modelData->curFile != INVALID_HANDLE_VALUE) {
		if (pSelf->modelData->samplesInBlock == 1) {
			return min(pSelf->modelData->rgCurRange.nLastSample, pSelf->modelData->rgCurRange.nFirstSample
				+ (int)roundf(LOWORD(clickCoords) / (float)pSelf->stepX));
		} else {
			return min(pSelf->modelData->rgCurRange.nLastSample, pSelf->modelData->rgCurRange.nFirstSample 
				+ pSelf->modelData->samplesInBlock * LOWORD(clickCoords));
		}
	}
	return 0;
}

void DrawingArea_DrawWave(PDRAWINGWINDATA pSelf)
{
	SaveDC(pSelf->backDC);

	SelectObject(pSelf->backDC, pSelf->borderPen);
	FillRect(pSelf->backDC, &pSelf->rcClientSize, pSelf->bckgndBrush);

	if (pSelf->modelData->curFile != INVALID_HANDLE_VALUE && pSelf->modelData->minMaxChunksCache != NULL) {
		drawRegion(pSelf);
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

	if (uMsg == WM_CREATE) {
		pDrawSelf = (PDRAWINGWINDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DRAWINGWINDATA));
		pDrawSelf->modelData = (PMODELDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MODELDATA));
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)pDrawSelf);
		pDrawSelf->winHandle = hWnd;
		pDrawSelf->modelData->curFile = INVALID_HANDLE_VALUE;
		pDrawSelf->modelData->psPlayerState = stopped;

		pDrawSelf->curPen = CreatePen(PS_SOLID, 1, RGB(255,128,0));
		pDrawSelf->borderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		pDrawSelf->bckgndBrush = CreateSolidBrush(RGB(72,72,72));
		pDrawSelf->highlightBrush = CreateSolidBrush(RGB(192,192,192));

		GetClientRect(pDrawSelf->winHandle, &pDrawSelf->rcClientSize);
		pDrawSelf->rcSelectedRange.top = pDrawSelf->rcClientSize.top;
		pDrawSelf->rcSelectedRange.bottom = pDrawSelf->rcClientSize.bottom;

		DrawingArea_CreateBackBuffer(pDrawSelf); // ADD CHECKING!!!

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	pDrawSelf = (PDRAWINGWINDATA)GetWindowLong(hWnd, 0);
	if (pDrawSelf == NULL) return DefWindowProc(hWnd, uMsg, wParam, lParam);

	switch (uMsg)
	{
		// setup: filling WAVEFORMATEX, buffers and other values
		case SM_SETINPUTFILE:
			res = DrawingArea_FileChange(pDrawSelf, (HANDLE)wParam);
			pDrawSelf->modelData->isChanged = FALSE;
			return res;
		case SM_SAVEFILEAS:
			res = DrawingArea_SaveFile(pDrawSelf, (HANDLE)wParam, FALSE);
			pDrawSelf->modelData->isChanged = FALSE;
			return res;
		case MAF_MAKESILENT:
			DrawingArea_MakeSilent(pDrawSelf);
			pDrawSelf->modelData->isChanged = TRUE;
			return 0;

		case MAF_DELETE:
			DrawingArea_DeletePiece(pDrawSelf);
			pDrawSelf->modelData->isChanged = TRUE;
			return 0;
		case MAF_COPY:
			pDrawSelf->modelData->rgCopyRange = pDrawSelf->modelData->rgSelectedRange;
			return 0;

		// return error if not enough memory or something
		case MAF_PASTE:
			DrawingArea_PastePiece(pDrawSelf);
			pDrawSelf->modelData->isChanged = TRUE;
			return 0;

		case MAF_SELECTALL:
			if (pDrawSelf->modelData->curFile != INVALID_HANDLE_VALUE) {
				pDrawSelf->modelData->rgSelectedRange.nFirstSample = 1;
				pDrawSelf->modelData->rgSelectedRange.nLastSample = pDrawSelf->modelData->dataSize / pDrawSelf->modelData->wfxFormat.nBlockAlign;
				pDrawSelf->rcSelectedRange.left = 0;
				pDrawSelf->rcSelectedRange.right = pDrawSelf->rcClientSize.right - 1;
				InvalidateRect(pDrawSelf->winHandle, NULL, TRUE);
			}
			return 0;

		case MAF_SAVESELECTED:
			if (pDrawSelf->modelData->rgSelectedRange.nFirstSample != pDrawSelf->modelData->rgSelectedRange.nLastSample) {
				res = DrawingArea_SaveFile(pDrawSelf, (HANDLE)wParam, TRUE);
				return res;				
			} else {
				return -2;
			}

		case MAF_REVERSESELECTED:
			DrawingArea_ReverseSelected(pDrawSelf);
			pDrawSelf->modelData->isChanged = TRUE;
			return 0;

		case MAF_ISCHANGED:
			return pDrawSelf->modelData->isChanged;

		case WM_LBUTTONDOWN:
			SetCapture(pDrawSelf->winHandle);
			pDrawSelf->modelData->rgSelectedRange.nFirstSample = DrawingArea_CoordsToSample(pDrawSelf, lParam);
			pDrawSelf->modelData->rgSelectedRange.nLastSample = pDrawSelf->modelData->rgSelectedRange.nFirstSample;
			pDrawSelf->rcSelectedRange.left = GET_X_LPARAM(lParam);
			pDrawSelf->rcSelectedRange.right = pDrawSelf->rcSelectedRange.left + 2;
			
			InvalidateRect(pDrawSelf->winHandle, NULL, TRUE);
			ReleaseCapture();
			return 0;
		case WM_RBUTTONDOWN:
			pDrawSelf->modelData->rgSelectedRange.nLastSample = DrawingArea_CoordsToSample(pDrawSelf, lParam);
			pDrawSelf->rcSelectedRange.right = GET_X_LPARAM(lParam);
			if (pDrawSelf->modelData->rgSelectedRange.nLastSample < pDrawSelf->modelData->rgSelectedRange.nFirstSample) {
				unsigned long tmp = pDrawSelf->modelData->rgSelectedRange.nFirstSample;
				pDrawSelf->modelData->rgSelectedRange.nFirstSample = pDrawSelf->modelData->rgSelectedRange.nLastSample;
				pDrawSelf->modelData->rgSelectedRange.nLastSample = tmp;
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
			if (pDrawSelf->modelData->curFile != INVALID_HANDLE_VALUE) DrawingArea_UpdateCache(pDrawSelf);
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		case WM_DESTROY:
			// check isChanged //TOOD: what?
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
