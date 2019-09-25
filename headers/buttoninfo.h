#pragma once

// interface-object-like structure with something like virtual methods. Allows to work with all buttons in the same way
typedef struct tagBUTTONINFO {
	RECT buttonPos;
	HPEN pen;
	HBRUSH brush;
	void (*Button_Draw)(struct tagBUTTONINFO *, HDC);
	void (*Button_OnClick)(void);

} BUTTONINFO, *PBUTTONINFO;

typedef void (*Button_DrawAction)(PBUTTONINFO, HDC);
typedef void (*Button_OnClickAction)(void);
