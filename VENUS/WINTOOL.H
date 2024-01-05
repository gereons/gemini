/*
 * wintool.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: functions for windows
 *
 * last change: 11.09.1990
 */


#ifndef __wintool__
#define __wintool__

void WT_Clip(word defhandle, word handle2, word handle3, word pxy[4]);

void WT_BuildRectList(word whandle);

char WT_GetRect(word index, word pxy[4]);

#endif