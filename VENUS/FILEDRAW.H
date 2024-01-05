/*
 * filedraw.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: functions to redraw text in file windows
 *
 * last change: 16.08.1990
 */

#ifndef __filedraw__
#define __filedraw__

#include <flydial\flydial.h>


extern word ff_wchar, ff_hchar;

word cdecl drawFileText(PARMBLK *pb);
void optFileDraw(WindInfo *wp,word r1[4]);

word doShowOptions(void);

void initFileFonts(void);
void exitFileFonts(void);
void switchFont(void);

word writeFontInfo(word fhandle, char *buffer);
void execFontInfo(char *line);

void GetFileFont(word *id, word *points);

word SetFont(FONTWORK *fw, word *id, word *points,
					word *width, word *height);

#endif