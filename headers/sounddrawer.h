#pragma once

//possibly a file handle (file with cached blocks); zoomLvl can be: 1, 2, 4, 8, 16, 32; 1/2, 1/4, 1/8, 1/16, 1/32
BOOL recalcMinMax(PDRAWINGWINDATA windowProps, void *soundData, int dataSize, PWAVEFORMATEX wfxFormat);
void drawRegion(PDRAWINGWINDATA windowProps, PWAVEFORMATEX wfxFormat);
