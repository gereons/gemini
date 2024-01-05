/*
 * rmmkdir.h - definitions for rmmkdir.c
 * 13.12.89
 */

#ifndef _M_RMMKDIR
#define _M_RMMKDIR
 
int mkdir(char *dir);
int rmdir(char *dir);
int remove(const char *file);

#endif