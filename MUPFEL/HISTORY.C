/*
 * history.c  -  history file management
 * 01.01.91
 */
 
#include <tos.h>
#include <stdio.h>
#include <string.h>

#include "alloc.h"
#include "chario.h"
#include "environ.h"
#include "filebuf.h"
#include "handle.h"
#include "history.h"
#include "mupfel.h"
#include "shellvar.h"
#include "toserr.h"

SCCS(history);

#define HISTORYFILE	"MUPFEL.HST"

static char *histfile(void)
{
	char *hdir, *hf;

	hf = getenv("HISTFILE");
	if (hf == NULL)
	{
		hdir = getenv("HOME");
		if (hdir == NULL)
			return NULL;
		hf = malloc(strlen(hdir)+strlen(HISTORYFILE)+2);
		strcpy(hf,hdir);
		chrapp(hf,'\\');
		strcat(hf,HISTORYFILE);
		return hf;
	}
	else
		return strdup(hf);
}

void readhistory(int maxhist, char **hbuf, int *maxhp)
{
	char *hf, *buffer, *bufp;
	char *s;
	int i, hnd;
	
	if ((hf = histfile()) == NULL)
		return;
	if ((hnd = Fopen(hf,0))<MINHND)
	{
		free(hf);
		return;
	}
	if (filebuf(hnd,hf,&buffer,"readhistory")==BUF_ERR)
	{
		free(hf);
		return;
	}
	while ((s=strrchr(buffer,'\r')) != NULL)
		*s = '\0';
	bufp = buffer;
	for (i=0; i<maxhist; ++i)
	{
		if (*bufp == '\0')
			break;
		hbuf[i] = strdup(bufp);
		bufp += strlen(bufp)+2;
	}
	*maxhp = i-1;
	Mfree(buffer);
	free(hf);
}

void writehistory(int maxhist, char **hbuf)
{
	char *hf, *buffer;
	size_t bufsize = 0;
	int i, hnd, first;
	
	if (getvar("histsave") == NULL)
		return;
		
	if ((hf = histfile()) == NULL)
		return;

	if (!stricmp(hbuf[0],"exit"))
		first = 1;
	else
		first = 0;

	for (i=first; i<=maxhist; ++i)
		bufsize += strlen(hbuf[i])+2;

	++bufsize;
	buffer = malloc(bufsize);
	*buffer = '\0';

	for (i=first; i<=maxhist; ++i)
	{
		strcat(buffer,hbuf[i]);
		strcat(buffer,"\r\n");
	}

	if (bufsize==0 || (hnd=Fcreate(hf,0))<MINHND)
	{
		free(hf);
		free(buffer);
		eprintf(HI_CRTERR "\n");
		return;
	}
	if (Fwrite(hnd,bufsize-1,buffer) != bufsize-1)
	{
		oserr = WRITE_FAULT;
		eprintf(HI_WRTERR "\n");
	}
	Fclose(hnd);
	free(hf);
	free(buffer);
	return;
}
