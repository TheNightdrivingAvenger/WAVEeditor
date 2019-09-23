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
	void *minMaxChunksCache;	// stores in int; [ [min][max]1st channel ... [min][max]nth channel ][ [min][max]1st channel ... [min][max]nth channel ]
	unsigned long cacheLength;	// length of the cache. min-max pairs (for every channel) = 1 length unit
	int samplesInBlock;			// samples in block for one channel
	RANGE rgCurRange;
	RANGE rgSelectedRange;
	RANGE rgCopyRange;
	BOOL isChanged;
	playerState psPlayerState;
} MODELDATA, *PMODELDATA;
