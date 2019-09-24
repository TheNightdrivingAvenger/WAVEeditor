#pragma once

typedef struct {
	unsigned long nFirstSample;
	unsigned long nLastSample;
} RANGE, *PRANGE;

typedef enum { playing, paused, stopped } playerState;

typedef struct tagMODELDATA {
	void *soundData;
	DWORD dataSize;
	WAVEFORMATEX wfxFormat;
	HANDLE curFile;
	RANGE rgSelectedRange;
	RANGE rgCopyRange;
	BOOL isChanged;
	playerState playerState;
} MODELDATA, *PMODELDATA;
