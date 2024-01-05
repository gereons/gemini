/*
 * mvwindow.c
 *
 * project: mvmerge
 *
 * author: Stefan Eissing & Gereon Steffens
 *
 * description: top layer of functions for mupfel window
 *
 * last change: 22.04.1990
 */

#define INV_INV		0
#define INV_UDL		1
#define INV_BLD		2

word openMWindow(WindInfo *wp);
word moveMWindow(WindInfo *wp);
word closeMWindow(WindInfo *wp);
void drawMWindow(WindInfo *wp, word r1[4]);
word redrawMWindow(WindInfo *wp,word r1[4]);
void SizeOfMWindow(word *wx,word *wy,word *ww,word *wh);
void FullMWindow(WindInfo *wp);
word fontDialog(void);

word MWindInit(void);
void MWindExit(void);

void getInMWindow(ShowInfo *ps);
void setInMWindow(ShowInfo *ps);

void setBackDestr(int flag);

void GetConsoleFont(word *id, word *points);
