/*CREATES MAIN WINDOW*/

#define UNICODE
#define _UNICODE
#include <Windows.h>
#include <mmreg.h>

#include "headers\mainwindow.h"
#include "headers\drawingarea.h"
#include "headers\toolspanel.h"

INT APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int cmdShow)
{
	MainWindow_RegisterClass(hInstance, L"MainWindow");
	DrawingArea_RegisterClass(hInstance, L"DrawingArea");
	ToolsPanel_RegisterClass(hInstance, L"ToolsPanel");
	
    HWND mainWindowHandle = CreateWindowEx(0, L"MainWindow", L"WAVeform audio editor", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
		10, 10, 1818, 1000, 0, 0, hInstance, NULL); 
	if (!mainWindowHandle) {
		MessageBox(0, L"Что-то пошло не так... Перезапустите программу", L"Ошибка", MB_ICONERROR);
		return 1;
	}

	UpdateWindow(mainWindowHandle);

	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessage(&msg, 0, 0, 0)) != 0) {
		if (bRet == -1) {
			MessageBoxW(0, L"Что-то пошло не так... Перезапустите программу", L"Ошибка", MB_ICONERROR);
		} else {
            DispatchMessage(&msg);
		}
	}
	return (int) msg.wParam;
}
