#pragma once
#include "headers\drawingarea.h"
#include "headers\toolspanel.h"
#include "headers\player.h"

struct tagMODELDATA;
struct tagDRAWINGWINDATA;
struct tagTOOLSWINDATA;

typedef struct tagMAINWINDOWDATA {
	HWND winHandle;
	HWND drawingAreaHandle;
	HWND toolsWinHandle;
	struct tagMODELDATA *modelData;
	struct tagDRAWINGWINDATA *drawingArea;
	struct tagTOOLSWINDATA *toolsPanel;
	PPLAYERDATA player;
} MAINWINDATA, *PMAINWINDATA;

typedef struct tagCREATEINFO {
	struct tagMODELDATA *model;
	PMAINWINDATA parent;
} CREATEINFO, *PCREATEINFO;

void MainWindow_PlaybackStart(PMAINWINDATA pSelf);
void MainWindow_PlaybackStop(PMAINWINDATA pSelf);
void MainWindow_AttachDrawingArea(PMAINWINDATA pSelf, PDRAWINGWINDATA pDrawer);
void MainWindow_AttachToolsPanel(PMAINWINDATA pSelf, PTOOLSWINDATA pTools);
void MainWindow_UpdateView(PMAINWINDATA pSelf, int reasons, PACTIONINFO aiLastAction);
void MainWindow_ShowModalError(PMAINWINDATA winHandle, const wchar_t *const message, const wchar_t *const header, UINT boxType);
void MainWindow_RegisterClass(HINSTANCE hInstance, const wchar_t *className);
