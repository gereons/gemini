/*
 * redir.h - definitions for redir.c
 * 16.10.89
 */

#ifndef _M_REDIR
#define _M_REDIR

typedef struct
{
	int hnd;
	int ohnd;
	int append;
	int clobber;
	char *file;
} redirinfo;

typedef struct
{
	redirinfo	in;
	redirinfo out;
	redirinfo aux;
} redirection;

extern redirection redirect;

void redirinit(void);
int doredir(char *cmd);
void endredir(int delete);

#endif