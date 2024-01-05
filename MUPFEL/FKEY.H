/*
 * fkey.h - definitions for fkey.c
 * 26.11.89
 */

#ifndef _M_FKEY
#define _M_FKEY

#include "imv.h"
 
void fkeyinit(void);		/* init function keys */
void fkeyexit(void);		/* de-init function keys */
char *getfkey(int keyno);	/* get function key string */
int convkey(int keyno);		/* convert scancode to index */
char **fkeyaddress(void);
void inheritfkey(IMV *imv);

#endif