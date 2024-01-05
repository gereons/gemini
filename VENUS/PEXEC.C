/*
 * @(#) Gemini\Pexec.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: Starten von Programmen
 *
 */
 
#include <vdi.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <flydial\flydial.h>

#include "vs.h"
#include "menu.rh"

#include "pexec.h"
#include "util.h"
#include "fileutil.h"
#include "init.h"
#include "myalloc.h"
#include "window.h"
#include "redraw.h"
#include "iconrule.h"
#include "iconhash.h"
#include "undo.h"
#include "infofile.h"
#include "applmana.h"
#include "stand.h"
#include "menu.h"
#include "filedraw.h"
#include "overlay.h"
#include "message.h"

store_sccs_id(pexec);

/* externals
 */
extern OBJECT *pmenu;
extern WindInfo wnull;
extern DeskInfo NewDesk;
extern word apid,phys_handle,handle;
extern word deskx,desky,deskw,deskh,hchar,wchar;
extern word pxy[];
#if MERGED
extern word mupfGem;
#endif

void makeBarName(char *cmd)
{
	word textx,texty;

	/* white bar at top */

	GrafMouse(M_OFF,NULL);
	pxy[0] = pxy[1]= 0;
	pxy[2]=deskx + deskw;
	pxy[3]=desky + deskh;
	
	switchFont();		/* install system font */
	
	vs_clip(handle,TRUE,pxy);
	vswr_mode(handle,MD_REPLACE);
	vsf_color(handle,WHITE);
	vsf_interior(handle,FIS_SOLID);
	vsl_type(handle,1);				/* durchgezogene Linie */
	pxy[0] = pxy[1] = 0;
	pxy[2] = deskx + deskw;
	pxy[3] = hchar+2;
	v_bar(handle,pxy);
	/* command's name centered */
	textx = ((deskx+deskw) - (int)strlen(cmd)*wchar)/2;
	texty = 1;
	v_gtext (handle, textx, texty, cmd);
	
	switchFont();			/* and back to own font */
	
	/* black line under bar */
	pxy[0] = 0;
	pxy[1] = pxy[3] = hchar+2;
	pxy[2] = deskx + deskw;
	v_pline(handle,2,pxy);
	GrafMouse(M_ON,NULL);
}

/*
 * static void TosScreen(word make)
 * if (make) cursor on and clear screen
 * else hide cursor
 */
static void TosScreen(word make)
{
	if(make)
	{
		GrafMouse(M_OFF,NULL);
		printf("\033e \33E");	/* clear screen cursor on */
	}
	else
	{
		printf("\33f");			/* cursor off */
		GrafMouse(M_ON,NULL);
	}
}

static void hideWindows(word flag)
{
	if(flag)				/* hide */
	{
		makeConfInfo();
		freeIconRules();	/* free List of IconRules */
		freeApplRules();	/* free List of Applicationrules */
		closeAllWind();
		freeTmpBlocks();	/* free tmpmalloc() Blocks */
	}
	else					/* show */
	{
		execConfInfo(FALSE);
	}
}

static void makeBackground(word startmode,word flag,char *name)
{
	if(flag)				/* hide own */
	{
		menu_bar(pmenu,FALSE);
		GrafMouse(HOURGLASS,NULL);
		if(startmode & WCLOSE_START)
		{
			delAccWindows();
			wind_set(0,WF_NEWDESK,NULL,0);
		}

		if(startmode & GEM_START)
		{
			if(startmode & WCLOSE_START)
				form_dial(FMD_FINISH,0,0,0,0,0,0,
							deskx+deskw,desky+deskh);
			makeBarName(name);
		}
		else
			TosScreen(TRUE);
			
		WindUpdate(END_UPDATE);
		appl_exit();		/* for AC_CLOSE messages */
	}
	else					/* show own */
	{
		apid = appl_init();
		WindUpdate(BEG_UPDATE);
		
		if (startmode & WAIT_KEY)
			WaitKeyButton();
		
		if (!(startmode & GEM_START))		/* hide cursor */
			TosScreen(FALSE);

		if (startmode & WCLOSE_START)
			delAccWindows();

		v_show_c(phys_handle,0);
		
		wind_set(0,WF_NEWDESK,wnull.tree,0);
		if ((startmode & WCLOSE_START) || !(startmode & GEM_START))
			form_dial(FMD_FINISH,0,0,0,0,0,0,deskx+deskw,desky+deskh);

		if ((startmode & GEM_START) && !(startmode & WCLOSE_START))
			allrewindow(WINDINFO|MUPFELTOO);
		
		menu_bar(pmenu,TRUE);
		GrafMouse(ARROW,NULL);
	}
}

