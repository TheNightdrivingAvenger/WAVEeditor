/*ГЛАВНОЕ ОКНО. ПОСЫЛАЕТ ДОЧЕРНЕМУ СООБЩЕНИЯ О ДЕЙСТВИЯХ (смена файла, смена размера)
* НЕПОСРЕДСТВЕННО ОТКРЫВАЕТ ФАЙЛЫ И ПЕРЕДАЁТ ДОЧЕРНЕМУ ОКНУ ИНФОРМАЦИЮ О НИХ;
* ПЕРЕДАЁТ ДОЧЕРНЕМУ ОКНУ ИНФОРМАЦИЮ О ФАЙЛЕ, В КОТОРЫЙ НЕОБХОДИМО СОХРАНИТЬ
*/

#define UNICODE
#define _UNICODE
#include <Windows.h>
#include <math.h>

#include "headers\drawingarea.h"
#include "headers\sounddrawer.h"
#include "headers\mainwindow.h"
#include "headers\mainmenu.h"
#include "headers\fileworker.h"
#include "headers\soundworker.h"
#include "headers\constants.h"

BOOL MainWindow_SendUPDCACHEToView(PMAINWINDATA pSelf);

// returns zero if saved successfully
// -1 -- written less or something
// -2 -- nothing to save
// non-zero LRESULT -- GetLastError() value
LRESULT MainWindow_SaveFile(PMAINWINDATA pSelf, HANDLE hFile, BOOL saveSelected)
{
	if ((pSelf->modelData->curFile == INVALID_HANDLE_VALUE) || (pSelf->modelData->dataSize == 0)
		|| (pSelf->modelData->rgSelectedRange.nFirstSample == pSelf->modelData->rgSelectedRange.nLastSample)) return -2;

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

BOOL checkFormat(PWAVEFORMATEX pWfx)
{
	return (pWfx->wBitsPerSample >= 8 && pWfx->wBitsPerSample <= 16)
		&& (pWfx->nChannels > 0 && pWfx->nChannels <= 2) && (pWfx->nSamplesPerSec > 0 && pWfx->nSamplesPerSec <= 44100) 
		&& (pWfx->nAvgBytesPerSec == (pWfx->nSamplesPerSec * pWfx->nChannels * (pWfx->wBitsPerSample / 8))) 
		&& (pWfx->nBlockAlign == (pWfx->nChannels * (pWfx->wBitsPerSample / 8)));
}

// TODO: looks ugly, maybe I can do something about it
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
int MainWindow_FileChange(PMAINWINDATA pSelf, HANDLE hNewFile)
{
	//working with the file: reading chunks and setting structure's fields
	if (pSelf->modelData->curFile != INVALID_HANDLE_VALUE) {
		CloseHandle(pSelf->modelData->curFile);
		pSelf->modelData->curFile = INVALID_HANDLE_VALUE;
	}

	if (pSelf->modelData->soundData != NULL) {
		HeapFree(GetProcessHeap(), 0, pSelf->modelData->soundData);
		ZeroMemory(&pSelf->modelData->wfxFormat, sizeof(pSelf->modelData->wfxFormat));
		pSelf->modelData->soundData = NULL;
	}

	DWORD readres;
	CHUNK curChunk = getChunk(hNewFile);
	if (strncmp(curChunk.chunkID, "RIFF", CHUNKIDLENGTH) != 0) {
		return 1;
	} else {
		ReadFile(hNewFile, curChunk.chunkID, CHUNKIDLENGTH, &readres, NULL);
		curChunk.chunkID[CHUNKIDLENGTH] = '\0';
		if (strncmp(curChunk.chunkID, "WAVE", CHUNKIDLENGTH) != 0) {
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
				PRANGE newDisplayedRange = HeapAlloc(GetProcessHeap(), 0, sizeof(RANGE));
				newDisplayedRange->nFirstSample = 0;
				newDisplayedRange->nLastSample = pSelf->modelData->dataSize / pSelf->modelData->wfxFormat.nBlockAlign; //same range for every channel
				SendMessage(pSelf->drawingAreaHandle, UPD_DISPLAYEDRANGE, 0, (LPARAM)newDisplayedRange);
				if (!MainWindow_SendUPDCACHEToView(pSelf)) {
					return 7;
				}
				pSelf->modelData->curFile = hNewFile;
				return 0;
			} else {
				HeapFree(GetProcessHeap(), 0, pSelf->modelData->soundData);
				pSelf->modelData = NULL;
				return 6;
			}
		} else {
			return 7;
		}
	} else {
		return 5;
	}
}

