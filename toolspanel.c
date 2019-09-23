#define _UNICODE
#define UNICODE

#include <math.h>
#include <Windows.h>
#include <windowsx.h>
#include "headers\toolspanel.h"
#include "headers\buttoninfo.h"
#include "headers\buttons.h"
#include "headers\player.h"
#include "headers\constants.h"
#include "headers\list.h"

inline POINT mouseCoordsToPoint(LPARAM lParam)
{
	POINT p = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	return p;
}

void ToolsPanel_Draw(PTOOLSWINDATA pSelf)
{
	SaveDC(pSelf->backDC);
	FillRect(pSelf->backDC, &pSelf->rcClientSize, pSelf->bckgndBrush);
	
	for (int i = 0; i < pSelf->totalButtonsCount; i++) {
		FillRect(pSelf->backDC, &pSelf->buttons[i].buttonPos, pSelf->buttonBckgndBrush);
		pSelf->buttons[i].Button_Draw(&(pSelf->buttons[i]), pSelf->backDC);
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

LRESULT CALLBACK ToolsPanel_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	PTOOLSWINDATA pToolsSelf;
	POINT clickCoords;

	if (uMsg == WM_CREATE) {
		pToolsSelf = (PTOOLSWINDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TOOLSWINDATA));
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)pToolsSelf);
		pToolsSelf->winHandle = hWnd;
		pToolsSelf->totalButtonsCount = 3;

		pToolsSelf->bckgndBrush = CreateSolidBrush(RGB(55, 55, 55));

		pToolsSelf->buttonBckgndBrush = CreateSolidBrush(RGB(133,133,133));

		GetClientRect(pToolsSelf->winHandle, &pToolsSelf->rcClientSize);

		pToolsSelf->buttons = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BUTTONINFO) * pToolsSelf->totalButtonsCount);

		// TODO: sort things out with Variable Length Arrays here. Improve buttons objects initialization
		RECT *resArray[3];
		for (int i = 0; i < pToolsSelf->totalButtonsCount; i++) {
			resArray[i] = &pToolsSelf->buttons[i].buttonPos;
		}
		calculateButtonsPositions(pToolsSelf->totalButtonsCount, resArray, pToolsSelf->rcClientSize);

		// *** BUTTONS INTERFACE OBJECTS INITIALIZATION *** /// 0 -- play button, 1 -- pause button, 2 -- stop button
		pToolsSelf->buttons[0].pen = CreatePen(PS_SOLID, 3, RGB(210,210,210));
		pToolsSelf->buttons[0].brush = CreateSolidBrush(RGB(65,208,34));
		pToolsSelf->buttons[0].Button_Draw = PlayButton_Draw;
		pToolsSelf->buttons[0].Button_OnClick = PlayButton_OnClick;

		pToolsSelf->buttons[1].pen = pToolsSelf->buttons[0].pen;
		pToolsSelf->buttons[1].brush = CreateSolidBrush(RGB(0,0,0));
		pToolsSelf->buttons[1].Button_Draw = PauseButton_Draw;
		pToolsSelf->buttons[1].Button_OnClick = PauseButton_OnClick;

		pToolsSelf->buttons[2].pen = pToolsSelf->buttons[0].pen;
		pToolsSelf->buttons[2].brush = CreateSolidBrush(RGB(254,5,18));
		pToolsSelf->buttons[2].Button_Draw = StopButton_Draw;
		pToolsSelf->buttons[2].Button_OnClick = StopButton_OnClick;

		// *** //
		ToolsPanel_CreateBackBuffer(pToolsSelf);	// ADD CHECKING!!!

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	pToolsSelf = (PTOOLSWINDATA)GetWindowLong(hWnd, 0);
	if (pToolsSelf == NULL) return DefWindowProc(hWnd, uMsg, wParam, lParam);

	//RECT *resArray[pToolsSelf->totalButtonsCount];
	switch (uMsg) {
		case WM_LBUTTONDOWN:
			clickCoords = mouseCoordsToPoint(lParam);
			int i = 0;
			for (; i < pToolsSelf->totalButtonsCount; i++) {
				if (PtInRect(&(pToolsSelf->buttons[i].buttonPos), clickCoords)) {
					pToolsSelf->buttons[i].Button_OnClick();
					break;
				}
			}
			switch (i) {
				case 0: //play
					Player_Play(hWnd, pToolsSelf->modelData);
					break;
				case 1: //pause
					break;
				case 2: //stop
					break;
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
			RECT *resArray[3];
			for (int i = 0; i < pToolsSelf->totalButtonsCount; i++) {
				resArray[i] = &pToolsSelf->buttons[i].buttonPos;
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
