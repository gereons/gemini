/* 
 * chario.h - definitions for chario.c
 * 05.01.91
 */

#ifndef _M_CHARIO
#define _M_CHARIO

int intr(void);				/* check for interrupt during output */
int wasintr(void);				/* last cmd was interrupted */
void resetintr(void);			/* reset interrupt state */
int checkintr(void);			/* direct check for ctrl-C */
void rawout(char c);			/* raw character output */
void canout(char c);			/* cooked character output */
void rawoutn(char c,unsigned int count);	/* print count chars (raw) */
void crlf(void);				/* print CR/LF */
int mprintf(char *fmt,...);		/* printf via outchar */
int eprintf(char *fmt,...);		/* printf to stderr */
void dprint(char *fmt,...);		/* debug mprint via rawcon */
void beep(void);				/* ring bell */
void vt52(char c);				/* vt52 esc sequence */
void vt52cm(int x, int y);		/* vt52 cursor motion */
int inbuffchar(long *lp);		/* get char from typeahead */
long inchar(void);				/* get char and scancode */
void initoutput(void);			/* init gemdosout pointer */
#endif