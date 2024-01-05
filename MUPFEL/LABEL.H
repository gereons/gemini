/*
 * label.h  -  definitions for label.c
 * 07.03.90
 */
 
#ifndef _M_LABEL
#define _M_LABEL

char *getlabel(char drv);
int setlabel(char drv, char *newlabel, const char *cmd);

#endif