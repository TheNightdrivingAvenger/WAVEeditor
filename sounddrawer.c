#define UNICODE
#define _UNICODE
#include <Windows.h>
#include <math.h>

#include "headers\drawingarea.h"
#include "headers\sounddrawer.h"
#include "headers\modeldata.h"

unsigned long calcMinMax8(WORD chanNum, int resArr[chanNum][2], void *source, unsigned long offset, int samplesNum);
unsigned long calcMinMax16(WORD chanNum, int resArr[chanNum][2], void *source, unsigned long offset, int samplesNum);

typedef unsigned long (*minMaxCalculator)(WORD, int [*][2], void *, unsigned long, int);

BOOL recalcMinMax(PDRAWINGWINDATA windowProps, void *soundData, int dataSize, PWAVEFORMATEX wfxFormat, enum zoomingLevelType zoomingType)
{
	float minSampleVal = 0;
	
	minMaxCalculator calcMinMax;

	switch (wfxFormat->wBitsPerSample)
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

	// total displayed samples
	sampleIndex totalSamples = windowProps->rgCurDisplayedRange.nLastSample - windowProps->rgCurDisplayedRange.nFirstSample + 1;
	if (totalSamples == 0) return FALSE;

	if (zoomingType != zoomNone) {
		float samplesToWidth = totalSamples / ((float)windowProps->rcClientSize.right - 1 - SCREEN_DELTA_RIGHT);

		if (samplesToWidth >= 1) {
			float intPart;
			if ((modff(samplesToWidth, &intPart) > 0) && (zoomingType == fitInWindow)) {
				windowProps->samplesInBlock = (int)truncf(intPart + 1);
			} else {
				windowProps->samplesInBlock = (int)truncf(intPart);
			}
		} else {
			windowProps->samplesInBlock = 1;
		}

		windowProps->stepX = 1;
		if (windowProps->samplesInBlock == 1) {
			if (zoomingType == fitInWindow) {
				windowProps->stepX = max(1, (windowProps->rcClientSize.right - 1 - SCREEN_DELTA_RIGHT) / totalSamples);
			} else {
				windowProps->stepX = max(1, (int)roundf((windowProps->rcClientSize.right - 1 - SCREEN_DELTA_RIGHT) / (float)totalSamples));
			}
		}
	}
	int delta; //how many samples were processed in min-max
	int resArr[wfxFormat->nChannels][2];

	if (windowProps->minMaxChunksCache != NULL) {
		HeapFree(GetProcessHeap(), 0, windowProps->minMaxChunksCache);
	}
	windowProps->lastSample = dataSize / wfxFormat->nBlockAlign - 1;
	// samples in all channels
	sampleIndex totalFileSamples = wfxFormat->nChannels * (windowProps->lastSample + 1);

																	  // 2 -- min and max
	if ((windowProps->minMaxChunksCache = HeapAlloc(GetProcessHeap(), 0, sizeof(int) * 2 * ((totalFileSamples / windowProps->samplesInBlock) + 1))) == NULL) {
		return FALSE;
	}

	int *cache = (int *)windowProps->minMaxChunksCache;

	windowProps->cacheLength = 0;
	sampleIndex i;
	for (i = 0; (i + windowProps->samplesInBlock * wfxFormat->nChannels) <= totalFileSamples; i += delta) {
		delta = calcMinMax(wfxFormat->nChannels, resArr, soundData, i, windowProps->samplesInBlock);

		for (int j = 0; j < wfxFormat->nChannels; j++) {
			cache[j * 2] = resArr[j][0];
			cache[j * 2 + 1] = resArr[j][1];
		}
		cache += wfxFormat->nChannels * 2; // * 2 -- min and max
		(windowProps->cacheLength)++;
	}
	if (i < totalFileSamples) {
		i += calcMinMax(wfxFormat->nChannels, resArr, soundData, i, (totalFileSamples - i) / wfxFormat->nChannels);
		for (int j = 0; j < wfxFormat->nChannels; j++) {
			cache[j * 2] = resArr[j][0];
			cache[j * 2 + 1] = resArr[j][1];
		}
		(windowProps->cacheLength)++;
	}
	return TRUE;//i - delta;//windowProps->samplesInBlock * wfxFormat->nChannels;
}

void drawRegion(PDRAWINGWINDATA windowProps, PWAVEFORMATEX wfxFormat)
{
	sampleIndex totalSamples = windowProps->rgCurDisplayedRange.nLastSample - windowProps->rgCurDisplayedRange.nFirstSample + 1;

	if (totalSamples == 0) return;

	int zeroChannelLvl = (windowProps->rcClientSize.bottom - 1) / wfxFormat->nChannels;

	// integer division -- truncation towards zero. We take the block in which the first sample is
	long cachePos = windowProps->rgCurDisplayedRange.nFirstSample / windowProps->samplesInBlock;
	unsigned long bufferStart = cachePos * wfxFormat->nChannels * 2; // min max cache offset

	int *cache = (int *)windowProps->minMaxChunksCache + bufferStart;

	int soundDepth = 2 << (wfxFormat->wBitsPerSample - 1);
	int normalizedMin, normalizedMax;
	int tempMins[wfxFormat->nChannels];

	for (int i = 0; i < wfxFormat->nChannels; i++) {
		tempMins[i] = cache[i * 2] * zeroChannelLvl / soundDepth + zeroChannelLvl * (i + 1);// *2 because in memory it's [min][max][min][max]...
	}

	// did user just set the marker or selected a range?
	//if (windowProps->rcSelectedRange.left >= 0) TODO: add hidden cursor (-1 on selected range)
	BOOL isVisible = (windowProps->rcSelectedRange.left >= 0);
	BOOL isMarker = ((windowProps->rcSelectedRange.right - windowProps->rcSelectedRange.left) <= CURSOR_THICKNESS);
	if (isVisible && !isMarker)
	{
		FillRect(windowProps->backDC, &windowProps->rcSelectedRange, windowProps->highlightBrush);
	}

	SelectObject(windowProps->backDC, windowProps->curPen);
	int xPos;
	// drawing until we hit the end of the cache or window border
	for (xPos = 0; xPos <= (windowProps->rcClientSize.right - 1 - SCREEN_DELTA_RIGHT)
			&& cachePos <= windowProps->cacheLength - 1; xPos += windowProps->stepX) {
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
	// min because there could be less samples in the last block than in others and we can go out of buffer's bounds
	windowProps->rgCurDisplayedRange.nLastSample = min(windowProps->lastSample, cachePos * windowProps->samplesInBlock - 1);

	windowProps->lastUsedPixelX = xPos - windowProps->stepX;
	if (isVisible && isMarker) {
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
