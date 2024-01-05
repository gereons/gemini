/*
 * hash.c  -  hash table management
 * 05.10.90
 */

#include <ctype.h>
#include <string.h> 

#include "alloc.h"
#include "chario.h"
#include "getopt.h"
#include "imv.h"
#include "hash.h"
#include "mupfel.h"
#include "tree.h"

typedef struct cmdhash
{
	struct cmdhash *next;
	char *cmd;
	char *path;
	ulong value;
	int hits;
	int cost;
} hashentry;
 
#define LL_ENTRY	hashentry
#include "linklist.h"

SCCS(hash);

static hashentry *hhead;

/*
 * hashinit(void)
 * initialize hash table
 */
void hashinit(void)
{
	hhead = NULL;
}

void *hashaddress(void)
{
	return (void *)hhead;
}

static ulong hashvalue(char *str)
{
	ulong val = 0;

	while (*str!='\0')
		val += tolower(*str++);
	return val;
}

/*
 * searchhash(char *cmd)
 * search hash table for cmd
 * return its path if found, NULL otherwise
 */
char *searchhash(char *cmd,int increment)
{
	hashentry *h = hhead;
	ulong hv = hashvalue(cmd);

	while (h != NULL)		
	{
		if (hv==h->value && !stricmp(h->cmd,cmd))
		{
			if (increment)
				++h->hits;
			return h->path;
		}
		h = h->next;
	}
	return NULL;
}

static int hasheq(hashentry *h1, hashentry *h2)
{
	return h1->value == h2->value && !stricmp(h1->cmd,h2->cmd);
}

/*
 * enterhash(char *cmd, char *path, int cost)
 * enter cmd,path,cost in hash table
 * cmd must be < 9 chars and may not contain any of \.:
 */
void enterhash(char *cmd, char *path, int cost)
{
	hashentry h;

	if (strchr(cmd,'\\') || strchr(cmd,'.') || *(cmd+1)==':')
		return;
	if (strlen(cmd)>8)
		return;

	h.cmd = cmd;
	h.value = hashvalue(cmd);
	
	if (search(hhead,&h,hasheq) != NULL)
		return;

	h.cmd = strlwr(strdup(cmd));
	h.path = strlwr(strdup(path));
	h.cost = cost;
	if (cost == 0)
		h.hits = 0;	/* entered by "hash -d" */
	else
		h.hits = 1;	/* entered by path search */
	insert(&h,&hhead,sizeof(hashentry));
}

/*
 * printhash(void)
 * display hash table
 */
static int printhash(hashentry *h)
{
	mprintf("%-6d%-6d%-11s%s\n",h->hits,h->cost,h->cmd,h->path);
	return !intr();
}

static int freehash(hashentry *h)
{
	free(h->cmd);
	free(h->path);
	return TRUE;
}

/*
 * deletehash(char *cmd)
 * Delete the hash entry for cmd. Used when a program is renamed or
 * moved to another directory
 */
void deletehash(char *cmd)
{
	hashentry h;
	
	h.cmd = cmd;
	h.value = hashvalue(cmd);
	delete(&hhead,&h,hasheq,freehash);
}

/*
 * decrement the hit counter for cmd's entry
 * used by execcmd in Gemini
 */
void hashadjust(char *cmd)
{
	hashentry *h = hhead;
	
	while (h)
	{
		if (!stricmp(cmd,h->path))
		{
			--h->hits;
			return;
		}
		h = h->next;
	}
}

/*
 * clearhash(void)
 * re-initialize hash table, free memory on the way
 */
int clearhash(void)
{
	freelist(hhead,freehash);
	hhead = NULL;
	return 0;
}

int m_hash(ARGCV)
{
	GETOPTINFO G;
	int c;
	static int clearflag;
	char *hashdir = NULL;
	struct option long_option[] =
	{
		{ "remove", FALSE, &clearflag, TRUE },
		{ "directory", TRUE, NULL, 0 },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;

	clearflag = FALSE;
	optinit (&G);

	while ((c = getopt_long (&G, argc, argv, "rd:", long_option,
		&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				if (G.optarg)
					goto dir;
				break;
			case 'r':
				clearflag = TRUE;
				break;
			case 'd':
			dir:
				hashdir = G.optarg;
				break;
			default:	
				return printusage(long_option);
		}
	if (clearflag && hashdir)
	{
		eprintf("hash: " HA_OPTERR "\n");
		return 1;
	}
	if (clearflag)
		return clearhash();
	if (hashdir)
	{
		if (isdir(hashdir))
			hashtree(hashdir);
		else
		{
			eprintf("hash: " HA_NODIR "\n",hashdir);
			return 1;
		}
	}
	else
	{
		if (G.optind < argc)
			return printusage(long_option);
		mprintf(HA_TITLE "\n");
		walklist(hhead,printhash);
	}
	return 0;
}

void inherithash(IMV *imv)
{
	hashentry *h;
	
	if (hhead!=NULL || imv->hash==NULL)
		return;
	h = imv->hash;
	while (h)
	{
		enterhash(h->cmd,h->path,h->cost);
		h = h->next;
	}
}
