/*
 * exec.h - definitions for exec.c
 * 28.09.89
 */

#ifndef _M_EXEC
#define _M_EXEC

extern int venuscmd;

int execcmd(char *cmdpath,ARGCV);	/* exec external command */

#endif