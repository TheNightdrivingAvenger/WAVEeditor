#define _UNICODE
#define UNICODE
#include <windows.h>
#include "headers\constants.h"
#include "headers\modeldata.h"
#include "headers\fileworker.h"
#include "headers\soundworker.h"
#include "headers\mainwindow.h"
#include "headers\drawingarea.h"
#include "headers\mainwindow.h"

BOOL checkFormat(PWAVEFORMATEX pWfx)
{
	return (pWfx->wBitsPerSample >= 8 && pWfx->wBitsPerSample <= 16)
		&& (pWfx->nChannels > 0 && pWfx->nChannels <= 2) && (pWfx->nSamplesPerSec > 0 && pWfx->nSamplesPerSec <= 44100) 
		&& (pWfx->nAvgBytesPerSec == (pWfx->nSamplesPerSec * pWfx->nChannels * (pWfx->wBitsPerSample / 8))) 
		&& (pWfx->nBlockAlign == (pWfx->nChannels * (pWfx->wBitsPerSample / 8)));
}

void Model_AttachView(PMODELDATA pSelf, struct tagMAINWINDOWDATA *view)
{
	pSelf->mainView = view;
}

void Model_Reset(PMODELDATA pSelf)
{
	if (pSelf->curFile != INVALID_HANDLE_VALUE) {
		CloseHandle(pSelf->curFile);
		pSelf->curFile = INVALID_HANDLE_VALUE;
	}
	if (pSelf->soundData != NULL) {
		HeapFree(GetProcessHeap(), 0, pSelf->soundData);
		pSelf->soundData = NULL;
	}
	pSelf->dataSize = 0;
	pSelf->isChanged = FALSE;
	ZeroMemory(&pSelf->wfxFormat, sizeof(pSelf->wfxFormat));
	ZeroMemory(&pSelf->rgCopyRange, sizeof(SAMPLERANGE));
	ZeroMemory(&pSelf->rgSelectedRange, sizeof(SAMPLERANGE));
	pSelf->soundData = NULL;
}

// TODO: looks ugly, maybe I can do something about it
// !TODO!: don't forget to set defaults
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
int Model_FileChange(PMODELDATA pSelf, HANDLE hNewFile)
{
	Model_Reset(pSelf);
	//working with the file: reading chunks and setting structure's fields
	pSelf->curFile = hNewFile;
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
		ReadFile(hNewFile, &(pSelf->wfxFormat), sizeof(pSelf->wfxFormat) - sizeof(WORD), &readres, NULL);
		
		if (checkFormat(&pSelf->wfxFormat)) {
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
		if ((pSelf->soundData = HeapAlloc(GetProcessHeap(), 0, curChunk.chunkSize)) != NULL) {
			pSelf->dataSize = curChunk.chunkSize;
			if (ReadFile(hNewFile, pSelf->soundData, curChunk.chunkSize, &readres, NULL)) {
				MainWindow_UpdateView(pSelf->mainView, pSelf, newFileOpened | selectionChange, NULL); // r: 
				return 0;
			} else {
				return 6;
			}
		} else {
			return 7;
		}
	} else {
		return 5;
	}
}

HANDLE openFile(const wchar_t *const fileName)
{
	HANDLE hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	return hFile;
}

