#define _UNICODE
#define UNICODE

#include <math.h>
#include <Windows.h>
#include <windowsx.h>
#include "headers\toolspanel.h"
#include "headers\buttoninfo.h"
#include "headers\buttons.h"
#include "headers\constants.h"
#include "headers\list.h"
#include "headers\mainwindow.h"
#include "headers\modeldata.h"

inline POINT mouseCoordsToPoint(LPARAM lParam)
{
	POINT p = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	return p;
}

void ToolsPanel_Draw(PTOOLSWINDATA pSelf)
{
	SaveDC(pSelf->backDC);
	FillRect(pSelf->backDC, &pSelf->rcClientSize, pSelf->bckgndBrush);

	PLISTNODE node = pSelf->buttonsContainer->first;
	while (node != NULL) {
		FillRect(pSelf->backDC, &node->pButton->buttonPos, pSelf->buttonBckgndBrush);
		node->pButton->Button_Draw(node->pButton, pSelf->backDC);
		node = node->next;
	}

	RestoreDC(pSelf->backDC, -1);
}

void calculateButtonsPositions(int buttonsNum, RECT *result[buttonsNum], RECT rcClientSize)
{
	const float BUTTONMARGINXYSCALE = 0.03;
	const float BUTTONSIZEXSCALE = 0.07;
	int buttonWidth;
	int buttonHeight;
	int buttonMarginX;
	int buttonMarginY;

	buttonWidth = (int)truncf(rcClientSize.right * BUTTONSIZEXSCALE);
	buttonMarginX = (int)truncf(rcClientSize.right * BUTTONMARGINXYSCALE);
	buttonMarginY = (int)truncf(rcClientSize.bottom * BUTTONMARGINXYSCALE);
	buttonHeight = rcClientSize.bottom - buttonMarginY * 2;

	for (int i = 0; i < buttonsNum; i++) {
		result[i]->left = buttonMarginX * (i + 1) + buttonWidth * i;
		result[i]->top = buttonMarginY;
		result[i]->right = buttonMarginX * (i + 1) + buttonWidth * (i + 1);
		result[i]->bottom = buttonHeight;
	}
}

void ToolsPanel_DestroyBackBuffer(PTOOLSWINDATA pSelf)
{
	RestoreDC(pSelf->backDC, -1);
	DeleteObject(pSelf->backBufBitmap);
	DeleteDC(pSelf->backDC);
}

BOOL ToolsPanel_UpdateBackBuffer(PTOOLSWINDATA pSelf)
{
	if ((pSelf->backBufBitmap = CreateCompatibleBitmap(pSelf->backDC, pSelf->rcClientSize.right, pSelf->rcClientSize.bottom)) != NULL) {
		HBITMAP prevBitmap = SelectObject(pSelf->backDC, pSelf->backBufBitmap);
		DeleteObject(prevBitmap); // previous bitmap was created by application, so it must be destroyed
		return TRUE;
	}
	return FALSE;
}

BOOL ToolsPanel_CreateBackBuffer(PTOOLSWINDATA pSelf)
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

int ToolsPanel_Stop_OnClick(PTOOLSWINDATA pSelf)
{
	MainWindow_PlaybackStop(pSelf->parentWindow);
	return 0;
}

int ToolsPanel_Pause_OnClick(PTOOLSWINDATA pSelf)
{
	return 0;
}

int ToolsPanel_Play_OnClick(PTOOLSWINDATA pSelf)
{
	MainWindow_PlaybackStart(pSelf->parentWindow);
	return 0;
}

