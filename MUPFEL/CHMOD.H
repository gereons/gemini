/*
 * chmod.h  -  definitions for chmod.c
 * 13.02.90
 */
 
#ifndef _M_CHMOD
#define _M_CHMOD

void setwritemode(const char *filename,int mode);
int docheckfast(const char *file, const char *cmd, int printit, int TTflags);

#endif