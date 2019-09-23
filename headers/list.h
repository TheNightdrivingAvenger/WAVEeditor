#pragma once
#include "headers\buttoninfo.h"
#include "headers\toolspanel.h"

typedef struct tagLISTDATA {
	PLISTNODE first;
	PLISTNODE last;
} LISTDATA, *PLISTDATA;

typedef struct tagLISTNODE {
	PBUTTONINFO pButton;
	ClickHandler handler;
	struct tagLISTNODE *next;
} LISTNODE, *PLISTNODE;
