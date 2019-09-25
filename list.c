
#include <windows.h>
#include "headers\list.h"

PLISTDATA List_Create(PBUTTONINFO buttonInfo, ClickHandler handler)
{
	PLISTNODE first = HeapAlloc(GetProcessHeap(), 0, sizeof(LISTNODE));
	first->next = NULL;
	first->pButton = buttonInfo;
	first->handler = handler;

	PLISTDATA listData = HeapAlloc(GetProcessHeap(), 0, sizeof(LISTDATA));
	listData->first = listData->last = first;
	listData->elementsCount = 1;
	return listData;
}

void List_Add(PLISTDATA pSelf, PBUTTONINFO buttonInfo, ClickHandler handler)
{
	pSelf->last->next = HeapAlloc(GetProcessHeap(), 0, sizeof(LISTNODE));
	pSelf->last = pSelf->last->next;
	pSelf->last->pButton = buttonInfo;
	pSelf->last->handler = handler;
	pSelf->last->next = NULL;
	(pSelf->elementsCount)++;
}

PLISTNODE List_FindBtnByCoords(PLISTDATA pSelf, POINT coords)
{
	PLISTNODE node = pSelf->first;
	while (node != NULL) {
		if (PtInRect(&(node->pButton->buttonPos), coords)) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

// TODO: implement remove, clear. // Don't forget to free the memory for pButton field
BOOL List_Remove(PLISTDATA pSelf, PBUTTONINFO buttonInfo, ClickHandler handler)
{
	PLISTNODE next = pSelf->first;
	while (next != NULL) {
		//if (buttonInfo
	}
}
