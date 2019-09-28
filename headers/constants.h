#pragma once

// SM -- service message
//#define SM_SETINPUTFILE (WM_USER + 0)
//#define SM_SETCACHEFILE (WM_USER + 1)
//#define SM_SETBUFFER	(WM_USER + 2)
//#define SM_SAVEFILE		(WM_USER + 3)
//#define SM_SAVEFILEAS 	(WM_USER + 4)
//#define SM_SETMODELDATA	(WM_USER + 5)

// UPD -- update message
//#define UPD_CURSOR			(WM_USER + 10)	// lParam contains X position of the cursor. Selection is discarded
//#define UPD_SELECTION		(WM_USER + 11)	// wParam contains a BOOL flag: FALSE -- use values from lParam, TRUE -- select all the file, ignore lParam
											//lParam contains Xbegin, Xend with selection coordinates (as X, Y in lParam of WM_LBUTTONDOWN)
