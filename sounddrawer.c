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

BOOL recalcMinMax(PDRAWINGWINDATA windowProps, void *soundData, int dataSize, PWAVEFORMATEX wfxFormat)
{
	float minSampleVal = 0;
	
	minMaxCalculator calcMinMax;

	switch (wfxFormat->wBitsPerSample)
	{
		case 8:
			calcMinMax = &calcMinMax8;
			minSampleVal = 0;
			break;
		case 16:
			calcMinMax = &calcMinMax16;
			minSampleVal = -32768;
			break;
		default:
			return FALSE; //invalid data
	}

	// total displayed samples
	unsigned long totalSamples = windowProps->rgCurDisplayedRange.nLastSample - windowProps->rgCurDisplayedRange.nFirstSample;
	if (totalSamples == 0) return FALSE;

	float samplesToWidth = totalSamples / ((float)windowProps->rcClientSize.right - SCREEN_DELTA_RIGHT);

	if (samplesToWidth >= 1) {
		float intPart;
		if (modff(samplesToWidth, &intPart) > 0) {
			windowProps->samplesInBlock = (int)truncf(intPart + 1);
		} else {
			windowProps->samplesInBlock = (int)roundf(intPart);
		}
	} else {
		windowProps->samplesInBlock = 1;
	}
	int delta; //how many samples were processed in min-max
	int resArr[wfxFormat->nChannels][2];

	if (windowProps->minMaxChunksCache != NULL) {
		HeapFree(GetProcessHeap(), 0, windowProps->minMaxChunksCache);
	}
	windowProps->lastSample = dataSize / wfxFormat->nBlockAlign - 1;
	// samples in all channels
	totalSamples = wfxFormat->nChannels * (windowProps->lastSample + 1);

																	  // 2 -- min and max
	if ((windowProps->minMaxChunksCache = HeapAlloc(GetProcessHeap(), 0, sizeof(int) * 2 * ((totalSamples / windowProps->samplesInBlock) + 1))) == NULL) {
		return FALSE;
	}

	int *cache = (int *)windowProps->minMaxChunksCache;

	windowProps->cacheLength = 0;
	for (unsigned long i = 0; (i + windowProps->samplesInBlock * wfxFormat->nChannels) <= totalSamples; i += delta) {
		delta = calcMinMax(wfxFormat->nChannels, resArr, soundData, i, windowProps->samplesInBlock);

		for (int j = 0; j < wfxFormat->nChannels; j++) {
			cache[j * 2] = resArr[j][0];
			cache[j * 2 + 1] = resArr[j][1];
		}
		cache += wfxFormat->nChannels * 2; // * 2 -- min and max
		(windowProps->cacheLength)++;
	}
	return TRUE;
}

void drawRegion(PDRAWINGWINDATA windowProps, PWAVEFORMATEX wfxFormat)
{
	unsigned long totalSamples = windowProps->rgCurDisplayedRange.nLastSample - windowProps->rgCurDisplayedRange.nFirstSample;

	if (totalSamples == 0) return;

	windowProps->stepX = 1;
	if (windowProps->samplesInBlock == 1) {
		windowProps->stepX = (windowProps->rcClientSize.right - SCREEN_DELTA_RIGHT) / totalSamples;
	}

	int zeroChannelLvl = (windowProps->rcClientSize.bottom - 1) / wfxFormat->nChannels;

	// integer division -- truncation towards zero. We take the block in which the first sample is
	unsigned long cachePos = windowProps->rgCurDisplayedRange.nFirstSample / windowProps->samplesInBlock;
	unsigned long bufferStart = cachePos * wfxFormat->nChannels * 2; //min max cache offset

	int *cache = (int *)windowProps->minMaxChunksCache + bufferStart;

	int soundDepth = 2 << (wfxFormat->wBitsPerSample - 1);
	int normalizedMin, normalizedMax;
	int tempMins[wfxFormat->nChannels];

	for (int i = 0; i < wfxFormat->nChannels; i++) {
		tempMins[i] = cache[i * 2] * zeroChannelLvl / soundDepth + zeroChannelLvl * (i + 1);// *2 because in memory it's [min][max][min][max]...
	}

	// did user just set the marker or selected a range?
	BOOL isMarker = ((windowProps->rcSelectedRange.right - windowProps->rcSelectedRange.left) <= CURSOR_THICKNESS);
	if (!isMarker)
	{
		FillRect(windowProps->backDC, &windowProps->rcSelectedRange, windowProps->highlightBrush);
	}

	SelectObject(windowProps->backDC, windowProps->curPen);
	int xPos;
	// drawing until we hit the end of the cache or window border
	for (xPos = 0; xPos < (windowProps->rcClientSize.right - SCREEN_DELTA_RIGHT)
			&& cachePos < windowProps->cacheLength; xPos += windowProps->stepX) {
		// drawing all channels in cycle
		for (int j = 0; j < wfxFormat->nChannels; j++) {
			normalizedMin = cache[j * 2] * zeroChannelLvl / soundDepth + zeroChannelLvl * (j + 1);
			normalizedMax = cache[j * 2 + 1] * zeroChannelLvl / soundDepth + zeroChannelLvl * (j + 1);
			MoveToEx(windowProps->backDC, xPos, normalizedMin - 1, NULL);
			LineTo(windowProps->backDC, xPos, normalizedMax);
			LineTo(windowProps->backDC, xPos - windowProps->stepX, tempMins[j]);
			tempMins[j] = normalizedMin;
		}
		cache += wfxFormat->nChannels * 2;
		cachePos++;
	}
	windowProps->lastUsedPixelX = xPos - windowProps->stepX;
	if (isMarker) {
		SelectObject(windowProps->backDC, windowProps->borderPen);
		FillRect(windowProps->backDC, &windowProps->rcSelectedRange, windowProps->highlightBrush);
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
		// TODO: should it be +=i on the first iteration?
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
