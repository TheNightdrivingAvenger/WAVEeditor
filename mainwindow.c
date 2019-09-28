#define UNICODE
#define _UNICODE
#include <windows.h>
#include <math.h>

#include "headers\mainwindow.h"
#include "headers\mainmenu.h"
#include "headers\constants.h"
#include "headers\drawingarea.h"
#include "headers\modeldata.h"

void MainWindow_UpdateView(PMAINWINDATA pSelf, int reasons, PACTIONINFO aiLastAction)
{
	if (reasons & newFileOpened) {
		PUPDATEINFO updInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(UPDATEINFO));
		updInfo->soundData = pSelf->modelData->soundData;
		updInfo->dataSize = pSelf->modelData->dataSize;
		updInfo->wfxFormat = &(pSelf->modelData->wfxFormat);
		DrawingArea_DrawNewFile(pSelf->drawingArea, updInfo);
		HeapFree(GetProcessHeap(), 0, updInfo);
	}
	if (reasons & soundDataChange) { // requires ACTIONINFO structure
		PUPDATEINFO updInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(UPDATEINFO));
		updInfo->soundData = pSelf->modelData->soundData;
		updInfo->dataSize = pSelf->modelData->dataSize;
		updInfo->wfxFormat = &(pSelf->modelData->wfxFormat);
		DrawingArea_UpdateCache(pSelf->drawingArea, updInfo, aiLastAction);
		HeapFree(GetProcessHeap(), 0, updInfo);
	}

	if (reasons & curFileNameChange) {
		//SetWindowText(pSelf->wiHandle, chosenFile);
	}
	if (reasons & cursorPosChange) {
	}
	if (reasons & selectionChange) {
	}
	// TODO: consider not resetting the cursor but moving it
	if (reasons & cursorReset) {
		DrawingArea_ResetCursor(pSelf->drawingArea);
	}
}

void MainWindow_AttachDrawingArea(PMAINWINDATA pSelf, PDRAWINGWINDATA pDrawer)
{
	pSelf->drawingArea = pDrawer;
}

void MainWindow_AttachToolsPanel(PMAINWINDATA pSelf, PTOOLSWINDATA pTools)
{
	pSelf->toolsPanel = pTools;
}

wchar_t *openFileExecute(HWND parentWindow)
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
		Model_ChangeCurFile(pSelf->modelData, chosenFile);
	}
}

wchar_t *saveFileAsExecute(HWND parentWindow)
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

void MainWindow_SaveFileAs(PMAINWINDATA pSelf, BOOL saveSelected)
{
	LRESULT res;
	wchar_t *chosenFile = saveFileAsExecute(pSelf->winHandle);
	if (chosenFile) {
		Model_SaveFileAs(pSelf->modelData, saveSelected, chosenFile);
	}
}

// TODO: temporary solution; may be improved and changed
void MainWindow_ShowModalError(PMAINWINDATA pSelf, const wchar_t *const message, const wchar_t *const header, UINT boxType)
{
	MessageBox(pSelf->winHandle, message, header, boxType);
}

void MainWindow_AddDrawingViewPart(PMAINWINDATA pSelf, PDRAWINGWINDATA pPainter)
{
	pSelf->drawingArea = pPainter;
}

void MainWindow_AddToolsPanelViewPart(PMAINWINDATA pSelf, PTOOLSWINDATA pPanel)
{
	pSelf->toolsPanel = pPanel;
}

void MainWindow_AddPlayerViewPart(PMAINWINDATA pSelf, PPLAYERDATA pPlayer)
{
	pSelf->player = pPlayer;
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

		// for now this window is responsible for model's memory; may make application (main.c) the owner
		pMainSelf->modelData = (PMODELDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MODELDATA));
		Model_Reset(pMainSelf->modelData);
		Model_AttachView(pMainSelf->modelData, pMainSelf);
		
		PCREATEINFO createInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(CREATEINFO));
		createInfo->model = pMainSelf->modelData;
		createInfo->parent = pMainSelf;
		drawingAreaHandle = CreateWindowEx(0, L"DrawingArea", NULL, WS_CHILD | WS_VISIBLE,
			0, (int)truncf(newSize.bottom * DRAWINGWINPOSYSCALE), newSize.right,
			(int)truncf(newSize.bottom * (1 - DRAWINGWINPOSYSCALE)), hWnd, 0, 0, createInfo);

		toolsPanelHandle = CreateWindowEx(0, L"ToolsPanel", NULL, WS_CHILD | WS_VISIBLE,
			0, 0, newSize.right, (int)truncf(newSize.bottom * DRAWINGWINPOSYSCALE), hWnd, 0, 0, createInfo);
	
		if (!drawingAreaHandle || !toolsPanelHandle) {
			return -1; // couldn't create drawing area or tools panel; CreateWindowEx in main window will return NULL handle
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
					MainWindow_ChangeCurFile(pMainSelf);
					return 0;
				case AMM_SAVEFILEAS:
					MainWindow_SaveFileAs(pMainSelf, FALSE);
					return 0;
				case AMM_COPY:
					Model_CopySelected(pMainSelf->modelData);
					break;
				case AMM_DELETE:
					Model_DeletePiece(pMainSelf->modelData);
					break;
				case AMM_PASTE:
					Model_PastePiece(pMainSelf->modelData);
					break;
				case AMM_MAKESILENT:
					Model_MakeSilent(pMainSelf->modelData);
					break;
				case AMM_SELECTALL:
					Model_SelectAll(pMainSelf->modelData);
					break;
				case AMM_SAVESELECTED:
					MainWindow_SaveFileAs(pMainSelf, TRUE);
					break;
				case AMM_REVERSESELECTED:
					Model_Reverse(pMainSelf->modelData);
					break;
				case AMM_ABOUT:
					MessageBox(pMainSelf->winHandle, L"Аудиоредактор WAVE-файлов.\nЕремеев Г. С., гр. 751002. БГУИР, ФКСиС, кафедра ПОИТ, 2019 г.", L"О программе", MB_ICONINFORMATION);
					break;
			}
			return 0;
		// MULTIMEDIA CONTROL // TODO: move to model?
		case WOM_DONE:
			Player_CleanUpAfterPlaying(pMainSelf->player);
			return 0;
		// *** //
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
