/*
 * getopt.h - definitions for getopt.c
 * 16/03/89
 */

#ifndef _M_GETOPT
#define _M_GETOPT
 
extern char *optarg;
extern int optind;

void optinit(void);
int getopt(int nargc, char **nargv, char *ostr);

#endif