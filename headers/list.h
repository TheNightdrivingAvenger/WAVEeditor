#pragma once
#include "headers\buttoninfo.h"

typedef int (*ClickHandler)(void);

typedef struct tagLISTNODE {
	PBUTTONINFO pButton;
	ClickHandler handler;
	struct tagLISTNODE *next;
} LISTNODE, *PLISTNODE;

typedef struct tagLISTDATA {
	PLISTNODE first;
	PLISTNODE last;
	unsigned int elementsCount;
} LISTDATA, *PLISTDATA;

PLISTDATA List_Create(PBUTTONINFO buttonInfo, ClickHandler handler);
void List_Add(PLISTDATA pSelf, PBUTTONINFO buttonInfo, ClickHandler handler);
PLISTNODE List_FindBtnByCoords(PLISTDATA pSelf, POINT coords);
