#pragma once

// SM -- service message
#define SM_SETINPUTFILE (WM_USER + 0)
#define SM_SETCACHEFILE (WM_USER + 1)
#define SM_SETBUFFER	(WM_USER + 2)
#define SM_SAVEFILE		(WM_USER + 3)
#define SM_SAVEFILEAS 	(WM_USER + 4)
#define SM_SETMODELDATA	(WM_USER + 5)

// MAF -- menu action on the file
//#define MAF_COPY			(WM_USER + 10)
//#define MAF_DELETE			(WM_USER + 11)
//#define MAF_PASTE			(WM_USER + 12)
//#define MAF_SELECTALL		(WM_USER + 13)
//#define MAF_MAKESILENT		(WM_USER + 14)
//#define MAF_SAVESELECTED	(WM_USER + 15)
//#define MAF_REVERSESELECTED	(WM_USER + 16)
//#define MAF_ISCHANGED		(WM_USER + 17)

// UPD -- update message
#define UPD_CURSOR			(WM_USER + 10)	// lParam contains X position of the cursor. Selection is discarded
#define UPD_SELECTION		(WM_USER + 11)	// lParam contains Xbegin, Xend with selection coordinates (as in lParam of WM_LBUTTONDOWN)
#define UPD_SELECTALL		(WM_USER + 12)	// tells the child window to select all the file
#define UPD_CACHE			(WM_USER + 13)	// lParam contains pointer to the update structure. Called code frees the memory
#define UPD_SELECTEDRANGE	(WM_USER + 14)	// wParam contains a BOOL flag: FALSE -- first sample chosen, TRUE -- last sample chosen;
											//lParam contains pointer to the RANGE struct with samples. Called code frees the memory
