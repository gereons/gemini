/*
 * shellvar.c  -  internal variables
 * 05.11.90
 */

#include <stdlib.h>
#include <string.h> 

#include "alloc.h"
#include "chario.h"
#include "imv.h"
#include "mupfel.h"
#include "shellvar.h"
#include "sysvec.h"

SCCS(shellvar);

typedef struct
{
	char *name;
	char *value;
	char	internal;
	char readonly;
} shvar;

static shvar shellvar[] =
{
	{"0",		NULL, TRUE, FALSE},
	{"1",		NULL, TRUE, FALSE},
	{"2",		NULL, TRUE, FALSE},
	{"3",		NULL, TRUE, FALSE},
	{"4",		NULL, TRUE, FALSE},
	{"5",		NULL, TRUE, FALSE},
	{"6",		NULL, TRUE, FALSE},
	{"7",		NULL, TRUE, FALSE},
	{"8",		NULL, TRUE, FALSE},
	{"9",		NULL, TRUE, FALSE},
	{"#",		NULL, TRUE, FALSE},
	{"*",		NULL, TRUE, FALSE},
	{"?",		NULL, TRUE, FALSE},	 /* return code of last command */
	{"cwd",		NULL, FALSE, TRUE},  /* current working dir */
	{"history",	NULL, FALSE, FALSE}, /* max. history entries */
	{"histsave",	NULL, FALSE, FALSE}, /* save history on exit? */
	{"keepfree",	NULL, FALSE, FALSE}, /* don't use this much for cp */
	{"noclobber",	NULL, FALSE, FALSE}, /* overwrite existing files? */
	{"nohistdouble",NULL,FALSE, FALSE}, /* duplicate history entries? */
	{"screensave",	NULL, FALSE, FALSE}, /* save screen for gem prgs */
	{"shellcount",	NULL, FALSE, TRUE},  /* how often is Mupfel installed? */
	{"slashconv",	NULL, FALSE, FALSE}, /* convert slashes to backslashes? */
	{"drivelist",  NULL, FALSE, TRUE},  /* list of all connected drives */
};

void *shvaraddress(void)
{
	return (void *)shellvar;
}

void initdrivelist(void)
{
	char drvmap[33];
	int i;
	size_t dmap;
	
	*drvmap = '\0';
	dmap = Drvmap();
	for (i=0; i<16; ++i)
	{
		if (((dmap>>i)&1)==1)
		{
			if (i==1 && getnflops()==1)
				continue;
			chrcat(drvmap,'A'+i);
		}
	}
	setvar("drivelist",drvmap);
}

void freevar(void)
{
	int i;
	
	for (i=0; i<DIM(shellvar); ++i)
		if (shellvar[i].value!=NULL)
			free(shellvar[i].value);
}

int varindex(const char *var)
{
	int i;

	for (i=0; i<DIM(shellvar); ++i)
		if (!strcmp(var,shellvar[i].name))
			return i;
	return -1;
}

char *getvar(const char *name)
{
	int i = varindex(name);

	return i!=-1 ? shellvar[i].value : NULL;
}

char *getvarstr(const char *name)
{
	char *val = getvar(name);
	
	if (val!=NULL)
		return val;
	else
		return "";
}

int getvarint(const char *name)
{
	char *val = getvar(name);
	
	if (val!=NULL)
		return atoi(val);
	else
		return 0;
}

int setvar(const char *name, const char *value)
{
	int i;

	i = varindex(name);
	
	if (i==-1)
		return -1;

	if (shellvar[i].value!=NULL)
		free(shellvar[i].value);
	shellvar[i].value = (value==NULL) ? NULL : strdup(value);
	return TRUE;
}

static void showvars(void)
{
	int i;
	
	for (i=0; i<DIM(shellvar) && !intr(); ++i)
		if (!shellvar[i].internal)
			mprintf("%-15s %s\n",shellvar[i].name,
				shellvar[i].value==NULL ? SV_NOTSET : shellvar[i].value);
}

static int unsetvar(char *var)
{
	int i;
	
	i=varindex(var);
	if (i!=-1)
	{
		setvar(var,NULL);
		return 0;
	}
	else
	{
		oserr = -1;
		eprintf("%s: " SV_ILLVAR "\n",var);
		return 1;
	}
}

int m_set(ARGCV)
{
	int i;
	
	switch (argc)
	{
		case 1:
			showvars();
			return 0;
		case 2:
			return unsetvar(argv[1]);
		case 3:
			i=varindex(argv[1]);
			if (i==-1 || shellvar[i].internal || shellvar[i].readonly)
			{
				oserr = -1;
				eprintf("%s: " SV_ILLVAR "\n",argv[1]);
				return 1;
			}
			setvar(argv[1],argv[2]);
			return 0;
		default:
			return printusage(NULL);
	}
}

void inheritvars(IMV *imv)
{
	shvar *parentvar;
	int i,v;
	char *varname[] =
	{
		"noclobber", "screensave", "slashconv",
		"keepfree",  "histsave", "history", "nohistdouble"
	};

	if ((parentvar = imv->shvar)==NULL)
		return;
		
	for (i=0; i<DIM(varname); ++i)
	{
		if ((v=varindex(varname[i]))!=-1)
			setvar(parentvar[v].name,parentvar[v].value);
	}
}