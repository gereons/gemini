/*
 * redir.c  -  I/O redirection
 * 05.11.90
 * 
 * Test mit Abfrage des aktuellen Handles per Basepage
 */

#include <stdio.h>
#include <tos.h>

#include "alloc.h"
#include "chario.h"
#include "comm.h"
#include "errfile.h"
#include "handle.h"
#include "mkargcv.h"
#include "mupfel.h"
#include "redir.h"
#include "shellvar.h"
#include "toserr.h"

#define NOREDIR	(ILLHND-1)
#define STDIN	0
#define STDOUT	1
#define STDAUX	2

#define DEBUG 0

SCCS(redir);

redirection redirect;

static int noclobber, auxrestore;


/* Original-Handle holen */

int get_original (int stdhandle)
{
	extern BASPAG *getactpd (void);	 /* XXX */

	BASPAG *B = getactpd ();
	
	if (B->p_stdfh[stdhandle])
		return B->p_stdfh[stdhandle];
	else
	{
		switch (stdhandle)
		{
			case 0:
			case 1:
				return Fopen ("CON:", O_RDWR);
			case 2:
				return Fopen ("AUX:", O_RDWR);
		}
	}
	return 0;
}

/* pseudocode */
#if COMMENT
if (exists(file,WRITE_ACCESS))
{
	if (may_not_clobber)
		return error
	if (append)
		seek_to_end(file)
	else
		create(file)
}
else
{
	if (exists(file,READ_ACCESS))	/* read-only */
		return error
	create(file)
}
#endif

void redirinit(void)
{
	redirect.in.hnd = redirect.out.hnd = redirect.aux.hnd = ILLHND;
}

static int outaux(char *cmd,redirinfo *r)
{
	int hnd;
	
	if (r->file==NULL)
		return NOREDIR;

#if DEBUG
	dprint("redirect %s to %s, ",
		r == &redirect.out ? "stdout" : "stdaux",
		r->file);
#endif

	if (isdevice(r->file))
		return Fopen(r->file,O_RDWR);
			
	if (access(r->file,A_WRITE))
	{
		if (noclobber && !r->clobber)
		{
			eprintf("%s: " RD_CANTMOD "\n",cmd,r->file);
			return ILLHND;
		}
		if (r->append)
		{
			hnd = Fopen(r->file,O_RDWR);
			Fseek(0L,hnd,SEEK_END);
		}
		else
			if ((hnd=Fcreate(r->file,0))<MINHND)
			{
				eprintf("%s: " RD_CANTCRT "\n",cmd,r->file);
				return ILLHND;
			}
	}
	else
	{
		if (access(r->file,A_EXIST))
		{
			eprintf("%s: " RD_ISRO "\n",cmd,r->file);
			return ILLHND;
		}
		if ((hnd=Fcreate(r->file,0))<MINHND)
		{
			eprintf("%s: " RD_CANTCRT "\n",cmd,r->file);
			return ILLHND;
		}
	}
#if DEBUG
	dprint("handle=%d\n",hnd);
#endif
	CommInfo.dirty |= drvbit(r->file);
	return hnd;
}

/* redirect handle 1 (console output) */
static int outredirect(char *cmd)
{
	return outaux(cmd,&redirect.out);	
}

/* redirect handle 2 (auxiliary output) */
static int auxredirect(char *cmd)
{
	if (redirect.aux.file != NULL)
	{
		closeerrfile();
		auxrestore = TRUE;
	}
	else
		auxrestore = FALSE;
	return outaux(cmd,&redirect.aux);	
}

/* redirect handle 0 (console input) */
static int inredirect(char *cmd)
{
	int inhnd;
	
	if (redirect.in.file==NULL)
		return NOREDIR;

	if ((inhnd=Fopen(redirect.in.file,0)) < MINHND)
	{
		eprintf("%s: " RD_CANTOPEN "\n",cmd,redirect.in.file);
		return ILLHND;
	}
	return inhnd;
}

/*
 * doredir(char *cmd)
 * connect handles 0,1 and/or 2 to files.
 * cmd is used for error messages
 * return FALSE if open/creat fails, TRUE otherwise
 */
int doredir(char *cmd)
{
	int outh, auxh, inh;
	
	redirinit();
	
	noclobber = (getvar("noclobber")!=NULL);

	if ((outh=outredirect(cmd))!=ILLHND)
		if ((auxh=auxredirect(cmd))!=ILLHND)
			inh=inredirect(cmd);
			
	redirect.in.hnd = inh;
	redirect.out.hnd = outh;
	redirect.aux.hnd = auxh;

	if (outh==ILLHND || auxh==ILLHND || inh==ILLHND)
		return FALSE;
				
	if (outh >= MINHND)
	{	
		redirect.out.ohnd = get_original (STDOUT);
		if (Fforce(STDOUT,outh)==EIHNDL)
			return FALSE;
	}
	if (inh >= MINHND)
	{
		redirect.in.ohnd = get_original (STDIN);
		if (Fforce(STDIN,inh)==EIHNDL)
			return FALSE;
	}
	if (auxh >= MINHND)
	{
		redirect.aux.ohnd = get_original (STDAUX);
		if (Fforce(STDAUX,auxh)==EIHNDL)
			return FALSE;
	}
	return TRUE;
}

static void closeredir(int orighnd,int delete,redirinfo *r)
{
	if (r->hnd >= MINHND)
	{
		Fforce (orighnd, r->ohnd);
		Fclose (r->hnd);
		Fclose (r->hnd);
		Fclose (r->ohnd);
		if (delete && !r->append)
			Fdelete(r->file);
	}
#if DEBUG
	if (r->file)
		dprint("closeredir(%d,%d,%s[%d])\n",orighnd,delete,
			r->file,r->hnd);
#endif
	if (r->file != NULL)
	{
		free(r->file);
		r->file = NULL;
	}
	r->hnd = ILLHND;
}		

/*
 * endredir(int delete)
 * if redirected, connect handles 0,1 and/or 2 back to console
 * if delete==TRUE, delete the source/target files
 */
void endredir(int delete)
{
	closeredir(STDIN,FALSE,&redirect.in);
	closeredir(STDOUT,delete,&redirect.out);
	closeredir(STDAUX,delete,&redirect.aux);
	if (auxrestore)
		restoreerrfile();
}
