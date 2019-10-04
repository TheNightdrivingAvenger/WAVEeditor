#pragma once

#include "headers\modeldata.h"
BOOL recalcMinMax(PDRAWINGWINDATA windowProps, void *soundData, int dataSize, PWAVEFORMATEX wfxFormat, enum zoomingLevelType zoomingType);
void drawRegion(PDRAWINGWINDATA windowProps, PWAVEFORMATEX wfxFormat);