LRESULT CALLBACK ToolsPanel_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	PTOOLSWINDATA pToolsSelf;
	POINT clickCoords;
	PLISTNODE buttonNode;

	if (uMsg == WM_CREATE) {
		pToolsSelf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TOOLSWINDATA));
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)pToolsSelf);
		pToolsSelf->winHandle = hWnd;
		pToolsSelf->modelData = ((PCREATEINFO)((LPCREATESTRUCT)lParam)->lpCreateParams)->model;
		pToolsSelf->parentWindow = ((PCREATEINFO)((LPCREATESTRUCT)lParam)->lpCreateParams)->parent;
		MainWindow_AttachToolsPanel(pToolsSelf->parentWindow, pToolsSelf);

		pToolsSelf->totalButtonsCount = 3;

		pToolsSelf->bckgndBrush = CreateSolidBrush(RGB(55, 55, 55));

		pToolsSelf->buttonBckgndBrush = CreateSolidBrush(RGB(133,133,133));

		GetClientRect(pToolsSelf->winHandle, &pToolsSelf->rcClientSize);

		//PBUTTONINFO buttonInfoArray[pToolsSelf->totalButtonsCount];
		PBUTTONINFO buttonInfoArray[3];
		RECT *resArray[3];
		for (int i = 0; i < pToolsSelf->totalButtonsCount; i++) {
			buttonInfoArray[i] = HeapAlloc(GetProcessHeap(), 0, sizeof(BUTTONINFO)); // memory is freed by the list when it's destroyed
			resArray[i] = &buttonInfoArray[i]->buttonPos;
		}
		calculateButtonsPositions(pToolsSelf->totalButtonsCount, resArray, pToolsSelf->rcClientSize);

		// *** BUTTONS INTERFACE OBJECTS INITIALIZATION *** /// 0 -- play button, 1 -- pause button, 2 -- stop button
		// TODO: improve this and make it extensible
		buttonInfoArray[0]->pen = CreatePen(PS_SOLID, 3, RGB(210,210,210));
		buttonInfoArray[0]->brush = CreateSolidBrush(RGB(65,208,34));
		buttonInfoArray[0]->Button_Draw = PlayButton_Draw;
		buttonInfoArray[0]->Button_OnClick = PlayButton_OnClick;

		buttonInfoArray[1]->pen = buttonInfoArray[0]->pen;
		buttonInfoArray[1]->brush = CreateSolidBrush(RGB(0,0,0));
		buttonInfoArray[1]->Button_Draw = PauseButton_Draw;
		buttonInfoArray[1]->Button_OnClick = PauseButton_OnClick;

		buttonInfoArray[2]->pen = buttonInfoArray[0]->pen;
		buttonInfoArray[2]->brush = CreateSolidBrush(RGB(254,5,18));
		buttonInfoArray[2]->Button_Draw = StopButton_Draw;
		buttonInfoArray[2]->Button_OnClick = StopButton_OnClick;

		pToolsSelf->buttonsContainer = List_Create(buttonInfoArray[0], ToolsPanel_Play_OnClick);
		List_Add(pToolsSelf->buttonsContainer, buttonInfoArray[1], ToolsPanel_Pause_OnClick);
		List_Add(pToolsSelf->buttonsContainer, buttonInfoArray[2], ToolsPanel_Stop_OnClick);
		// TODO: improve this and make it extensible
		//for (int i = 1; i < buttonInfoArray; i++) {
			//List_Add(buttonInfoArray[i], ToolsPanel_Play_OnClick);
		//}
		// *** //
		ToolsPanel_CreateBackBuffer(pToolsSelf);	// ADD CHECKING!!!

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	pToolsSelf = (PTOOLSWINDATA)GetWindowLong(hWnd, 0);
	if (pToolsSelf == NULL) return DefWindowProc(hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
		case WM_LBUTTONDOWN:
			clickCoords = mouseCoordsToPoint(lParam);
			if ((buttonNode = List_FindBtnByCoords(pToolsSelf->buttonsContainer, clickCoords)) != NULL) {
				buttonNode->pButton->Button_OnClick(); // tell button to repaint or something
				buttonNode->handler(pToolsSelf); // perform according to the button action
			}
			return 0;
		case WM_PAINT:
			pToolsSelf->hdc = BeginPaint(pToolsSelf->winHandle, &ps);
			ToolsPanel_Draw(pToolsSelf);
			BitBlt(pToolsSelf->hdc, 0, 0, pToolsSelf->rcClientSize.right, pToolsSelf->rcClientSize.bottom, pToolsSelf->backDC, 0, 0, SRCCOPY);
			EndPaint(pToolsSelf->winHandle, &ps);
			return 0;
		case WM_SIZE:
			GetClientRect(pToolsSelf->winHandle, &(pToolsSelf->rcClientSize));
			// TODO: VLA or pointer
			RECT *resArray[3];
			PLISTNODE node = pToolsSelf->buttonsContainer->first;
			int i = 0;
			while (node != NULL) {
				resArray[i] = &node->pButton->buttonPos;
				node = node->next;
				i++;
			}
			calculateButtonsPositions(pToolsSelf->totalButtonsCount, resArray, pToolsSelf->rcClientSize);
			ToolsPanel_UpdateBackBuffer(pToolsSelf); // add checking?
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		case WM_ERASEBKGND:
			return TRUE;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

void ToolsPanel_RegisterClass(HINSTANCE hInstance, const wchar_t *className)
{
	WNDCLASSEXW toolsWinClass = {0};

	toolsWinClass.cbSize = sizeof(toolsWinClass);
	toolsWinClass.style = CS_GLOBALCLASS;
	toolsWinClass.lpszClassName = className;
	toolsWinClass.hInstance = hInstance;
	toolsWinClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	toolsWinClass.lpfnWndProc = ToolsPanel_WindowProc;
	toolsWinClass.hCursor = LoadCursor(0, IDC_ARROW);
	toolsWinClass.cbWndExtra = sizeof(LONG_PTR); // extra space for struct with window data

	RegisterClassEx(&toolsWinClass);
}
