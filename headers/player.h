#include "headers\modeldata.h"

typedef struct _tagPLAYERDATA {
	HWAVEOUT deviceHandle;
	WAVEHDR lastUsedHeader;
	HWND callbackWindow;
	PlayerState playerState;
} PLAYERDATA, *PPLAYERDATA;

PPLAYERDATA Player_Init(PWAVEFORMATEX wfxFormat, HWND callbackWindow);
MMRESULT Player_Dispose(PPLAYERDATA pSelf);
int Player_Play(PPLAYERDATA pSelf, void *soundData, int dataSize);
void Player_Pause(PPLAYERDATA pSelf);
void Player_Stop(PPLAYERDATA pSelf);
void Player_CleanUpAfterPlaying(PPLAYERDATA pSelf);
