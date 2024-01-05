/*
 * history.h  -  definitions for history.c
 * 21.02.90
 */
 
#ifndef _M_HISTORY
#define _M_HISTORY

void readhistory(int maxhist, char **hbuf, int *maxhp);
void writehistory(int maxhist, char **hbuf);

#endif