void Model_ChangeCurFile(PMODELDATA pSelf, const wchar_t *const chosenFile)
{
	MainWindow_UpdateView(pSelf->mainView, pSelf, playbackStop, NULL);
	HANDLE hFile;
	if ((hFile = openFile(chosenFile)) != INVALID_HANDLE_VALUE) {
		switch (Model_FileChange(pSelf, hFile)) {
			case 0:
				// TODO: consider making a separate method in the View
				MainWindow_UpdateView(pSelf->mainView, pSelf, curFileNameChange, NULL);
				break;
			case 1:
				MainWindow_ShowModalError(pSelf->mainView, L"Файл не поддерживается либо имеет повреждённую структуру.\nВозможно, он не является .wav файлом.", L"Ошибка", MB_ICONERROR);
				Model_Reset(pSelf);
				break;
			case 2:
				MainWindow_ShowModalError(pSelf->mainView, L"Файл не является .wav файлом", L"Ошибка", MB_ICONERROR);
				Model_Reset(pSelf);
				break;
			case 3:
				MainWindow_ShowModalError(pSelf->mainView, L"В файле не найден блок, описывающий формат.\nВозможно, файл повреждён или имеет неизвестный формат.", L"Ошибка", MB_ICONERROR);
				Model_Reset(pSelf);
				break;
			case 4:
				MainWindow_ShowModalError(pSelf->mainView, L"Данный формат .wav файлов не поддерживается", L"Ошибка", MB_ICONERROR);
				Model_Reset(pSelf);
				break;
			case 5:
				MainWindow_ShowModalError(pSelf->mainView, L"В файле не обнаружен блок данных.\nВозможно, файл повреждён или имеет неизвестный формат.", L"Ошибка", MB_ICONERROR);
				Model_Reset(pSelf);
				break;
			case 6:
				MainWindow_ShowModalError(pSelf->mainView, L"Произошла неизвестная ошибка при чтении файла.\nУбедитесь, что он существует и не занят другой программой.", L"Ошибка", MB_ICONERROR);
				Model_Reset(pSelf);
				break;
			case 7:
				MainWindow_ShowModalError(pSelf->mainView, L"Для открытия файла недостаточно свободной оперативной памяти", L"Ошибка", MB_ICONERROR);
				Model_Reset(pSelf);
				break;
		}
	} else {
		MainWindow_ShowModalError(pSelf->mainView, L"Невозможно открыть выбранный файл", L"Ошибка", MB_ICONERROR);
	}
	MainWindow_UpdateView(pSelf->mainView, pSelf, selectionChange, NULL);
}

