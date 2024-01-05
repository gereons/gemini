/*
 * @(#) Gemini\iconhash.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: hash table for icon rules
 */

#include <string.h>
#include "vs.h"
#include "iconhash.h"
#include "iconrule.h"

store_sccs_id(iconhash);

#define HASHMAX	23		/* L„nge der Hashtabelle */

static DisplayRule *hashTable[HASHMAX];	/* the table itself */

/*
 * static word hashvalue(const char *name)
 * a simple hash function for strings
 */
static word hashValue(const char *name)
{
	register word h = 0;
	const char *p = name; 
	
	while(*p)
	{
		h = (h << 1) + (*p++);
	}

	h %= HASHMAX;
	if(h < 0)
		h = -h;
		
	return h;
}

static void hashRule(DisplayRule *pd)
{
	DisplayRule *cd;
	word hash;
	
	pd->wasHashed = TRUE;
	
	hash = hashValue(pd->wildcard);
	
	cd = hashTable[hash];
	hashTable[hash] = pd;
	pd->nextHash = cd;
}

void builtIconHash(void)
{
	DisplayRule *firstRule,*pd;
	
	clearHashTable();
	if((firstRule = getFirstDisplayRule()) != NULL)
	{
		for(pd = firstRule; pd != NULL; pd = pd->nextrule)
		{
			pd->nextHash = NULL;
			pd->wasHashed = FALSE;
		}
		clearHashTable();
				
		for(pd = firstRule; pd != NULL; pd = pd->nextrule)
		{
			if(strcspn(pd->wildcard, "*?[],;") == strlen(pd->wildcard))
			{
				hashRule(pd);
			}
		}
	}
}

DisplayRule *getHashedRule(word isFolder,const char *name)
{
	DisplayRule *pd;
	word hash;
	
	hash = hashValue(name);
	pd = hashTable[hash];
	if(isFolder)
	{
		while(pd)
		{
			if((pd->isFolder)
				&& !strcmp(name,pd->wildcard))
				return pd;

			pd = pd->nextHash;
		}
	}
	else
	{
		while(pd)
		{
			if((!pd->isFolder)
				&& !strcmp(name,pd->wildcard))
				return pd;
	
			pd = pd->nextHash;
		}
	}
	return NULL;
}

void clearHashTable(void)
{
	register word i;
	
	for(i=0; i < HASHMAX; i++)
		hashTable[i] = NULL;
}