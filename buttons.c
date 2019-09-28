
#include <Windows.h>

#include "headers\buttoninfo.h"

void PlayButton_Draw(PBUTTONINFO pInfo, HDC targetDC)
{
	const float BUTTONPADDINGSCALE = 0.1;
	int sidePadding = (pInfo->buttonPos.right - pInfo->buttonPos.left) * BUTTONPADDINGSCALE;
	int topPadding = (pInfo->buttonPos.bottom - pInfo->buttonPos.top) * BUTTONPADDINGSCALE;

	POINT playButtonVertexes[3];
	playButtonVertexes[0].x = pInfo->buttonPos.left + sidePadding;
	playButtonVertexes[0].y = pInfo->buttonPos.top + topPadding;
	playButtonVertexes[1].x = pInfo->buttonPos.left + sidePadding;
	playButtonVertexes[1].y = pInfo->buttonPos.bottom - topPadding;
	playButtonVertexes[2].x = pInfo->buttonPos.right - sidePadding;
	playButtonVertexes[2].y = (pInfo->buttonPos.bottom - pInfo->buttonPos.top) / 2;

	HBRUSH tempBrush = SelectObject(targetDC, pInfo->brush);
	HPEN tempPen = SelectObject(targetDC, pInfo->pen);

	Polygon(targetDC, playButtonVertexes, 3);

	SelectObject(targetDC, tempBrush);
	SelectObject(targetDC, tempPen);
}

void PauseButton_Draw(PBUTTONINFO pInfo, HDC targetDC)
{
	const float BUTTONPADDINGSCALE = 0.1;
	int sidePadding = (pInfo->buttonPos.right - pInfo->buttonPos.left) * BUTTONPADDINGSCALE;
	int topPadding = (pInfo->buttonPos.bottom - pInfo->buttonPos.top) * BUTTONPADDINGSCALE;

	const float PAUSEBARTHICKNESSSCALE = 0.3;
	RECT pauseButtonBar = (RECT) { pInfo->buttonPos.left + sidePadding,
		pInfo->buttonPos.top + topPadding,
		(pInfo->buttonPos.right - pInfo->buttonPos.left) * PAUSEBARTHICKNESSSCALE + pInfo->buttonPos.left + sidePadding,
		pInfo->buttonPos.bottom - topPadding };

	HBRUSH tempBrush = SelectObject(targetDC, pInfo->brush);
	HPEN tempPen = SelectObject(targetDC, pInfo->pen);

	Rectangle(targetDC, pauseButtonBar.left, pauseButtonBar.top, pauseButtonBar.right, pauseButtonBar.bottom);
	pauseButtonBar.left = pInfo->buttonPos.right - (pInfo->buttonPos.right - pInfo->buttonPos.left) * PAUSEBARTHICKNESSSCALE - sidePadding;
	pauseButtonBar.right = pInfo->buttonPos.right - sidePadding;
	Rectangle(targetDC, pauseButtonBar.left, pauseButtonBar.top, pauseButtonBar.right, pauseButtonBar.bottom);

	SelectObject(targetDC, tempBrush);
	SelectObject(targetDC, tempPen);
}

void StopButton_Draw(PBUTTONINFO pInfo, HDC targetDC)
{
	const float BUTTONPADDINGSCALE = 0.1;
	int sidePadding = (pInfo->buttonPos.right - pInfo->buttonPos.left) * BUTTONPADDINGSCALE;
	int topPadding = (pInfo->buttonPos.bottom - pInfo->buttonPos.top) * BUTTONPADDINGSCALE;

	RECT stop = (RECT) { pInfo->buttonPos.left + sidePadding,
		pInfo->buttonPos.top + topPadding,
		pInfo->buttonPos.right - sidePadding,
		pInfo->buttonPos.bottom - topPadding };

	HBRUSH tempBrush = SelectObject(targetDC, pInfo->brush);
	HPEN tempPen = SelectObject(targetDC, pInfo->pen);

	Rectangle(targetDC, stop.left, stop.top, stop.right, stop.bottom);

	SelectObject(targetDC, tempBrush);
	SelectObject(targetDC, tempPen);
}


// TODO: OnClick should just change the button's state (for example change the style)
void PlayButton_OnClick(void)
{
	//MessageBoxW(NULL, L"Play clicked", L"", MB_OK);
}

void PauseButton_OnClick(void)
{
	//MessageBoxW(NULL, L"Pause clicked", L"", MB_OK);
}

void StopButton_OnClick(void)
{
	//MessageBoxW(NULL, L"Stop clicked", L"", MB_OK);
}
