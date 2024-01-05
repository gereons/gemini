/*
 * fkey.c  -  internal "fkey" function
 * 05.11.90
 */
 
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "chario.h"
#include "comm.h"
#include "fkey.h"
#include "imv.h"
#include "mupfel.h"
#include "scan.h"

#define MAXFKEY 21
 
SCCS(fkey);

static char *fkey[MAXFKEY];

char **fkeyaddress(void)
{
	return fkey;
}

void fkeyinit(void)
{
	int i;
	
	for (i=1; i<MAXFKEY; ++i)
		fkey[i]=NULL;
	CommInfo.fkeys = fkey;
}

void fkeyexit(void)
{
	int i;
	
	for (i=1; i<MAXFKEY; ++i)
		if (fkey[i]!=NULL)
			free(fkey[i]);
}

int convkey(int keyno)
{
	if (keyno>=F1 && keyno<=F10)
		return keyno-F1+1;
	if (keyno>=SF1 && keyno<=SF10)
		return keyno-SF1+11;
	return -1;
}

void setfkey(int keyno, char *text)
{
	if (keyno>0 && keyno<MAXFKEY)
	{
		if (fkey[keyno]!=NULL)
			free(fkey[keyno]);
		if (strlen(text)>0)
			fkey[keyno] = strdup(text);
		else
			fkey[keyno] = NULL;
	}
	else
	{
		oserr = -1;
		eprintf(FK_ILLKEY "\n");
	}
}

char *getfkey(int keyno)
{
	if (keyno>0 && keyno<MAXFKEY)
		return fkey[keyno]!=NULL ? fkey[keyno] : "";
	else
		return NULL;
}

static char *keyname(int key)
{
	static char *kname[] =
	{
		"??",
		"F1",  "F2",  "F3",  "F4",  "F5",
		"F6",  "F7",  "F8",  "F9",  "F10",
		"SF1", "SF2", "SF3", "SF4", "SF5",
		"SF6", "SF7", "SF8", "SF9", "SF10"
	};
	
	return (key>0 && key<MAXFKEY) ? kname[key] : kname[0];
}

static int getkeyno(char *name)
{
	int keyno,i;
	
	strupr(name);
	if ((keyno=atoi(name))==0)
	{
		if (*name=='F')
		{
			keyno=atoi(&name[1]);
			if (keyno>0 && keyno<21)
				return keyno;
		}
		for (i=1; i<MAXFKEY; ++i)
			if (!strcmp(name,keyname(i)))
				return i;
		return -1;
	}
	else
		return keyno;
}

void printfkeys(void)
{
	int i;
	
	for (i=1; i<MAXFKEY && !intr(); ++i)
		if (fkey[i]!=NULL)
			mprintf("%4s: %s\n",keyname(i),fkey[i]);
}

int m_fkey(ARGCV)
{
	switch(argc)
	{
		case 1:
			printfkeys();
			break;
		case 2:
			setfkey(getkeyno(argv[1]),"");
			break;
		case 3:
			setfkey(getkeyno(argv[1]),argv[2]);
			break;
		default:
			return printusage(NULL);
	}
	return 0;
}

void inheritfkey(IMV *imv)
{
	char **parentfkey;
	int i;
	
	if ((parentfkey = imv->fkey)==NULL)
		return;
	
	for (i=1; i<MAXFKEY; ++i)
		fkey[i] = (parentfkey[i]) ? strdup(parentfkey[i]) : NULL;
}