#pragma once

#define BYTESINGiB 1073741824

#define CHUNKIDLENGTH 4
#define SIZEFIELDLENGTHBYTES 4

typedef struct tagCHUNK {
	char chunkID[CHUNKIDLENGTH + 1];
	unsigned long chunkSize;
} CHUNK, *PCHUNK;

// returns \0 in chunk.chunkID[0] if error during reading occurs
CHUNK getChunk(HANDLE file);
