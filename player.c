#define _UNICODE
#define UNICODE

#include <windows.h>

#include "headers\player.h"
#include "headers\modeldata.h"

void CALLBACK waveCallback(HWAVEOUT hWaveDevice, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg == WOM_DONE) {
		((PPLAYERDATA)dwInstance)->playerState = stopped;
	}
	PostMessage(((PPLAYERDATA)dwInstance)->callbackWindow, uMsg, dwParam1, dwParam2);
}

PPLAYERDATA Player_Init(PWAVEFORMATEX wfxFormat, HWND callbackWindow)
{
	PPLAYERDATA result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLAYERDATA));
	result->playerState = stopped;
	result->callbackWindow = callbackWindow;

	MMRESULT res = waveOutOpen(&result->deviceHandle, WAVE_MAPPER, wfxFormat, (DWORD_PTR)waveCallback, (DWORD_PTR)result, CALLBACK_FUNCTION);
	if (res == MMSYSERR_NOERROR)
	{
		//device was opened successfully
		return result;
	} else { // can add more cases if want to return status code and display error message
		HeapFree(GetProcessHeap(), 0, result);
		return NULL;
	}
}

int Player_Play(PPLAYERDATA pSelf, void *soundData, int dataSize)
{
	ZeroMemory(&pSelf->lastUsedHeader, sizeof(WAVEHDR));
	pSelf->lastUsedHeader.lpData = soundData;
	pSelf->lastUsedHeader.dwBufferLength = dataSize;

	MMRESULT res = waveOutPrepareHeader(pSelf->deviceHandle, &pSelf->lastUsedHeader, sizeof(WAVEHDR));

	if (res == MMSYSERR_NOERROR)
	{
		// header is prepared, can play
		pSelf->playerState = playing;
		res = waveOutWrite(pSelf->deviceHandle, &pSelf->lastUsedHeader, sizeof(WAVEHDR));
	}
	else
	{
		// TODO: handle some header error
		return 1;
	}

	return 0;
}

void Player_Pause(PPLAYERDATA pSelf)
{
	// TODO: implement pausing
}

void Player_Stop(PPLAYERDATA pSelf)
{
	waveOutReset(pSelf->deviceHandle);
	Player_CleanUpAfterPlaying(pSelf);
	pSelf->playerState = stopped;
}

void Player_CleanUpAfterPlaying(PPLAYERDATA pSelf)
{
	waveOutUnprepareHeader(pSelf->deviceHandle, &pSelf->lastUsedHeader, sizeof(WAVEHDR));
}

MMRESULT Player_Dispose(PPLAYERDATA pSelf)
{
	if (pSelf->playerState != stopped) {
		waveOutReset(pSelf->deviceHandle);
		Player_CleanUpAfterPlaying(pSelf);
		pSelf->playerState = stopped;
	}
	return waveOutClose(pSelf->deviceHandle);
}
