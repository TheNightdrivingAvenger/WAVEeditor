#define _UNICODE
#define UNICODE

#include <windows.h>

#include "headers\player.h"
#include "headers\modeldata.h"

void CALLBACK waveCallback(HWAVEOUT hWaveDevice, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	PostMessage((HWND)dwInstance, uMsg, dwParam1, dwParam2);
}

PPLAYERDATA Player_Init(PMODELDATA pModel, HWND callbackWindow)
{
	PPLAYERDATA result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLAYERDATA));

	MMRESULT res = waveOutOpen(&result->deviceHandle, WAVE_MAPPER, &pModel->wfxFormat, (DWORD_PTR)waveCallback, (DWORD_PTR)callbackWindow, CALLBACK_FUNCTION);
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
	HWAVEOUT deviceHandle;
	MMRESULT res;

	ZeroMemory(&pSelf->lastUsedHeader, sizeof(WAVEHDR));
	pSelf->lastUsedHeader.lpData = soundData;
	pSelf->lastUsedHeader.dwBufferLength = dataSize;

	res = waveOutPrepareHeader(deviceHandle, &pSelf->lastUsedHeader, sizeof(WAVEHDR));

	if (res == MMSYSERR_NOERROR)
	{
		// header is prepared, can play
		res = waveOutWrite(deviceHandle, &pSelf->lastUsedHeader, sizeof(WAVEHDR));
	}
	else
	{
		// TODO: handle some header error
		return 1;
	}

	return 0;
}

void Player_CleanUpAfterPlaying(PPLAYERDATA pSelf)
{
	waveOutReset(pSelf->deviceHandle);
	waveOutUnprepareHeader(pSelf->deviceHandle, &pSelf->lastUsedHeader, sizeof(WAVEHDR));
}
