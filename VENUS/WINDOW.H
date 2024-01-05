/*
 * @(#) Gemini\Window.h
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: Header File for window.c
 *
 */

#ifndef __window__
#define __window__

#define NORMWIND	(NAME|INFO|CLOSER|FULLER|SIZER|MOVER|VSLIDE| \
						UPARROW|DNARROW)
#define MUPFWIND	(NAME|CLOSER|MOVER|FULLER)

WindInfo *openWindow(word wx, word wy, word ww, word wh, word slpos,
			char *path, char *wcard, char *title, char *label,
			word kind);
void closeWindow(word whandle, word goToParent);
void deleteWindow(word whandle);
void calcWindData(WindInfo *wp);
void setWindData(WindInfo *wp);
void UpdateWindowData(void);
void moveWindow(word whandle,word r[4]);
void sizeWindow(word whandle,word r[4]);		/* r maybe changed */
void fullWindow(word whandle);
void closeAllWind(void);
void DoTopWindow (word whandle);

WindInfo *newwp(void);
WindInfo *getwp(word whandle);
word freewp(word whandle);

void setWpOnTop(word whandle);
void cycleWindow(void);
void delAccWindows(void);

WindInfo *getMupfWp(void);
word openMupfelWindow(void);

void WindNewLabel(char drive, const char *oldlabel, 
						const char *newlabel);

void SetTOPWIND(void);
void SetTopWindowInfo(word topwindow);

#endif