static word doSystem(const char *fname, const char *command,
					word mupfel, word startmode)
{
	size_t len;
	char *sysline;
	word retcode;

	len = strlen(fname) + 1;
	if(command)
		len += 1 + strlen(command);
		
	sysline = malloc(len);

	if(sysline)
	{
		strcpy(sysline,fname);
		if(command)
		{
			strcat(sysline," ");
			strcat(sysline,command);
		}

#if STANDALONE
		(void)startmode;
		(void)mupfel;
#else
		if (mupfel)
		{
			doMupfel(sysline, (startmode & WAIT_KEY));
			retcode = 0;
		}
		else
#endif
			retcode = system(sysline);		/* mupfel calling... */
		
		free(sysline);
	}
	return retcode;
}

static void envStartMode(char *name, word *startmode)
{
	char *cp;
	
	if ((cp = getenv(name)) != NULL)
	{
		strupr(cp);
		if (strstr(cp,"W:N") != NULL)
			*startmode |= WCLOSE_START;
		if (strstr(cp,"W:Y") != NULL)
			*startmode &= ~WCLOSE_START;
		if (strstr(cp,"O:N") != NULL)
			*startmode &= ~OVL_START;
		if (strstr(cp,"O:Y") != NULL)
			*startmode |= OVL_START;
	}
}

static void checkStartMode (const char *ext, word *startmode)
{
	char str[MAX_FILENAME_LEN];
	
	if (ext)
	{
		strcpy (str, ext);
		strupr (str);
		if (!strcmp (str, "MUP"))
		{
			*startmode &= ~OVL_START;
		}
	}
}

/*
 * word executer(word startmode,word showpath,char *fname,char *command)
 * execute program "fname" with commandline "command", use
 * showpath to display full path or name only
 * startmode for TOS-Screen or GEM-Background etc.
 */
word executer(word startmode,word showpath,char *fname,char *command)
{
	IconInfo *pii;
	word retcode;
	char myname[MAXLEN];
	char *cp;
	
	getBaseName(myname,fname);
	
	envStartMode((startmode & GEM_START)? 
				"GEMDEFAULT" : "TOSDEFAULT", &startmode);
	
	if ((cp = strchr(myname,'.')) != NULL)
	{
		*cp = '_';
		envStartMode(myname, &startmode);
		checkStartMode (cp+1, &startmode);
	}

	/* das Programm wird als Overlay gestartet.
	 */
	if (startmode & OVL_START)
	{
		setOverlay(fname, command, startmode & GEM_START);
		return 0;
	}
	
#if MERGED	
	if ((startmode & (WCLOSE_START|GEM_START)) == 0)
	{
		return doSystem(fname,command,TRUE, startmode);
	}
#endif
	
	if (!showpath && ((cp = strrchr(fname,'\\')) != NULL))
	{
		cp++;
		strcpy(myname,cp);
	}
	else
		strcpy(myname,fname);
	strupr(myname);

	if(startmode & WCLOSE_START)
		hideWindows(TRUE);

#if THOSE_WERE_THE_DAYS
	if(mupfGem)
		Setscreen(oldlogbase,(void *)-1,-1);
#endif

	makeBackground(startmode,TRUE,myname);
	
	retcode = doSystem(fname, command, FALSE, 0);

#if THOSE_WERE_THE_DAYS
	if(mupfGem)
		Setscreen(round256(scrnbuf),(void *)-1,-1);
#endif

	if (!(startmode & WCLOSE_START))
		allFileChanged(FALSE);			/* read files again */

	makeBackground(startmode,FALSE,NULL);

	if(NewDesk.scrapNr > 0)	/* the following should be functions */
	{
		pii = getIconInfo(&NewDesk.tree[NewDesk.scrapNr]);
		getSPath(pii->path);
		if (getLabel(pii->path[0] - 'A', pii->label))
			updateSpecialIcon(NewDesk.scrapNr);
	}
	if(NewDesk.trashNr > 0)
		updateSpecialIcon(NewDesk.trashNr);
	
	if(startmode & WCLOSE_START)
	{
		clearUndo();		/* clear undobuffer */
		hideWindows(FALSE);
		builtIconHash();
	}

	return retcode;
}


void checkMupfel(void)
{
#if STANDALONE
	if (!system(NULL))
		menu_ienable(pmenu,TOMUPFEL,FALSE);
#endif
}