/*
 * lineedit.h  -  definitions for lineedit.c
 * 06.02.90
 */

#ifndef _M_LINEEDIT
#define _M_LINEEDIT

/* Size of command line buffer */
#define LINESIZE	256
#define LINSIZ		LINESIZE+1

void lineedinit(void);			/* init line editor */
void lineedexit(void);			/* de-init line editor */
void histinit(void);			/* allocate history buffer */

char *feedchar(long l);
void initline(int flag);
char *readline(int flag);

extern char lbuf[];
extern int firstscroll;

#endif