// returns zero if saved successfully
// -1 -- written less or something
// -2 -- nothing to save
// non-zero LRESULT -- GetLastError() value
LRESULT Model_SaveFile(PMODELDATA pSelf, HANDLE hFile, BOOL saveSelected)
{
	if ((pSelf->dataSize == 0)
		|| ((saveSelected) && (pSelf->rgSelectedRange.nFirstSample == pSelf->rgSelectedRange.nLastSample))
		|| (pSelf->curFile == INVALID_HANDLE_VALUE)) return -2;

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
	dataStart.fileLen = 4 + 8 + pSelf->dataSize + 8 + sizeof(pSelf->wfxFormat) - sizeof(WORD);
	memcpy(&dataStart.waveTag, "WAVE", CHUNKIDLENGTH);
	memcpy(&dataStart.fmtTag, "fmt ", CHUNKIDLENGTH);
	dataStart.fmtLen = sizeof(pSelf->wfxFormat) - sizeof(WORD);
	
	memcpy(&dataEnd.dataTag, "data", CHUNKIDLENGTH);

	unsigned long amountToWrite, bufferStart;
	if (saveSelected) {
		amountToWrite = (pSelf->rgSelectedRange.nLastSample - pSelf->rgSelectedRange.nFirstSample + 1) 
																							* pSelf->wfxFormat.nBlockAlign;
		bufferStart = (pSelf->rgSelectedRange.nFirstSample - 1) * pSelf->wfxFormat.nBlockAlign;
	} else {
		amountToWrite = pSelf->dataSize;
		bufferStart = 0;
	}
	dataEnd.dataLen = amountToWrite;

	if (WriteFile(hFile, &dataStart, sizeof(struct writingDataStart), &writeRes, NULL)) {
		DWORD chunkSize = sizeof(pSelf->wfxFormat) - sizeof(WORD);
		if (writeRes == sizeof(struct writingDataStart) && WriteFile(hFile, &pSelf->wfxFormat, chunkSize, &writeRes, NULL)) {
			if (writeRes == chunkSize && WriteFile(hFile, &dataEnd, sizeof(struct writingDataEnd), &writeRes, NULL)) {
				if (writeRes == sizeof(struct writingDataEnd) // NOT DATA SIZE
						&& WriteFile(hFile, (char *)pSelf->soundData + bufferStart, amountToWrite, &writeRes, NULL)) {
					if (writeRes == amountToWrite) {
						return 0; //written OK
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

HANDLE saveFile(wchar_t *fileName)
{
	HANDLE hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
	return hFile;
}

void Model_SaveFileAs(PMODELDATA pSelf, BOOL saveSelected, wchar_t *const chosenFile)
{
	MainWindow_UpdateView(pSelf->mainView, pSelf, playbackStop, NULL);
	LRESULT res;
	HANDLE hFile;
	if ((hFile = saveFile(chosenFile)) != INVALID_HANDLE_VALUE) {
		res = Model_SaveFile(pSelf, hFile, saveSelected);
		switch (res) {
		case 0:
			//all good
			if (saveSelected) {
				// close the saved piece and forget about it
				CloseHandle(hFile);
			} else {
				// read the newly-saved file as a new one. TODO: consider changing this
				SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
				if (Model_FileChange(pSelf, hFile) != 0) {	// saved file reading error
					MainWindow_ShowModalError(pSelf->mainView, L"Что-то пошло не так... Пожалуйста, перезапустите программу.\nВаша работа была сохранена", L"Ошибка", MB_ICONERROR);
					Model_Reset(pSelf);
				} else {
					MainWindow_UpdateView(pSelf->mainView, pSelf, curFileNameChange, NULL);
					pSelf->isChanged = FALSE;
				}
			}
			break;
		case -2:
			MainWindow_ShowModalError(pSelf->mainView, L"Нечего сохранять", L"Ошибка", MB_ICONERROR);
			CloseHandle(hFile);
			DeleteFile(chosenFile);
			break;
		default:
			//other cases (when GetLastError returned nonzero or written less)
			MainWindow_ShowModalError(pSelf->mainView, L"При записи файла произошла ошибка", L"Ошибка", MB_ICONERROR);
			CloseHandle(hFile);
			DeleteFile(chosenFile);
			break;
		}
	}
}

void Model_UpdateActiveRange(PMODELDATA pSelf, PSAMPLERANGE newRange)
{
	PACTIONINFO lastAction = HeapAlloc(GetProcessHeap(), 0, sizeof(ACTIONINFO));
	lastAction->action = eaPaste;
	lastAction->rgRange.nFirstSample = newRange->nFirstSample;
	lastAction->rgRange.nLastSample = newRange->nLastSample;
	MainWindow_UpdateView(pSelf->mainView, pSelf, activeRangeChange, lastAction);
	HeapFree(GetProcessHeap(), 0, lastAction);
}

void Model_CopySelected(PMODELDATA pSelf)
{
	// structure assignment is OK, just a memory copy
	pSelf->rgCopyRange = pSelf->rgSelectedRange;
}

// lastAction: first sample -- first selected sample, last sample -- last selected sample
void Model_DeletePiece(PMODELDATA pSelf)
{
	if (pSelf->rgSelectedRange.nFirstSample != pSelf->rgSelectedRange.nLastSample) {
		MainWindow_UpdateView(pSelf->mainView, pSelf, playbackStop, NULL);

		deletePiece(pSelf);

		PACTIONINFO lastAction = HeapAlloc(GetProcessHeap(), 0, sizeof(ACTIONINFO));
		lastAction->action = eaDelete;
		lastAction->rgRange = pSelf->rgSelectedRange;

		ZeroMemory(&pSelf->rgSelectedRange, sizeof(SAMPLERANGE));
		// if we deleted something, then copied piece is no longer valid
		ZeroMemory(&pSelf->rgCopyRange, sizeof(SAMPLERANGE));

		MainWindow_UpdateView(pSelf->mainView, pSelf, soundDataChange | selectionChange, lastAction); //r: sound data change
		HeapFree(GetProcessHeap(), 0, lastAction);
	}
	pSelf->isChanged = TRUE;
}

// lastAction: first sample -- paste position, last sample -- piece length
void Model_PastePiece(PMODELDATA pSelf)
{
	if (pSelf->rgCopyRange.nFirstSample != pSelf->rgCopyRange.nLastSample) {
		MainWindow_UpdateView(pSelf->mainView, pSelf, playbackStop, NULL);

		pastePiece(pSelf);

		PACTIONINFO lastAction = HeapAlloc(GetProcessHeap(), 0, sizeof(ACTIONINFO));
		lastAction->action = eaPaste;
		lastAction->rgRange.nFirstSample = pSelf->rgSelectedRange.nFirstSample;
		lastAction->rgRange.nLastSample = pSelf->rgCopyRange.nLastSample - pSelf->rgCopyRange.nFirstSample;

		// soundworker in pastePiece moves the copied range (as well as selected range) if needed so it will be valid, no need to zero it
		// TODO: consider to make it more explicit

		MainWindow_UpdateView(pSelf->mainView, pSelf, soundDataChange | selectionChange, lastAction);
		HeapFree(GetProcessHeap(), 0, lastAction);
	}
	pSelf->isChanged = TRUE;
}

void Model_MakeSilent(PMODELDATA pSelf)
{
	if (pSelf->rgSelectedRange.nFirstSample != pSelf->rgSelectedRange.nLastSample) {
		MainWindow_UpdateView(pSelf->mainView, pSelf, playbackStop, NULL);

		makeSilent(pSelf);
		// zero the copy range just for safety
		ZeroMemory(&pSelf->rgCopyRange, sizeof(SAMPLERANGE));

		PACTIONINFO lastAction = HeapAlloc(GetProcessHeap(), 0, sizeof(ACTIONINFO));
		lastAction->action = eaMakeSilent;
		lastAction->rgRange = pSelf->rgSelectedRange;
		MainWindow_UpdateView(pSelf->mainView, pSelf, soundDataChange, lastAction);
		HeapFree(GetProcessHeap(), 0, lastAction);
	}
	pSelf->isChanged = TRUE;
}

void Model_SelectAll(PMODELDATA pSelf)
{
	if (pSelf->curFile != INVALID_HANDLE_VALUE) {
		pSelf->rgSelectedRange.nFirstSample = 0;
		pSelf->rgSelectedRange.nLastSample = pSelf->dataSize / pSelf->wfxFormat.nBlockAlign - 1;

		MainWindow_UpdateView(pSelf->mainView, pSelf, selectionChange, NULL);
	}
}

void Model_Reverse(PMODELDATA pSelf)
{
	if (pSelf->rgSelectedRange.nFirstSample != pSelf->rgSelectedRange.nLastSample) {
		MainWindow_UpdateView(pSelf->mainView, pSelf, playbackStop, NULL);

		reversePiece(pSelf);

		PACTIONINFO lastAction = HeapAlloc(GetProcessHeap(), 0, sizeof(ACTIONINFO));
		lastAction->action = eaReverse;
		lastAction->rgRange = pSelf->rgSelectedRange;
		MainWindow_UpdateView(pSelf->mainView, pSelf, soundDataChange, lastAction);
		HeapFree(GetProcessHeap(), 0, lastAction);
	}
	pSelf->isChanged = TRUE;
}

void Model_UpdateSelection(PMODELDATA pSelf, BOOL isRangeSelected, PSAMPLERANGE range)
{
	if (pSelf->curFile != INVALID_HANDLE_VALUE) {
		if (!isRangeSelected) {
			// only left click has occured, nothing has been selected yet
			pSelf->rgSelectedRange.nFirstSample = pSelf->rgSelectedRange.nLastSample = range->nFirstSample;
		} else {
			// right click occured, we have a selected range
			pSelf->rgSelectedRange.nLastSample = range->nLastSample;
			if (pSelf->rgSelectedRange.nLastSample < pSelf->rgSelectedRange.nFirstSample) {
				unsigned long tmp = pSelf->rgSelectedRange.nFirstSample;
				pSelf->rgSelectedRange.nFirstSample = pSelf->rgSelectedRange.nLastSample;
				pSelf->rgSelectedRange.nLastSample = tmp;
			}
		}
		MainWindow_UpdateView(pSelf->mainView, pSelf, selectionChange, NULL);
	}
}

// zoomIn: true if zooming in, false if zooming out
void Model_ZoomLevelChange(PMODELDATA pSelf, enum zoomingLevelType zoomingType)
{
	switch (zoomingType) {
		case zoomIn:
			MainWindow_UpdateView(pSelf->mainView, pSelf, zoomingIn, NULL);
			break;
		case zoomOut:
			MainWindow_UpdateView(pSelf->mainView, pSelf, zoomingOut, NULL);
			break;
		case fitInWindow:
			MainWindow_UpdateView(pSelf->mainView, pSelf, fittingInWindow, NULL);
			break;
	}
}

PSAMPLERANGE Model_GetSelectionInfo(PMODELDATA pSelf)
{
	return &pSelf->rgSelectedRange;
}
