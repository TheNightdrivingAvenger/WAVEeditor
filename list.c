
#include <windows.h>
#include "headers\list.h"

PLISTDATA List_Create(PBUTTONINFO buttonInfo, ClickHandler handler)
{
	PLISTNODE first = HeapAlloc(GetProcessHeap(), 0, sizeof(LISTNODE));
	first->next = NULL;
	first->pButton = buttonInfo;
	first->handler = handler;

	PLISTDATA listData = HeapAlloc(GetProcessHeap(), 0, sizeof(LISTDATA));
	listData->first = listData->last = NULL;
	return listData;
}

void List_Add(PLISTDATA pSelf, PBUTTONINFO buttonInfo, ClickHandler handler)
{
	pSelf->last->next = HeapAlloc(GetProcessHeap(), 0, sizeof(LISTNODE));
	pSelf->last = pSelf->last->next;
	pSelf->last->pButton = buttonInfo;
	pSelf->last->handler = handler;
	pSelf->last->next = NULL;
}

// TODO: implement remove, clear
BOOL List_Remove(PLISTDATA pSelf, PBUTTONINFO buttonInfo, ClickHandler handler)
{
	PLISTNODE next = pSelf->first;
	while (next != NULL) {
		//if (buttonInfo
	}
}
