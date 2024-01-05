/*
 * exit.h - definitions for exit.c
 * 20.01.90
 */

#ifndef _M_EXIT
#define _M_EXIT
 
void fatal(char *fmt,...);		/* terminate with error msg */
void terminate(int verbose);

#endif