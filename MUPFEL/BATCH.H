/*
 * batch.h - definitions for batch.c
 * 25.09.90
 */

#ifndef _M_BATCH
#define _M_BATCH

#include "mupfel.h"
#include "redir.h"

typedef struct
{
	int argc;
	char **argv;
	int autoexec;
	char *buffer;
	char *bufp;
	char *dir;
	char drv;
	redirinfo in,out,aux;
} batch;

typedef struct cmdlin
{
	struct cmdlin *next;
	char *line;
} cmdline;

typedef struct linsrc
{
	struct linsrc *next;
	struct linsrc *prev;
	cmdline *linestack;
	size_t pushedline;
	batch batch;
} linesrc;

extern int isbatchcmd;
extern linesrc *curline;

int execbatch(void);
int isautoexec(void);
char *sgets(char *line, int cnt, batch *b);
int openbatch(char *cmdpath, int setbatch);
void closebatch(void);			/* close current batchfile */
void autoexec(char *name);		/* execute startup batch */
void batchargs(ARGCV,int copy);

#endif