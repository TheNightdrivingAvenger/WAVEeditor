/*МОДУЛЬ ДЛЯ РИСОВАНИЯ ЗВУКА
* СОЗДАЁТ БУФЕР, ОТКУДА ПРОЦЕДУРА РИСОВАНИЯ БЕРЁТ ЗНАЧЕНИЯ.
* РИСУЕТ ПО ЗНАЧЕНИЯМ ИЗ БУФЕРА.
* РИСУЕТ ВЫДЕЛЕНИЕ И ПОЛЗУНОК.
*/

#define UNICODE
#define _UNICODE
#include <Windows.h>
#include <mmreg.h>
#include <math.h>

#include "headers\drawingarea.h"
#include "headers\sounddrawer.h"

unsigned long calcMinMax8(WORD chanNum, int resArr[chanNum][2], void *source, unsigned long offset, int samplesNum);
unsigned long calcMinMax16(WORD chanNum, int resArr[chanNum][2], void *source, unsigned long offset, int samplesNum);

typedef unsigned long (*minMaxCalculator)(WORD, int [*][2], void *, unsigned long, int);

// USES: model->bitspersample; model->rgCurRange; drawingArea->size; model->samplesInBlock; model->wfxFormat
// model->minMaxCache; model->dataSize; model->cacheLength; model->soundData

BOOL recalcMinMax(PDRAWINGWINDATA pSelf)
{
	float minSampleVal = 0;
	
	minMaxCalculator calcMinMax;

	switch (pSelf->modelData->wfxFormat.wBitsPerSample)
	{
		case 8:
			calcMinMax = calcMinMax8;
			minSampleVal = 0;
			break;
		case 16:
			calcMinMax = calcMinMax16;
			minSampleVal = -32768;
			break;
		default:
			return FALSE; //invalid data
	}

	unsigned long totalSamples = pSelf->modelData->rgCurRange.nLastSample - pSelf->modelData->rgCurRange.nFirstSample;
	if (totalSamples == 0) return FALSE;

	float samplesToWidth = totalSamples / (float)pSelf->rcClientSize.right; 

	if (samplesToWidth >= 1) {
		float intPart;
		if (modff(samplesToWidth, &intPart) > 0) {
			pSelf->modelData->samplesInBlock = (int)truncf(intPart + 1);
		} else {
			pSelf->modelData->samplesInBlock = (int)roundf(intPart);
		}
	} else {
		pSelf->modelData->samplesInBlock = 1;
	}
	int delta; //how many samples were processed in min-max
	int resArr[pSelf->modelData->wfxFormat.nChannels][2];

	if (pSelf->modelData->minMaxChunksCache != NULL) {
		HeapFree(GetProcessHeap(), 0, pSelf->modelData->minMaxChunksCache);
	}
	totalSamples = pSelf->modelData->wfxFormat.nChannels * (pSelf->modelData->dataSize / pSelf->modelData->wfxFormat.nBlockAlign); // samples in all channels

																	  // 2 -- min and max
	if ((pSelf->modelData->minMaxChunksCache = HeapAlloc(GetProcessHeap(), 0, sizeof(int) * 2 * ((totalSamples / pSelf->modelData->samplesInBlock) + 1))) == NULL) {
		return FALSE;
	}

	int *cache = (int *)pSelf->modelData->minMaxChunksCache;

	pSelf->modelData->cacheLength = 0;
	for (unsigned long i = 0; (i + pSelf->modelData->samplesInBlock * pSelf->modelData->wfxFormat.nChannels) <= totalSamples; i += delta) {
		delta = calcMinMax(pSelf->modelData->wfxFormat.nChannels, resArr, pSelf->modelData->soundData, i, pSelf->modelData->samplesInBlock);

		for (int j = 0; j < pSelf->modelData->wfxFormat.nChannels; j++) {
			cache[j * 2] = resArr[j][0];
			cache[j * 2 + 1] = resArr[j][1];
		}
		cache += pSelf->modelData->wfxFormat.nChannels * 2;
		(pSelf->modelData->cacheLength)++;
	}
	return TRUE;
}


