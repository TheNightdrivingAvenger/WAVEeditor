#define UNICODE
#define _UNICODE
#include <Windows.h>

#include "headers\fileworker.h"

CHUNK getChunk(HANDLE hFile)
{
	CHUNK result;
	DWORD readres;
	if ((ReadFile(hFile, result.chunkID, CHUNKIDLENGTH, &readres, NULL) == 0) || (readres < CHUNKIDLENGTH)) {
		result.chunkID[0] = '\0';
		return result;
	}

	if ((ReadFile(hFile, &result.chunkSize, SIZEFIELDLENGTHBYTES, &readres, NULL) == 0) || (readres < SIZEFIELDLENGTHBYTES))
	{
		result.chunkID[0] = '\0';
		return result;
	}

	result.chunkID[CHUNKIDLENGTH] = '\0';

	return result;
}