HANDLE openFile(wchar_t *fileName)
{
	HANDLE hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	return hFile;
}

wchar_t *openFileExecute(HANDLE parentWindow)
{
	OPENFILENAME ofn = {0};

	static wchar_t fileName[MAX_PATH];

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = parentWindow;
	ofn.lpstrFilter = L"Аудиофайлы windows waveform\0*.wav\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	BOOL result = FALSE;
	result = GetOpenFileName(&ofn);
	if (result)
		return fileName;
	else
		return NULL;
}

void MainWindow_ChangeCurFile(PMAINWINDATA pSelf)
{
	wchar_t *chosenFile = openFileExecute(pSelf->winHandle);
	if (chosenFile) {
		HANDLE hFile;
		if ((hFile = openFile(chosenFile)) != INVALID_HANDLE_VALUE)
		{
			switch (MainWindow_FileChange(pSelf, hFile)) {
				case 0:
					SetWindowText(pSelf->winHandle, chosenFile);
					break;
				case 1:
					MessageBox(pSelf->winHandle, L"Файл не поддерживается либо имеет повреждённую структуру.\nВозможно, он не является .wav файлом.", L"Ошибка", MB_ICONERROR);
					CloseHandle(hFile);
					break;
				case 2:
					MessageBox(pSelf->winHandle, L"Файл не является .wav файлом", L"Ошибка", MB_ICONERROR);
					CloseHandle(hFile);
					break;
				case 3:
					MessageBox(pSelf->winHandle, L"В файле не найден блок, описывающий формат.\nВозможно, файл повреждён или имеет неизвестный формат.", L"Ошибка", MB_ICONERROR);
					CloseHandle(hFile);
					break;
				case 4:
					MessageBox(pSelf->winHandle, L"Данный формат .wav файлов не поддерживается", L"Ошибка", MB_ICONERROR);
					CloseHandle(hFile);
					break;
				case 5:
					MessageBox(pSelf->winHandle, L"В файле не обнаружен блок данных.\nВозможно, файл повреждён или имеет неизвестный формат.", L"Ошибка", MB_ICONERROR);
					CloseHandle(hFile);
					break;
				case 6:
					MessageBox(pSelf->winHandle, L"Произошла неизвестная ошибка при чтении файла.\nУбедитесь, что он существует и не занят другой программой.", L"Ошибка", MB_ICONERROR);
					CloseHandle(hFile);
					break;
				case 7:
					MessageBox(pSelf->winHandle, L"Для открытия файла недостаточно свободной оперативной памяти", L"Ошибка", MB_ICONERROR);
					CloseHandle(hFile);
					break;
			}
		} else {
			MessageBox(pSelf->winHandle, L"Невозможно открыть выбранный файл", L"Ошибка", MB_ICONERROR);
		}
	}
}

wchar_t *saveFileAsExecute(HANDLE parentWindow)
{
	OPENFILENAME sfn = {0};

	static wchar_t savedFileName[MAX_PATH];

	sfn.lStructSize = sizeof(OPENFILENAME);
	sfn.hwndOwner = parentWindow;
	sfn.lpstrFilter = L"Аудиофайлы windows waveform\0*.wav\0";
	sfn.lpstrFile = savedFileName;
	sfn.nMaxFile = MAX_PATH;
	sfn.Flags = OFN_OVERWRITEPROMPT;
	sfn.lpstrDefExt = L"wav";

	BOOL result = FALSE;
	result = GetSaveFileName(&sfn);
	if (result) return savedFileName;
	else return NULL;

}

