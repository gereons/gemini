/*
 * alias.c  -  alias management routines
 * 05.01.91
 */
 
#include <string.h>

#include "alias.h"
#include "alloc.h"
#include "chario.h"
#include "imv.h"
#include "mupfel.h"

typedef struct a_entry
{
	struct a_entry *next;
	char *name;
	char *replace;
	int used;
} aliasentry;

#define LL_ENTRY aliasentry
#include "linklist.h"

SCCS(alias);

static aliasentry *ahead;

void aliasinit(void)
{
	ahead = NULL;
}

void *aliasaddress(void)
{
	return (void *)ahead;
}

static int freealias(aliasentry *a)
{
	free(a->name);
	free(a->replace);
	return TRUE;
}

void aliasexit(void)
{
	freelist(ahead,freealias);
}

static int aliaseq(aliasentry *a1, aliasentry *a2)
{
	return !strcmp(a1->name,a2->name);
}

static int aliascmp(aliasentry *a1, aliasentry *a2)
{
	return strcmp(a1->name,a2->name);
}

static int setalias(char *name, char *replace)
{
	aliasentry a, *srch;

	if (!strcmp(name,NOALIAS))
	{
		oserr = -1;
		eprintf("alias: " AL_CANTSET "\n",NOALIAS);
		return 1;
	}
	
	a.name = name;
	if ((srch=search(ahead,&a,aliaseq))==NULL)
	{
		a.name = strdup(name);
		a.replace = strdup(replace);
		insert(&a,&ahead,sizeof(aliasentry));
		ahead = sortlist(ahead,0,aliascmp);
	}
	else
	{
		free(srch->replace);
		srch->replace = strdup(replace);
	}
	return 0;
}

static int delalias(char *name)
{
	aliasentry a, *newhead;

	a.name = name;	
	if ((newhead=delete(&ahead,&a,aliaseq,freealias))==(aliasentry *)-1)
	{
		eprintf("alias: " AL_UNDEF "\n",name);
		return 1;
	}
	ahead = newhead;
	return 0;
}

static int showalias(aliasentry *a)
{
	mprintf("%s: %s\n",a->name,a->replace);
	return !intr();
}

int m_alias(ARGCV)
{
	int retcode = 0;
	
	switch(argc)
	{
		case 1:
			walklist(ahead,showalias);
			break;
		case 2:
			retcode = delalias(argv[1]);
			break;
		case 3:
			retcode = setalias(argv[1],argv[2]);
			break;
		default:
			return printusage(NULL);
	}
	return retcode;
}

char *isalias(char *name)
{
	aliasentry a, *srch;

	a.name = name;
	if ((srch=search(ahead,&a,aliaseq))==NULL)
		return NULL;
	else
	{
		if (!srch->used)
		{
			srch->used = TRUE;
			return srch->replace;
		}
		else
			return (char *)-1;
	}
}

char *searchalias(char *name)
{
	aliasentry a, *srch;

	a.name = name;
	if ((srch=search(ahead,&a,aliaseq))==NULL)
		return NULL;
	else
		return srch->replace;
}

void inheritalias(IMV *imv)
{
	aliasentry *a;
	
	if (ahead!=NULL || imv->alias == NULL)
		return;
	a = imv->alias;
	while (a)
	{
		setalias(a->name,a->replace);
		a = a->next;
	}
}

void unusedalias(void)
{
	aliasentry *a = ahead;
	while (a)
	{
		a->used = FALSE;
		a = a->next;
	}
}

int m_noalias(void)
{
	return printusage(NULL);
}
