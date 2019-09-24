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

HANDLE openFile(wchar_t *fileName)
{
	HANDLE hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	return hFile;
}

wchar_t *MainWindow_OpenFileExecute(PMAINWINDATA pSelf)
{
	OPENFILENAME ofn = {0};

	static wchar_t fileName[MAX_PATH];

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = pSelf->winHandle;
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
	wchar_t *chosenFile = MainWindow_OpenFileExecute(pSelf);
	if (chosenFile) {
		HANDLE hFile;
		if ((hFile = openFile(chosenFile)) != INVALID_HANDLE_VALUE)
		{
			// TODO: continue to detach the child window from model
			switch (SendMessage(pSelf->drawingAreaHandle, DM_SETINPUTFILE, (WPARAM)hFile, 0)) {
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

wchar_t *MainWindow_SaveFileAsExecute(PMAINWINDATA pSelf)
{
	OPENFILENAME sfn = {0};

	static wchar_t savedFileName[MAX_PATH];

	sfn.lStructSize = sizeof(OPENFILENAME);
	sfn.hwndOwner = pSelf->winHandle;
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
	wchar_t *chosenFile = MainWindow_SaveFileAsExecute(pSelf);
	if (chosenFile) {
		HANDLE hFile;
		if ((hFile = saveFile(chosenFile)) != INVALID_HANDLE_VALUE) {
			if (saveSelected) {
				res = SendMessage(pSelf->drawingAreaHandle, ADM_SAVESELECTED, (WPARAM)hFile, 0);
			} else {
				res = SendMessage(pSelf->drawingAreaHandle, DM_SAVEFILEAS, (WPARAM)hFile, 0);
			}
			switch (res) {
				case 0:
					//all good
					if (saveSelected) {
						CloseHandle(hFile);
					} else {
						SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
						if (SendMessage(pSelf->drawingAreaHandle, DM_SETINPUTFILE, (WPARAM)hFile, 0) != 0) { // saved file reading error
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

void MainWindow_SendUPDCACHEToView(PMAINWINDATA pSelf)
{
	updInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(UPDATEINFO));
	updInfo->soundData = pMainSelf->modelData->soundData;
	updInfo->dataSize = pMainSelf->modelData->dataSize;
	updInfo->wfxFormat = &(pMainSelf->modelData->wfxFormat);
	SendMessage(pMainSelf->drawingAreaHandle, UPD_CACHE, 0, updInfo);
}

LRESULT CALLBACK MainWindow_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND drawingAreaHandle;
	HWND toolsPanelHandle;
	PMAINWINDATA pMainSelf;
	RECT newSize;
	LPMINMAXINFO lpMMI;

	PUPDATEINFO updInfo;

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
			0, truncf(newSize.bottom * DRAWINGWINPOSYSCALE), newSize.right, truncf(newSize.bottom * (1 - DRAWINGWINPOSYSCALE)), hWnd, 0, 0, NULL);
		toolsPanelHandle = CreateWindowEx(0, L"ToolsPanel", NULL, WS_CHILD | WS_VISIBLE,
			0, 0, newSize.right, truncf(newSize.bottom * DRAWINGWINPOSYSCALE), hWnd, 0, 0, NULL);
	
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
				case IDM_OPENFILE:
					MainWindow_ChangeCurFile(pMainSelf);
					return 0;
				case IDM_SAVEFILEAS:
					MainWindow_SaveFileAs(pMainSelf, FALSE);
					return 0;
				case IDM_COPY:
					// structure assignment is OK, just a memory copy
					pMainSelf->modelData->rgCopyRange = pSelf->modelData->rgSelectedRange;
					break;
				case IDM_DELETE:
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
				case IDM_PASTE:
					if (pMainSelf->modelData->rgCopyRange.nFirstSample != pMainSelf->modelData->rgCopyRange.nLastSample) {
						pastePiece(pMainSelf->modelData);
						pMainSelf->modelData->rgSelectedRange.nLastSample = pMainSelf->modelData->rgSelectedRange.nFirstSample = 0;

						MainWindow_SendUPDCACHEToView(pMainSelf);
						SendMessage(pMainSelf->drawingAreaHandle, UPD_CURSOR, 0, 0);
					}
					pMainSelf->modelData->isChanged = TRUE;
					break;
				case IDM_MAKESILENT:
					if (pMainSelf->modelData->rgSelectedRange.nFirstSample != pMainSelf->modelData->rgSelectedRange.nLastSample) {
						makeSilent(pMainSelf->modelData);
						pMainSelf->modelData->rgSelectedRange.nLastSample = pMainSelf->modelData->rgSelectedRange.nFirstSample = 0;

						MainWindow_SendUPDCACHEToView(pMainSelf);
						SendMessage(pMainSelf->drawingAreaHandle, UPD_CURSOR, 0, 0);
					}
					pMainSelf->modelData->isChanged = TRUE;
					break;
				case IDM_SELECTALL:
					if (pDrawSelf->modelData->curFile != INVALID_HANDLE_VALUE) {
						pMainSelf->modelData->rgSelectedRange.nFirstSample = 1;
						pMainSelf->modelData->rgSelectedRange.nLastSample = pMainSelf->modelData->dataSize / pMainSelf->modelData->wfxFormat.nBlockAlign;
						
						SendMessage(pMainSelf->drawingAreaHandle, UPD_SELECTALL, 0, 0);
					}
					break;
				case IDM_SAVESELECTED:
					// TODO: sort this
					MainWindow_SaveFileAs(pMainSelf, TRUE);
					break;
				case IDM_REVERSESELECTED:
					if (pMainSelf->modelData->rgSelectedRange.nFirstSample != pMainSelf->modelData->rgSelectedRange.nLastSample) {
						reversePiece(pMainSelf->modelData);
						
						MainWindow_SendUPDCACHEToView(pMainSelf);
					}
					pDrawSelf->modelData->isChanged = TRUE;
					break;
				case IDM_ABOUT:
					MessageBox(pMainSelf->winHandle, L"Аудиоредактор WAVE-файлов.\nЕремеев Г. С., гр. 751002. БГУИР, ФКСиС, кафедра ПОИТ, 2019 г.", L"О программе", MB_ICONINFORMATION);
					break;
			}
			return 0;
		case UPD_SELECTEDRANGE:
			if (!wParam) {
				// only left click has occured, nothing has been selected yet
				pMainSelf->modelData.rgSelectedRange.nFirstSample = ((PRANGE)lParam)->nFirstSample;
			} else {
				// right click occured, we have a selected range
				pMainSelf->modelData.rgSelectedRange.nLastSample = ((PRANGE)lParam)->nLastSample;
				if (pMainSelf->modelData->rgSelectedRange.nLastSample < pMainSelf->modelData->rgSelectedRange.nFirstSample) {
					unsigned long tmp = pMainSelf->modelData->rgSelectedRange.nFirstSample;
					pMainSelf->modelData->rgSelectedRange.nFirstSample = pMainSelf->modelData->rgSelectedRange.nLastSample;
					pMainSelf->modelData->rgSelectedRange.nLastSample = tmp;
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
				MoveWindow(pMainSelf->drawingAreaHandle, 0, truncf(newSize.bottom * DRAWINGWINPOSYSCALE), newSize.right, 
					truncf(newSize.bottom * (1 - DRAWINGWINPOSYSCALE)), FALSE);
				MoveWindow(pMainSelf->toolsWinHandle, 0, 0, newSize.right, truncf(newSize.bottom * DRAWINGWINPOSYSCALE), TRUE);

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
