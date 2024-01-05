/*
 * environ.c - environment management routines
 * 03.12.90
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "chario.h"
#include "curdir.h"
#include "comm.h"
#include "environ.h"
#include "gemsubs.h"
#include "hash.h"
#include "mupfel.h"
#include "parse.h"
#include "shellvar.h"

typedef struct enventry
{
	struct enventry *next;
	char *name;
	char *value;
} enventry;

#define LL_ENTRY enventry
#include "linklist.h"

SCCS(environ);

char *suffenv;		/* Duplicate of $SUFFIX, for extinsuffix() */

static enventry *ehead;

static int enveq(enventry *e1, enventry *e2)
{
	return !strcmp(e1->name,e2->name);
}

static int envcmp(enventry *e1, enventry *e2)
{
	return strcmp(e1->name,e2->name);
}

static int freeenv(enventry *e)
{
	free(e->name);
	free(e->value);
	return TRUE;
}

char *getenv(const char *name)
{
	enventry e, *e1;

	if (varindex(name)!=-1)
		return getvar(name);
			
	e.name = (char *)name;

	if ((e1=search(ehead,&e,enveq))!=NULL)
		return e1->value;
	else
		return NULL;
}

/*
 * splitenv(char *entry, char **name, char **value)
 * Split entry (which is of the form X[=[Y]]) into name and value pair.
 * If Y and/or = is missing, *value is NULL.
 */
static void splitenv(char *entry, char **name, char **value)
{
	char save, *eq;
	
	if ((eq=strchr(entry,'='))==NULL)
	{
		/* No '=' sign */
		*name = strdup(entry);
		*value = NULL;
	}
	else
	{
		save = *eq;
		*eq = '\0';
		*name = strdup(entry);
		/* Anything after '=' ? */
		if (*(eq+1)!='\0')
			*value = strdup(eq+1);
		else
			*value = NULL;
		*eq = save;
	}
}

int putenv(char *entry)
{
	char *name, *value;
	enventry e, *srch;
	
	splitenv(entry,&name,&value);

	/* clear hash table when PATH or SUFFIX are deleted/changed */
	if (!strcmp(name,"PATH") || !strcmp(name,"SUFFIX"))
		clearhash();

	if (!strcmp(name,"SUFFIX"))
	{
		if (suffenv!=NULL)
			free(suffenv);
		if (value!=NULL)
		{
			suffenv = strdup(value);
			strlwr(suffenv);
		}
		else
			suffenv = NULL;
	}
	
	e.name = name;
	if (value == NULL)
	{
		delete(&ehead,&e,enveq,freeenv);
		free(name);
	}
	else
	{
		if ((srch=search(ehead,&e,enveq))==NULL)
		{
			e.name = name;
			e.value = value;
			insert(&e,&ehead,sizeof(enventry));
			ehead = sortlist(ehead,0,envcmp);
		}
		else
		{
			free(srch->value);
			free(name);
			srch->value = value;
		}
	}
	return TRUE;
}

static void setshell(void)
{
	char var[100];

	strcpy(var,"SHELL=");
	chrcat(var,getdrv());
	chrcat(var,':');
	strcat(var,getdir());
	chrapp(var,'\\');

	if (CommInfo.isGemini)
		strcat(var,"MUPFEL.APP");
	else
		strcat(var,strfnm(CommInfo.PgmPath));

	if (access(&var[6],A_EXEC))
		putenv(strupr(var));
	else
	{
		strcpy(var,"SHELL=");
		strcat(var,"MUPFEL.APP");
		putenv(var);
	}
}

static int printentry(enventry *e)
{
	mprintf("%s=%s\n",e->name,e->value);
	return !intr();
}

static void printenv(void)
{
	walklist(ehead,printentry);
}

void envinit(char **envp)
{
	char env[30];

	ehead = NULL;
	suffenv = NULL;

	/* copy parent environment */
	while (*envp)
	{
		putenv(*envp);
		++envp;
	}

	if (getenv("SHELL")==NULL)
		setshell();
	
	if (_maxcol>0)
	{
		sprintf(env,"COLUMNS=%d",_maxcol+1);
		putenv(env);
	}
	if (_maxrow>0)
	{
		sprintf(env,"ROWS=%d",_maxrow+1);
		putenv(env);
	}
}

/*
 * makeenv(void)
 * Build a TOS environment area from the linked list.
 */
char *makeenv(int exarg, int argc, char **argv, char *cmdpath)
{
	char *env, *ep;
	enventry *e = ehead;
	long envsize = 0L;
	int i;

	/* Calculate size of environment */	
	while (e != NULL)
	{
		envsize += strlen(e->name)+strlen(e->value)+2L;
		e = e->next;
	}

	/* if using EXARG, add size of each argv member */
	if (exarg)
	{
		for (i=1; i<argc; ++i)
			envsize += strlen(argv[i]);
		envsize += strlen(cmdpath)+1;
		envsize += 5L+(long)i;	/* account for ARGV= and zeroes */
	}

	/*
	 * Get enough memory. 1K added in case the child process doesn't
	 * maintain his own copy
	 */
	ep = env = Malloc(envsize + 1024L);
	if (ep==NULL)
		return "";
		
	e = ehead;
	/* Copy all entries */
	while (e != NULL)
	{
		strcpy(ep,e->name);
		chrcat(ep,'=');
		strcat(ep,e->value);
		ep += strlen(ep)+1;
		e = e->next;
	}
	/* if using EXARG, append "ARGV=" and the argv[] contents */
	if (exarg)
	{
		strcpy(ep,"ARGV=");
		/* skip ARGV= and '\0' */
		ep += 6L;
		strcpy(ep,cmdpath);
		ep += strlen(ep)+1;
		for (i=1; i<argc; ++i)
		{
			strcpy(ep,argv[i]);
			ep += strlen(ep)+1;
		}
	}
	/* Terminate with double '\0' */
	*ep = '\0';
	return env;
}

void envexit(void)
{
	freelist(ehead,freeenv);
	if (suffenv!=NULL)
		free(suffenv);
}

int envnameok(char *s)
{
	while (*s)
	{
		if (!isalnum(*s) && *s!='+' && *s!='_' && *s!='#' && *s!='?')
			return FALSE;
		++s;
	}
	return TRUE;
}

int m_setenv(ARGCV)
{
	int retcode = 0;
	char *newvar;
	
	switch (argc)
	{
		case 1:
			printenv();
			break;
		case 2:
			retcode = putenv(argv[1]);
			break;
		case 3:
			if (!envnameok(argv[1]))
			{
				retcode = 1;
				eprintf("setenv: " EV_ILLNAME "\n",argv[1]);
				break;
			}
			if (varindex(argv[1]) != -1)
			{
				retcode = 1;
				eprintf("setenv: " EV_ILLVAR "\n",argv[1]);
				break;
			}
			newvar = malloc(strlen(argv[1])+strlen(argv[2])+2);
			strcpy(newvar,argv[1]);
			chrcat(newvar,'=');
			strcat(newvar,argv[2]);
			if (!putenv(newvar))
			{
				retcode = 1;
				eprintf("setenv: " EV_CANTSET "\n",argv[1]);
			}
			free(newvar);
			break;
		default:
			return printusage(NULL);
	}
	return retcode;
}

m_printenv(ARGCV)
{
	int i;
	char *env;
	
	if (argc==1)
		printenv();
	else
		for (i=1; i<argc && !intr(); ++i)
			if ((env=getenv(argv[i]))!=NULL)
				mprintf("%s=%s\n",argv[i],env);
			else
				eprintf(EV_NOTSET "\n",argv[i]);
	return 0;
}
