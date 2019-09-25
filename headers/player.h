#include "headers\modeldata.h"

typedef struct _tagPLAYERDATA {
	HWAVEOUT deviceHandle;
	WAVEHDR lastUsedHeader;
} PLAYERDATA, *PPLAYERDATA;

int Player_Play(PPLAYERDATA pSelf, void *soundData, int dataSize);
