/*
 * cmdline.h - definitions for cmdline.h
 * 02.10.89
 */

#ifndef _M_CMDLINE
#define _M_CMDLINE

void appendline(char *line);
void pushline(char *line);

char *getline(int gemini);			/* get one line */
void setcmdline(int argc,char **argv);

extern int docrlf;

#endif