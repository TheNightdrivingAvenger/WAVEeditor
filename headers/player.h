#include "headers\modeldata.h"

typedef struct _tagPLAYERDATA {
	HWAVEOUT deviceHandle;
	WAVEHDR lastUsedHeader;
} PLAYERDATA, *PPLAYERDATA;

int play(HWND responsibleWindow, PMODELDATA pModel);
