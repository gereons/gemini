/*
 * hash.h - definitions for hash.c
 * 02.06.90
 */

#ifndef _M_HASH
#define _M_HASH

#include "imv.h"

void hashinit(void);			/* init hash table */
char *searchhash(char *cmd, int increment);	/* search cmd in hash table */
void enterhash(char *cmd, char *path, int cost);
int clearhash(void);			/* clear hashtable */
void deletehash(char *cmd);		/* delete one entry (for rehash) */
void hashadjust(char *cmd);		/* decrement hit counter */
void *hashaddress(void);
void inherithash(IMV *imv);

#endif