// USES: model->rgCurRange; pSelf->stepX; model->samplesInBlock; pSelf->rcClientSize; model->wfxFormat.nChannels
// model->minMaxCache; pSelf->backDC; pSelf->rcSelectedRange; pSelf-backBrush; pSelf->curPen; pSelf->rcClientSize
void drawRegion(PDRAWINGWINDATA pSelf)
{
	unsigned long totalSamples = pSelf->modelData->rgCurRange.nLastSample - pSelf->modelData->rgCurRange.nFirstSample;

	if (totalSamples == 0) return;

	pSelf->stepX = 1;
	if (pSelf->modelData->samplesInBlock == 1) {
		pSelf->stepX = pSelf->rcClientSize.right / totalSamples;
	}

	int zeroChannelLvl = (pSelf->rcClientSize.bottom - 1) / pSelf->modelData->wfxFormat.nChannels;

	unsigned long cachePos = pSelf->modelData->rgCurRange.nFirstSample / pSelf->modelData->samplesInBlock;
	unsigned long bufferStart = cachePos * pSelf->modelData->wfxFormat.nChannels * 2; //buffer offset

	int *cache = (int *)pSelf->modelData->minMaxChunksCache + bufferStart;

	int soundDepth = 2 << (pSelf->modelData->wfxFormat.wBitsPerSample - 1);
	int normalizedMin, normalizedMax;
	int tempMins[pSelf->modelData->wfxFormat.nChannels];

	for (int i = 0; i < pSelf->modelData->wfxFormat.nChannels; i++) {
		tempMins[i] = cache[i * 2] * zeroChannelLvl / soundDepth + zeroChannelLvl * (i + 1);// *2 because in memory it's [min][max][min][max]...
	}

	// 2 is a bad idea for marker, should be some constant
	BOOL isMarker = ((pSelf->rcSelectedRange.right - pSelf->rcSelectedRange.left) <= 2); // did user just set the marker or selected a range?
	if (!isMarker)
	{
		FillRect(pSelf->backDC, &pSelf->rcSelectedRange, pSelf->highlightBrush);
	}

	SelectObject(pSelf->backDC, pSelf->curPen);
	int curSample = pSelf->modelData->rgCurRange.nFirstSample;
	for (int xPos = 0; xPos < pSelf->rcClientSize.right 
			&& cachePos < pSelf->modelData->cacheLength; xPos += pSelf->stepX) {
		// drawing all channels in cycle
		for (int j = 0; j < pSelf->modelData->wfxFormat.nChannels; j++) {
			normalizedMin = cache[j * 2] * zeroChannelLvl / soundDepth + zeroChannelLvl * (j + 1);
			normalizedMax = cache[j * 2 + 1] * zeroChannelLvl / soundDepth + zeroChannelLvl * (j + 1);
			MoveToEx(pSelf->backDC, xPos, normalizedMin - 1, NULL);
			LineTo(pSelf->backDC, xPos, normalizedMax);
			LineTo(pSelf->backDC, xPos - pSelf->stepX, tempMins[j]);
			tempMins[j] = normalizedMin;
		}
		cache += pSelf->modelData->wfxFormat.nChannels * 2;
		curSample += pSelf->modelData->samplesInBlock;
		cachePos++;
	}
	pSelf->modelData->rgCurRange.nLastSample = pSelf->modelData->dataSize / pSelf->modelData->wfxFormat.nBlockAlign;
	if (isMarker) {
		SelectObject(pSelf->backDC, pSelf->borderPen);
		FillRect(pSelf->backDC, &pSelf->rcSelectedRange, pSelf->highlightBrush);
	}
}

/*
 store pairs of "MIN MAX" for every channel in *resArr (size of passed resArr must euqal to the number of channels in audio file)
 return number of samples processed (for ALL channels; that is, relative file offset in samples)
 samplesNum is for one channel
*/
unsigned long calcMinMax8(WORD chanNum, int resArr[chanNum][2], void *source, unsigned long offset, int samplesNum)
{
	const int maxLvl = 256;
	const int minLvl = 0;

	int totalSamples = 0;

	for (int i = 0; i < chanNum; i++) {
		int min = maxLvl, max = minLvl;
		int sampleCounter = 0;
	
		unsigned char *curPos = (unsigned char *)source + offset;
		for (curPos += i; sampleCounter < samplesNum; curPos += chanNum) {
			unsigned char temp = *curPos;
			if (temp < min) min = temp;
			if (temp > max) max = temp;
			sampleCounter++;
		}
		totalSamples += sampleCounter;
		resArr[i][0] = -min;
		resArr[i][1] = -max;
	}
	return totalSamples;
}

unsigned long calcMinMax16(WORD chanNum, int resArr[chanNum][2], void *source, unsigned long offset, int samplesNum)
{
	const int maxLvl = 32767;
	const int minLvl = -32768;

	int totalSamples = 0;

	for (int i = 0; i < chanNum; i++) {
		int min = maxLvl, max = minLvl;
		int sampleCounter = 0;

		short *curPos = (short *)source + offset;	
		for (curPos += i; sampleCounter < samplesNum; curPos += chanNum) {
			short temp = *curPos;
			if (temp < min) min = temp;
			if (temp > max) max = temp;
			sampleCounter++;
		}
		totalSamples += sampleCounter;
		resArr[i][0] = -(min + maxLvl + 1);
		resArr[i][1] = -(max + maxLvl + 1);
	}
	return totalSamples;
}
