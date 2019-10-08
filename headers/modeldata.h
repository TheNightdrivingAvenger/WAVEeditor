#pragma once
#include "headers\player.h"

typedef long sampleIndex;

typedef struct tagSAMPLERANGE {
	sampleIndex nFirstSample;
	sampleIndex nLastSample;
} SAMPLERANGE, *PSAMPLERANGE;

// TODO: add "close file" in menu in here?
enum changeReasonsFlags { soundDataChange = 2 /* requires ACTIONINFO structure*/, curFileNameChange = 4 /* change window text */,
						selectionChange = 16,
						newFileOpened = 64, playbackStop = 128, zoomingIn = 256, zoomingOut = 512, fittingInWindow = 1024,
						activeRangeChange = 2048 };

enum zoomingLevelType { zoomNone, zoomIn, zoomOut, fitInWindow };

enum editActions { eaNone /*none is used for "change active range" message for now, may change later*/,
				eaDelete, eaPaste, eaMakeSilent, eaReverse };

typedef struct tagEDITACTION {
	enum editActions action;
	SAMPLERANGE rgRange;
} ACTIONINFO, *PACTIONINFO;

struct tagMAINWINDOWDATA;

typedef struct tagMODELDATA {
	struct tagMAINWINDOWDATA *mainView;	// link to a main window, which is the main (dispatching) view
	void *soundData;
	DWORD dataSize;
	WAVEFORMATEX wfxFormat;
	HANDLE curFile;
	SAMPLERANGE rgSelectedRange;
	SAMPLERANGE rgCopyRange;
	BOOL isChanged;
} MODELDATA, *PMODELDATA;

typedef struct tagUPDATEINFO {
	void *soundData;
	int dataSize;
	PWAVEFORMATEX wfxFormat;
} UPDATEINFO, *PUPDATEINFO;

void Model_AttachView(PMODELDATA pSelf, struct tagMAINWINDOWDATA *view);
void Model_Reset(PMODELDATA pSelf);
void Model_ChangeCurFile(PMODELDATA pSelf, const wchar_t *const chosenFile);
void Model_SaveFileAs(PMODELDATA pSelf, BOOL saveSelected, wchar_t *const chosenFile);
void Model_UpdateActiveRange(PMODELDATA pSelf, PSAMPLERANGE newRange);
void Model_CopySelected(PMODELDATA pSelf);
void Model_DeletePiece(PMODELDATA pSelf);
void Model_PastePiece(PMODELDATA pSelf);
void Model_MakeSilent(PMODELDATA pSelf);
void Model_SelectAll(PMODELDATA pSelf);
void Model_Reverse(PMODELDATA pSelf);
void Model_UpdateSelection(PMODELDATA pSelf, BOOL updateOnlyLastSample, PSAMPLERANGE range);
void Model_UpdatePlayerStatus(PMODELDATA pSelf, PlayerState status);
void Model_ZoomLevelChange(PMODELDATA pSelf, enum zoomingLevelType zoomingType);
void Model_SaveFile(PMODELDATA pSelf);
BOOL Model_IsChanged(PMODELDATA pSelf);
