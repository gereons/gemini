/*
 * imv.h  -  definitions for internal mupfel variables
 * 26.11.89
 */
 
#ifndef _M_IMV
#define _M_IMV

typedef struct
{
	char magic[4];	/* "IMV" */
	void *alias;	/* address of alias list */
	void *hash;	/* address of hash list */
	void *fkey;	/* address of fkey array */
	void *shvar;	/* address of shvar array */
} IMV;

#endif