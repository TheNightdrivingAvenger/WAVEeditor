#pragma once

// SM -- service message
#define SM_SETINPUTFILE (WM_USER + 0)
#define SM_SETCACHEFILE (WM_USER + 1)
#define SM_SETBUFFER	(WM_USER + 2)
#define SM_SAVEFILE		(WM_USER + 3)
#define SM_SAVEFILEAS 	(WM_USER + 4)
#define SM_SETMODELDATA	(WM_USER + 5)

// UPD -- update message
#define UPD_CURSOR			(WM_USER + 10)	// lParam contains X position of the cursor. Selection is discarded
#define UPD_SELECTION		(WM_USER + 11)	// wParam contains a BOOL flag: FALSE -- use values from lParam, TRUE -- select all the file, ignore lParam
											//lParam contains Xbegin, Xend with selection coordinates (as X, Y in lParam of WM_LBUTTONDOWN)
#define UPD_CACHE			(WM_USER + 12)	// lParam contains pointer to the update structure. Called code frees the memory
#define UPD_SELECTEDRANGE	(WM_USER + 13)	// wParam contains a BOOL flag: FALSE -- first sample chosen, TRUE -- last sample chosen;
											//lParam contains pointer to the RANGE struct with samples. Called code frees the memory
#define UPD_DISPLAYEDRANGE	(WM_USER + 14)	// lParam contains pointer to the RANGE struct with displayed range. Called code frees the memory
#define UPD_PLAYERSTATUS	(WM_USER + 15)	// wParam contains PlayerState enum value
