/*
 * @(#) Mupfel\mvenus.c  -  Mupfel/Venus interface
 * @(#) Gereon Steffens & Stefan Eissing, 04. MÑrz 1991
 */

/* this file is needed only when compiling GEMINI */

#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "batch.h"
#include "chario.h"
#include "comm.h"
#include "curdir.h"
#include "environ.h"
#include "gemsubs.h"
#include "mupfel.h"
#include "sysvec.h"
#include "stand.h"
#include "vt52.h"
#include "lineedit.h"
#include "flydial\flydial.h"

#if STANDALONE
#error why compile MVENUS.C when STANDALONE==1 ?
#endif

SCCS(mvenus);

#define DEBUG 0
#if DEBUG
#include "alert.h"
#endif

static void dowindopen(char *dir, char *drv)
{
	static int firstopen = TRUE;
	char tmp[40];

	coninit();
	cursinit();

	setLAxy(_maxcol,_maxrow);
	sprintf(tmp,"ROWS=%d",_maxrow+1);
	putenv(tmp);
	sprintf(tmp,"COLUMNS=%d",_maxcol+1);
	putenv(tmp);

	wrapon();
	cursoron();

	if (firstopen)
	{
		char *startdir;

		firstopen = FALSE;

		if ((startdir=getenv("CONSOLEDIR"))!=NULL && isdir(startdir))
			chdir(startdir);
		strcpy(dir,getdir());	/* remember start dir */
		*drv = getdrv();		/* and drive */

		clearscreen();
		initline(TRUE);
	}

	CommInfo.cmd = neverMind;
	windupdate(FALSE);
	GrafMouse(M_ON, NULL);
}

static void dowindclose(void)
{
	char tmp[30];
	
	cursoroff();
	wrapoff();	
	conexit();
	cursexit();
	resetLAxy();
	
	sprintf(tmp,"ROWS=%d",_physrow+1);
	putenv(tmp);
	sprintf(tmp,"COLUMNS=%d",_physcol+1);
	putenv(tmp);
	
	CommInfo.cmd = neverMind;
	windupdate(FALSE);
	GrafMouse(M_ON, NULL);
}

void m_mupfel(void)
{
	static char drv = -1, dir[300]; 
	/* drv/dir are used to remember dir where window was opened */
	char *cmdline;
	char savedrv, *savedir;
	const char *cmd;
	int saved, command_pending = FALSE;
	int venus_wants_exec;
	int pushed_venus_command = FALSE;
	long key;

	windupdate(TRUE);
	GrafMouse(M_OFF, NULL);

	venus_wants_exec = (CommInfo.cmd == execPrg);

	if (!venus_wants_exec)
	{
		if (CommInfo.cmd == neverMind)
		{ 
			if (execbatch() || curline->pushedline)
			{
				command_pending = TRUE;
				if (curline->pushedline)
					pushed_venus_command = TRUE;
			}
			else
			{
				initline(TRUE);
				windupdate(FALSE);
				GrafMouse(M_ON, NULL);
				return;
			}
		}
		cmd = NULL;
	}
	else
	{
		cmd = CommInfo.cmdArgs.cmdspec;
#if DEBUG
		alert(1,1,1,"exec %s","OK",cmd);
#endif
	}

	saved = !cmd;

	switch (CommInfo.cmd)
	{
		case windOpen:
			dowindopen(dir, &drv);
			return;
		case windClose:
			dowindclose();
			return;
		case feedKey:
			if (((CommInfo.cmdArgs.key & 0xFF) == '\t')
				|| ((CommInfo.cmdArgs.key & 0xFF) == '\r'))
			{
				chdrv(drv);	/* change to our own drive */
				chdir(dir);	/* ... and directory */
			}
			cmd = feedchar(CommInfo.cmdArgs.key);
			while (!cmd && inbuffchar(&key))
				cmd = feedchar(key);
			break;
	}
	/* 
	 * Reset CommInfo if nothing else happens
	 */
	CommInfo.cmd = neverMind;
	cmdline = NULL;

	if (!cmd && !command_pending)
	{
		windupdate(FALSE);
		GrafMouse(M_ON, NULL);
		return;
	}

	if (saved)
	{
		if (!execbatch())
		{
			int mydrv;
			char mydir[256];
#if DEBUG
			alert(1,1,1,"m_mupfel: cd#1 to %c:%s","OK",drv,dir);
#endif
			mydrv = Dgetdrv();
			Dgetpath(mydir,mydrv+1);
			mydrv += 'A';
			if (mydir[0] == '\0')
				strcpy(mydir,"\\");
			if (mydrv != drv || stricmp(mydir,dir))
			{
				chdrv(drv);	/* change to our own drive */
				chdir(dir);	/* ... and directory */
			}
		}
	}
	else
	{
		savedrv = getdrv();
		savedir = strdup(getdir());
#if DEBUG
		alert(1,1,1,"m_mupfel: setcurdir()","OK");
#endif
		setcurdir();
	}

	/* 
	 * Normalerweise soll bei cmd == NULL nichts getan werden, aber
	 * Venus gibt damit bekannt, daû nach dem Start eines GEM-Prgs
	 * die Mupfel weitermachen soll, wenn ein Batch-File bearbeitet
	 * wird.
	 */
	if (venus_wants_exec && cmd)
		crlf();
	while (cmd || command_pending)
	{
		char cmdstr[400];
		
		command_pending = FALSE;

		comefromvenus = pushed_venus_command;		
		if (cmd)
		{
			strcpy(cmdstr,cmd);
			cmdline = mupfel(cmdstr);	/* main control loop */
		}
		else
			cmdline = mupfel(NULL);
		cmd = NULL;
		comefromvenus = FALSE;
		
		if (!cmdline && (CommInfo.cmd != overlay))
		{
			long key;

			/* Wir hatten ein Kommando von Venus (!saved)
			 * und schauen jetzt ob wir typeaheads ausfÅhren
			 * mÅssen. Dazu mÅssen wir erst wieder in unser
			 * eigenes Directory gehen.
			 */
			if (!saved && savedir)
			{
#if DEBUG
				alert(1,1,1,"m_mupfel: cd#2 to %c:%s","OK",savedrv,savedir);
#endif
				chdrv(savedrv);
				chdir(savedir);
				free(savedir);
				savedir = NULL;
			}
			initline(TRUE);
			while (!cmd && inbuffchar(&key))
			{
				cmd = feedchar(key);
			}
		}
	}

	if (saved)
	{
		drv = getdrv();		/* remember drive */
		strcpy(dir,getdir());	/* and dir */
#if DEBUG
		alert(1,1,1,"m_mupfel: remember path %c:%s","OK",drv,dir);
#endif
	}
	else
		if (savedir)
		{
			if (!execbatch())
			{
#if DEBUG
				alert(1,1,1,"m_mupfel: cd#3 to %c:%s","OK",savedrv,savedir);
#endif
				chdrv(savedrv);
				chdir(savedir);
			}
			free(savedir);
			savedir = NULL;
		}

	windupdate(FALSE);
	GrafMouse(M_ON, NULL);

	if (cmdline)
	{
		CommInfo.cmd = execPrg;
		CommInfo.cmdArgs.cmdspec = cmdline;
	}
}
