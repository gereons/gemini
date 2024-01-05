/*
 * init.h  -  definitions for init.c
 * 01.12.90
 */
 
#ifndef _M_INIT
#define _M_INIT

int cleartrack(int trk, char *trkbuf, int drv, int sides, int sectors);
int writebootsector(char *trkbuf, int drv, int sides, int sectors, int tracks);

#endif