#pragma once

// SM -- service message
#define SM_SETINPUTFILE (WM_USER + 0)
#define SM_SETCACHEFILE (WM_USER + 1)
#define SM_SETBUFFER	(WM_USER + 2)
#define SM_SAVEFILE		(WM_USER + 3)
#define SM_SAVEFILEAS 	(WM_USER + 4)
#define SM_SETMODELDATA	(WM_USER + 5)

// MAF -- menu action on the file
#define MAF_COPY			(WM_USER + 10)
#define MAF_DELETE			(WM_USER + 11)
#define MAF_PASTE			(WM_USER + 12)
#define MAF_SELECTALL		(WM_USER + 13)
#define MAF_MAKESILENT		(WM_USER + 14)
#define MAF_SAVESELECTED	(WM_USER + 15)
#define MAF_REVERSESELECTED	(WM_USER + 16)
#define MAF_ISCHANGED		(WM_USER + 17)
