#define _UNICODE
#define UNICODE

#include <windows.h>

#include "headers\player.h"
#include "headers\modeldata.h"
// TODO: consider making an object to store opened device and other info
void CALLBACK waveCallback(HWAVEOUT hWaveDevice, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	PostMessage((HWND)dwInstance, uMsg, dwParam1, dwParam2);
}

int playIt(HWND responsibleWindow, PMODELDATA pModel)
{
	HWAVEOUT deviceHandle;
	MMRESULT res;

	res = waveOutOpen(&deviceHandle, WAVE_MAPPER, &pModel->wfxFormat, (DWORD_PTR)waveCallback, (DWORD_PTR)responsibleWindow, CALLBACK_FUNCTION);
	if (res == MMSYSERR_NOERROR)
	{
		//device was opened successfully
		//LPVOID memBlckPointer;

		//DWORD dataSize;
		//fseek(*file, 40, 0);
		//fread(&dataSize, 4, 1, *file);

		//memBlckPointer = HeapAlloc(heapID, 0, dataSize);	//memory allocation; size value is taken from WAVE file itself
		//printf("Memory has been allocated\n");

		//fseek(*file, 44, 0);
		//fread(memBlckPointer, 1, dataSize, *file);

		WAVEHDR waveHdr = { 0 };

		waveHdr.lpData = pModel->soundData;
		waveHdr.dwBufferLength = pModel->dataSize;

		res = waveOutPrepareHeader(deviceHandle, &waveHdr, sizeof(WAVEHDR));
		
		if (res == MMSYSERR_NOERROR)
		{
			//printf("Header has been prepared...\n");

			//int audioLength = dataSize / (waveFormat.nSamplesPerSec * waveFormat.nChannels * waveFormat.wBitsPerSample / 8);
			//printf("Length of the file in seconds is %d\n", audioLength);
			//MessageBox(NULL, L"Be4 waveOutWrite", L"", MB_OK);
			res = waveOutWrite(deviceHandle, &waveHdr, sizeof(WAVEHDR));
			//MessageBox(NULL, L"After waveOutWrite", L"", MB_OK);
			//_getch();

			//waveOutReset(deviceHandle);

			//waveOutUnprepareHeader(deviceHandle, &waveHdr, sizeof(WAVEHDR));
			//waveOutClose(deviceHandle);
			//HeapFree(GetProcessHeap(), 0, memBlckPointer);

		}
		else
		{
			//printf("Header wasn't prepared...\n");
			//exit(1);
		}


	}
	return 0;
}