HANDLE saveFile(wchar_t *fileName)
{
	HANDLE hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
	return hFile;
}

void MainWindow_SaveFileAs(PMAINWINDATA pSelf, BOOL saveSelected)
{
	LRESULT res;
	wchar_t *chosenFile = saveFileAsExecute(pSelf->winHandle);
	if (chosenFile) {
		HANDLE hFile;
		if ((hFile = saveFile(chosenFile)) != INVALID_HANDLE_VALUE) {
			res = MainWindow_SaveFile(pSelf, hFile, saveSelected);
			switch (res) {
				case 0:
					//all good
					if (saveSelected) {
						// close the saved piece and forget about it
						CloseHandle(hFile);
					} else {
						// read the newly-saved file as a new one. TODO: consider changing this
						SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
						if (MainWindow_FileChange(pSelf, hFile) != 0) { // saved file reading error
							MessageBox(pSelf->winHandle, L"Что-то пошло не так... Пожалуйста, перезапустите программу.\nВаша работа была сохранена", L"Ошибка", MB_ICONERROR);
							CloseHandle(hFile);
						} else {
							SetWindowText(pSelf->winHandle, chosenFile);
						}
					}
					break;
				case -2:
					MessageBox(pSelf->winHandle, L"Нечего сохранять", L"Ошибка", MB_ICONERROR);
					CloseHandle(hFile);
					break;
				default:
				//other cases (when GetLastError returned nonzero or written less)
					MessageBox(pSelf->winHandle, L"При записи файла произошла ошибка", L"Ошибка", MB_ICONERROR);
					CloseHandle(hFile);
					DeleteFile(chosenFile);
					break;
			}
		}
	}
}

BOOL MainWindow_SendUPDCACHEToView(PMAINWINDATA pSelf)
{
	PUPDATEINFO updInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(UPDATEINFO));
	updInfo->soundData = pSelf->modelData->soundData;
	updInfo->dataSize = pSelf->modelData->dataSize;
	updInfo->wfxFormat = &(pSelf->modelData->wfxFormat);
	return SendMessage(pSelf->drawingAreaHandle, UPD_CACHE, 0, (LPARAM)updInfo);
}

