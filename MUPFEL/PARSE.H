/*
 * parse.h - definitions for parse.c
 * 05.02.91
 */

#ifndef _M_PARSE
#define _M_PARSE

extern char cmdpath[];
extern int icmd, iserrfile;

typedef enum {EXEC_NOTFOUND, EXEC_BINARY, EXEC_BATCH} exectype;
exectype canexec(const char *cmd);

int parse(int argc,char **argv);
int extinsuffix(const char *cmd);
int findintern(const char *cmd);
int allupper(const char *str);
int locatecmd(char *cmd, int setbatch, int *cost);

#endif