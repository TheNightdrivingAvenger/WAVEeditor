#pragma once

typedef struct {
	unsigned long nFirstSample;
	unsigned long nLastSample;
} RANGE, *PRANGE;

typedef struct tagMODELDATA {
	void *soundData;
	DWORD dataSize;
	WAVEFORMATEX wfxFormat;
	HANDLE curFile;
	void *minMaxChunksCache;	// stores in int; [min][max]
	unsigned long cacheLength;	// length of the cache. min-max pairs (for every channel) = 1 length unit
	int samplesInBlock;			// samples in block for one channel
	RANGE rgCurRange;
	RANGE rgSelectedRange;
	RANGE rgCopyRange;
	BOOL isChanged;
} MODELDATA, *PMODELDATA;