LRESULT CALLBACK MainWindow_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND drawingAreaHandle;
	HWND toolsPanelHandle;
	PMAINWINDATA pMainSelf;
	RECT newSize;
	LPMINMAXINFO lpMMI;

	const float DRAWINGWINPOSYSCALE = 0.1;

	int modalRes;

	if (uMsg == WM_CREATE) {
		pMainSelf = (PMAINWINDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MAINWINDATA));
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)pMainSelf);
		pMainSelf->winHandle = hWnd;
		GetClientRect(pMainSelf->winHandle, &newSize);

		pMainSelf->modelData = (PMODELDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MODELDATA));
		pMainSelf->modelData->curFile = INVALID_HANDLE_VALUE;
		pMainSelf->modelData->playerState = stopped;
		
		drawingAreaHandle = CreateWindowEx(0, L"DrawingArea", NULL, WS_CHILD | WS_VISIBLE,
			0, (int)truncf(newSize.bottom * DRAWINGWINPOSYSCALE), newSize.right, (int)truncf(newSize.bottom * (1 - DRAWINGWINPOSYSCALE)), hWnd, 0, 0, NULL);
		toolsPanelHandle = CreateWindowEx(0, L"ToolsPanel", NULL, WS_CHILD | WS_VISIBLE,
			0, 0, newSize.right, (int)truncf(newSize.bottom * DRAWINGWINPOSYSCALE), hWnd, 0, 0, NULL);
	
		if (!drawingAreaHandle || !toolsPanelHandle) {
			return -1; // couldn't create drawing area; CreateWindowEx of main window will return NULL handle
		} else {
			UpdateWindow(drawingAreaHandle);
			pMainSelf->drawingAreaHandle = drawingAreaHandle;
			UpdateWindow(toolsPanelHandle);
			pMainSelf->toolsWinHandle = toolsPanelHandle;
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	pMainSelf = (PMAINWINDATA)GetWindowLongPtr(hWnd, 0);
	if (pMainSelf == NULL) return DefWindowProc(hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case AMM_OPENFILE:
					// setup: filling WAVEFORMATEX, buffers and other values
					MainWindow_ChangeCurFile(pMainSelf);
					return 0;
				case AMM_SAVEFILEAS:
					if (pMainSelf->modelData->curFile != INVALID_HANDLE_VALUE) {
						MainWindow_SaveFileAs(pMainSelf, FALSE);
					}
					return 0;
				case AMM_COPY:
					// structure assignment is OK, just a memory copy
					pMainSelf->modelData->rgCopyRange = pMainSelf->modelData->rgSelectedRange;
					break;
				case AMM_DELETE:
					if (pMainSelf->modelData->rgSelectedRange.nFirstSample != pMainSelf->modelData->rgSelectedRange.nLastSample) {
						deletePiece(pMainSelf->modelData);
						pMainSelf->modelData->rgSelectedRange.nFirstSample = pMainSelf->modelData->rgSelectedRange.nLastSample = 0;
						// if we deleted something, then copied piece is no longer valid
						pMainSelf->modelData->rgCopyRange.nFirstSample = pMainSelf->modelData->rgCopyRange.nLastSample = 0;

						MainWindow_SendUPDCACHEToView(pMainSelf);
						SendMessage(pMainSelf->drawingAreaHandle, UPD_CURSOR, 0, 0);
					}
					pMainSelf->modelData->isChanged = TRUE;
					break;
				case AMM_PASTE:
					if (pMainSelf->modelData->rgCopyRange.nFirstSample != pMainSelf->modelData->rgCopyRange.nLastSample) {
						pastePiece(pMainSelf->modelData);
						pMainSelf->modelData->rgSelectedRange.nLastSample = pMainSelf->modelData->rgSelectedRange.nFirstSample = 0;
						// soundworker in pastePiece moves the copied range if needed so it will be valid, no need to zero it
						// TODO: consider to make it more explicit

						MainWindow_SendUPDCACHEToView(pMainSelf);
						SendMessage(pMainSelf->drawingAreaHandle, UPD_CURSOR, 0, 0);
					}
					pMainSelf->modelData->isChanged = TRUE;
					break;
				case AMM_MAKESILENT:
					if (pMainSelf->modelData->rgSelectedRange.nFirstSample != pMainSelf->modelData->rgSelectedRange.nLastSample) {
						makeSilent(pMainSelf->modelData);
						// zero the copy range just for safety
						pMainSelf->modelData->rgCopyRange.nLastSample = pMainSelf->modelData->rgCopyRange.nFirstSample = 0;

						MainWindow_SendUPDCACHEToView(pMainSelf);
					}
					pMainSelf->modelData->isChanged = TRUE;
					break;
				case AMM_SELECTALL:
					if (pMainSelf->modelData->curFile != INVALID_HANDLE_VALUE) {
						pMainSelf->modelData->rgSelectedRange.nFirstSample = 1;
						pMainSelf->modelData->rgSelectedRange.nLastSample = pMainSelf->modelData->dataSize / pMainSelf->modelData->wfxFormat.nBlockAlign;
						
						SendMessage(pMainSelf->drawingAreaHandle, UPD_SELECTION, TRUE, 0);
					}
					break;
				case AMM_SAVESELECTED:
					// TODO: sort this
					MainWindow_SaveFileAs(pMainSelf, TRUE);
					break;
				case AMM_REVERSESELECTED:
					if (pMainSelf->modelData->rgSelectedRange.nFirstSample != pMainSelf->modelData->rgSelectedRange.nLastSample) {
						reversePiece(pMainSelf->modelData);
						
						MainWindow_SendUPDCACHEToView(pMainSelf);
					}
					pMainSelf->modelData->isChanged = TRUE;
					break;
				case AMM_ABOUT:
					MessageBox(pMainSelf->winHandle, L"Аудиоредактор WAVE-файлов.\nЕремеев Г. С., гр. 751002. БГУИР, ФКСиС, кафедра ПОИТ, 2019 г.", L"О программе", MB_ICONINFORMATION);
					break;
			}
			return 0;
		case UPD_SELECTEDRANGE:
			if (pMainSelf->modelData->curFile != INVALID_HANDLE_VALUE) {
				if (!wParam) {
					// only left click has occured, nothing has been selected yet
					pMainSelf->modelData->rgSelectedRange.nFirstSample = pMainSelf->modelData->rgSelectedRange.nLastSample = ((PRANGE)lParam)->nFirstSample;
				} else {
					// right click occured, we have a selected range
					pMainSelf->modelData->rgSelectedRange.nLastSample = ((PRANGE)lParam)->nLastSample;
					if (pMainSelf->modelData->rgSelectedRange.nLastSample < pMainSelf->modelData->rgSelectedRange.nFirstSample) {
						unsigned long tmp = pMainSelf->modelData->rgSelectedRange.nFirstSample;
						pMainSelf->modelData->rgSelectedRange.nFirstSample = pMainSelf->modelData->rgSelectedRange.nLastSample;
						pMainSelf->modelData->rgSelectedRange.nLastSample = tmp;
					}
				}
			}
			HeapFree(GetProcessHeap(), 0, (PRANGE)lParam);
			return 0;
		case WM_GETMINMAXINFO:
			lpMMI = (LPMINMAXINFO)lParam;
    		lpMMI->ptMinTrackSize.x = 600;
    		lpMMI->ptMinTrackSize.y = 400;
			return 0;
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED) {
				GetClientRect(pMainSelf->winHandle, &newSize);
				MoveWindow(pMainSelf->drawingAreaHandle, 0, (int)truncf(newSize.bottom * DRAWINGWINPOSYSCALE), newSize.right, 
					(int)truncf(newSize.bottom * (1 - DRAWINGWINPOSYSCALE)), FALSE);
				MoveWindow(pMainSelf->toolsWinHandle, 0, 0, newSize.right, (int)truncf(newSize.bottom * DRAWINGWINPOSYSCALE), TRUE);

				MainWindow_SendUPDCACHEToView(pMainSelf);
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		case WM_CLOSE:
			if (pMainSelf->modelData->isChanged) {
				modalRes = MessageBoxW(pMainSelf->winHandle, L"Есть несохранённые изменения. Желаете их сохранить?", L"Внимание", MB_YESNOCANCEL);
				if (modalRes == IDYES) {
					MainWindow_SaveFileAs(pMainSelf, FALSE);
				} else if (modalRes == IDCANCEL) {
					return 0;
				}
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);

		case WM_DESTROY:
			PostQuitMessage(0);
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

void MainWindow_RegisterClass(HINSTANCE hInstance, const wchar_t *className)
{
	WNDCLASSEXW mainWinClass = {0};

	mainWinClass.cbSize = sizeof(mainWinClass);
	mainWinClass.style = CS_VREDRAW | CS_HREDRAW;
	mainWinClass.lpszClassName = className;
	mainWinClass.hInstance = hInstance;
	mainWinClass.lpszMenuName = MAKEINTRESOURCE(MAINMENU);
	mainWinClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	mainWinClass.lpfnWndProc = MainWindow_WindowProc;
	mainWinClass.hCursor = LoadCursor(0, IDC_ARROW);
	mainWinClass.cbWndExtra = sizeof(LONG_PTR); // extra space for struct with window data

	RegisterClassEx(&mainWinClass);
}
