#pragma once
#include "headers\buttoninfo.h"
#include "headers\toolspanel.h"

typedef struct tagLISTNODE {
	PBUTTONINFO pButton;
	ClickHandler handler;
	struct tagLISTNODE *next;
} LISTNODE, *PLISTNODE;

typedef struct tagLISTDATA {
	PLISTNODE first;
	PLISTNODE last;
} LISTDATA, *PLISTDATA;
