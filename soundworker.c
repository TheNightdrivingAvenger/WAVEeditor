/*ВСЯ РАБОТА СО ЗВУКОМ
* ЗАПОЛНЕНИЕ ТИШИНОЙ
* КОПИРОВАНИЕ
* УДАЛЕНИЕ
* ВСТАВКА
* РЕВЕРС
*/

#include <Windows.h>
#include <string.h>
#include "headers\drawingarea.h"
#include "headers\soundworker.h"

// TODO: silence is not always 0, fix for different formats
void makeSilent(PMODELDATA pModelData)
{
	unsigned long bufferStart = pModelData->rgSelectedRange.nFirstSample * pModelData->wfxFormat.nBlockAlign;
	ZeroMemory((char *)pModelData->soundData + bufferStart, 
		(pModelData->rgSelectedRange.nLastSample - pModelData->rgSelectedRange.nFirstSample + 1) * pModelData->wfxFormat.nBlockAlign);
}

// copying is inclusive on borders
void pastePiece(PMODELDATA pModelData)
{
	DWORD copiedBlockSize = (pModelData->rgCopyRange.nLastSample - pModelData->rgCopyRange.nFirstSample + 1) *
																											pModelData->wfxFormat.nBlockAlign;

	pModelData->soundData = HeapReAlloc(GetProcessHeap(), 0, pModelData->soundData, pModelData->dataSize + copiedBlockSize);

	// paste in place of the first selected sample (so move the first selected)
	DWORD pastePos = pModelData->rgSelectedRange.nFirstSample * pModelData->wfxFormat.nBlockAlign;
	char *movingPiece = (char *)pModelData->soundData + pastePos;

	char *copiedPiece = (char *)pModelData->soundData + pModelData->rgCopyRange.nFirstSample * pModelData->wfxFormat.nBlockAlign;
	
	char *temp = HeapAlloc(GetProcessHeap(), 0, copiedBlockSize);
	memcpy(temp, copiedPiece, copiedBlockSize);

	memmove((char *)pModelData->soundData + pastePos + copiedBlockSize, (char *)pModelData->soundData + pastePos, pModelData->dataSize - pastePos);
	memcpy(movingPiece, temp, copiedBlockSize);

	HeapFree(GetProcessHeap(), 0, temp);

	pModelData->dataSize += copiedBlockSize;

	DWORD copiedBlockSampleSize = pModelData->rgCopyRange.nLastSample - pModelData->rgCopyRange.nFirstSample + 1;
	if (pModelData->rgSelectedRange.nFirstSample <= pModelData->rgCopyRange.nFirstSample) {
		pModelData->rgCopyRange.nFirstSample += copiedBlockSampleSize;
		pModelData->rgCopyRange.nLastSample += copiedBlockSampleSize;
		pModelData->rgSelectedRange.nFirstSample += copiedBlockSampleSize;
		pModelData->rgSelectedRange.nLastSample += copiedBlockSampleSize;
	}
}

// deleting is inclusive on borders
void deletePiece(PMODELDATA pModelData)
{
	DWORD cutBlockSize = (pModelData->rgSelectedRange.nLastSample - pModelData->rgSelectedRange.nFirstSample + 1) * 
																											pModelData->wfxFormat.nBlockAlign;
	DWORD pastePos = pModelData->rgSelectedRange.nFirstSample * pModelData->wfxFormat.nBlockAlign;

	// address of the piece that needs to get moved for shrinking
	char *movingPiece = (char *)pModelData->soundData + pastePos + cutBlockSize;

	memmove((char *)pModelData->soundData + pastePos, movingPiece, pModelData->dataSize - pastePos - cutBlockSize);
	pModelData->soundData = HeapReAlloc(GetProcessHeap(), 0, pModelData->soundData, pModelData->dataSize - cutBlockSize);

	pModelData->dataSize -= cutBlockSize;
}

void reversePiece(PMODELDATA pModelData)
{
	unsigned long blockSize = (pModelData->rgSelectedRange.nLastSample - pModelData->rgSelectedRange.nFirstSample + 1) * pModelData->wfxFormat.nBlockAlign;
	
	// TODO: watch out for possible overlapping
	unsigned long bufferStart = pModelData->rgSelectedRange.nFirstSample * pModelData->wfxFormat.nBlockAlign;

	char *curPiece = (char *)pModelData->soundData + bufferStart;

	char temp[pModelData->wfxFormat.nBlockAlign];

	for (unsigned long i = 0; i <= blockSize / 2; i += pModelData->wfxFormat.nBlockAlign) {
		memcpy(temp, curPiece + i, pModelData->wfxFormat.nBlockAlign);
		memcpy(curPiece + i, curPiece + blockSize - i - pModelData->wfxFormat.nBlockAlign, pModelData->wfxFormat.nBlockAlign);
		memcpy(curPiece + blockSize - i, temp, pModelData->wfxFormat.nBlockAlign);
	}
}
