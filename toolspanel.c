#define _UNICODE
#define UNICODE

#include <math.h>
#include <windows.h>
#include "headers\toolspanel.h"

void ToolsPanel_Draw(PTOOLSWINDATA pSelf)
{

	SaveDC(pSelf->backDC);
	FillRect(pSelf->backDC, &pSelf->rcClientSize, pSelf->bckgndBrush);
	
	// TODO: Array of RECTs and for?..
	FillRect(pSelf->backDC, &pSelf->rcPlayButtonPos, pSelf->buttonBckbndBrush);
	FillRect(pSelf->backDC, &pSelf->rcPauseButtonPos, pSelf->buttonBckbndBrush);
	FillRect(pSelf->backDC, &pSelf->rcStopButtonPos, pSelf->buttonBckbndBrush);

	SelectObject(pSelf->backDC, pSelf->greenPlayBrush);

	const float BUTTONPADDINGSCALE = 0.1;
	int sidePadding = (pSelf->rcPlayButtonPos.right - pSelf->rcPlayButtonPos.left) * BUTTONPADDINGSCALE;
	int topPadding = (pSelf->rcPlayButtonPos.bottom - pSelf->rcPlayButtonPos.top) * BUTTONPADDINGSCALE;
	/* drawing play button contents*/
	POINT playButtonVertexes[3];
	playButtonVertexes[0].x = pSelf->rcPlayButtonPos.left + sidePadding;
	playButtonVertexes[0].y = pSelf->rcPlayButtonPos.top + topPadding;
	playButtonVertexes[1].x = pSelf->rcPlayButtonPos.left + sidePadding;
	playButtonVertexes[1].y = pSelf->rcPlayButtonPos.bottom - topPadding;
	playButtonVertexes[2].x = pSelf->rcPlayButtonPos.right - sidePadding;
	playButtonVertexes[2].y = (pSelf->rcPlayButtonPos.bottom - pSelf->rcPlayButtonPos.top) / 2;

	Polygon(pSelf->backDC, playButtonVertexes, 3);
	/**/
	/* drawing pause button */
	const float PAUSEBARTHICKNESSSCALE = 0.3;
	RECT pauseButtonBar = (RECT) { pSelf->rcPauseButtonPos.left + sidePadding,
		pSelf->rcPauseButtonPos.top + topPadding,
		(pSelf->rcPauseButtonPos.right - pSelf->rcPauseButtonPos.left) * PAUSEBARTHICKNESSSCALE + pSelf->rcPauseButtonPos.left + sidePadding,
		pSelf->rcPauseButtonPos.bottom - topPadding };

	FillRect(pSelf->backDC, &pauseButtonBar, pSelf->buttonFillBrush);
	pauseButtonBar.left = pSelf->rcPauseButtonPos.right - (pSelf->rcPauseButtonPos.right - pSelf->rcPauseButtonPos.left) * PAUSEBARTHICKNESSSCALE - sidePadding;
	pauseButtonBar.right = pSelf->rcPauseButtonPos.right - sidePadding;
	FillRect(pSelf->backDC, &pauseButtonBar, pSelf->buttonFillBrush);
	/**/
	/* drawing stop button */
	RECT stop = (RECT) { pSelf->rcStopButtonPos.left + sidePadding,
		pSelf->rcStopButtonPos.top + topPadding,
		pSelf->rcStopButtonPos.right - sidePadding,
		pSelf->rcStopButtonPos.bottom - topPadding };
	FillRect(pSelf->backDC, &stop, pSelf->buttonFillBrush);
	/**/
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

	if (uMsg == WM_CREATE) {
		pToolsSelf = (PTOOLSWINDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TOOLSWINDATA));
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)pToolsSelf);
		pToolsSelf->winHandle = hWnd;
		pToolsSelf->totalButtonsCount = 3;

		pToolsSelf->bckgndBrush = CreateSolidBrush(RGB(55, 55, 55));

		pToolsSelf->greenPlayBrush = CreateSolidBrush(RGB(65,208,34));
		pToolsSelf->buttonFillBrush = CreateSolidBrush(RGB(0,0,0));
		pToolsSelf->buttonBckbndBrush = CreateSolidBrush(RGB(133,133,133));
		pToolsSelf->buttonsOutline = CreatePen(PS_SOLID, 3, RGB(210,210,210));

		GetClientRect(pToolsSelf->winHandle, &pToolsSelf->rcClientSize);

		RECT *resArray[3] = { &pToolsSelf->rcPlayButtonPos, &pToolsSelf->rcPauseButtonPos, &pToolsSelf->rcStopButtonPos };
		calculateButtonsPositions(pToolsSelf->totalButtonsCount, resArray, pToolsSelf->rcClientSize);

		ToolsPanel_CreateBackBuffer(pToolsSelf);	// ADD CHECKING!!!

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	pToolsSelf = (PTOOLSWINDATA)GetWindowLong(hWnd, 0);
	if (pToolsSelf == NULL) return DefWindowProc(hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
		case WM_PAINT:
			pToolsSelf->hdc = BeginPaint(pToolsSelf->winHandle, &ps);
			ToolsPanel_Draw(pToolsSelf);
			BitBlt(pToolsSelf->hdc, 0, 0, pToolsSelf->rcClientSize.right, pToolsSelf->rcClientSize.bottom, pToolsSelf->backDC, 0, 0, SRCCOPY);
			EndPaint(pToolsSelf->winHandle, &ps);
			return 0;
		case WM_SIZE:
			GetClientRect(pToolsSelf->winHandle, &(pToolsSelf->rcClientSize));
			RECT *resArray[3] = { &pToolsSelf->rcPlayButtonPos, &pToolsSelf->rcPauseButtonPos, &pToolsSelf->rcStopButtonPos };
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
	toolsWinClass.cbWndExtra = sizeof(void *); // extra space for struct with window data

	RegisterClassEx(&toolsWinClass);
}
