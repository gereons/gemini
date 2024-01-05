/*
 * @(#) Gemini\Menu.c
 * @(#) Stefan Eissing, 17. M„rz 1991
 *
 * description: menufunktionen zu Venus
 *
 */

#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "menu.rh"
#include "sorticon.rh"
#include "sorttyp.rh"

#include "menu.h"
#include "dialog.h"
#include "dispatch.h"
#include "getinfo.h"
#include "window.h"
#include "redraw.h"
#include "undo.h"
#include "util.h"
#include "venuserr.h"
#include "infofile.h"
#include "applmana.h"
#include "iconrule.h"
#include "select.h"
#include "iconinst.h"
#include "pexec.h"
#include "fileutil.h"
#include "init.h"
#include "stand.h"
#include "wildcard.h"
#include "filedraw.h"
#include "order.h"
#if MERGED
#include "mvwindow.h"
#endif

store_sccs_id(menu);

/* externals
 */
extern ShowInfo show;
extern WindInfo wnull;
extern OBJECT *pmenu,*pcopyinfo;
extern char *sorticonstring, *sorttypstring;

extern short LastTopWindowHandle;

#if MERGED
extern word mupfGem;
#endif

/* internal texts
 */
#define NlsLocalSection "G.menu"
enum NlsLocalText{
T_CONOPEN,		/*Kann das Console-Fenster nicht ”ffnen!*/
T_SAVE,			/*Momentanen Zustand des Desktop abspeichern?*/
T_PRESSKEY		/*Weiter mit Tastendruck...*/
};


#if MERGED

static void updateDirty(void)
{
	unsigned long dirty;
	word i;
	char path[4];
	
	dirty = CommInfo.dirty;
	
	if (!dirty)
		return;
	strcpy(path, "A:\\");
	
	for (i = 0; i < 26; ++i)
	{
		if (legalDrive(i) && (dirty & 1))
		{
			path[0] = 'A' + (char)i;
			buffPathUpdate(path);
		}
		dirty = dirty >> 1;
	}
	flushPathUpdate();
	CommInfo.dirty = 0L;
}

word callMupfel(void)
{
	word retcode;
	char *cmdline;
	
	do
	{
		m_mupfel();
		if (CommInfo.cmd == execPrg)
		{
			char *comm;
			WindInfo *wp;
			
			cmdline = (char *)CommInfo.cmdArgs.cmdspec;
			CommInfo.cmd = neverMind;
			CommInfo.cmdArgs.cmdspec = NULL;
			
			menu_bar(pmenu,FALSE);
			while(*cmdline==' ' || *cmdline=='\t')
				cmdline++;
			comm = strpbrk(cmdline," \t");
			if(comm != NULL)
				*comm++ = '\0';
			mupfGem = TRUE;
			retcode = executer(GEM_START|WCLOSE_START,
						FALSE,cmdline,comm);
			mupfGem = FALSE;
			/* redraw all windows */
			allFileChanged(FALSE);
			allrewindow(NEWWINDOW);
			
			wp = wnull.nextwind;
			while (wp)
			{
				wp->update |= KILLREDRAW;
				wp = wp->nextwind;
			}
		}
		else
		{
			cmdline = NULL;
			
			if (CommInfo.cmd != overlay)
				updateDirty();
			retcode = 0;
		}
	} while(cmdline != NULL);

	return retcode;
}

#endif /* MERGED */

word doMupfel(const char *cmd, word waitkey)
{
	word retcode;
#if MERGED
	WindInfo *mwp;
	char myPath[MAXLEN];
	word startm, closeagain = FALSE;
	word old_top_window = -1;
	void m_mupfel(void);

	if ((mwp = getMupfWp()) != NULL)
	{
		wind_get (0, WF_TOP, &old_top_window);
		
		if (old_top_window != mwp->handle)
		{
			word r[4];
			
			setWpOnTop (mwp->handle);
			LastTopWindowHandle = old_top_window;
			wind_set (mwp->handle, WF_TOP);
			r[0] = mwp->workx;
			r[1] = mwp->worky;
			r[2] = mwp->workw;
			r[3] = mwp->workh;
			drawMWindow (mwp, r);
		}
		else
			old_top_window = -1;
		startm = TRUE;
	}
	else
	{
		getFullPath(myPath);

		if ((startm=openMupfelWindow())==TRUE)
		{
			mwp = getMupfWp();
			LastTopWindowHandle = mwp->handle;
			closeagain = TRUE;
		}
	}

	if (startm)
	{
		
		if (!cmd)				/* no command, just opened */
			return 0;
		
		/*
		 * Set the window's parameters,
		 * build a rectangle list for it !!
		 */
		openMWindow(mwp);
		
		/* Das Fenster wurde ge”ffnet und es wird ein Kommando
		 * ausgefhrt: der ursprngliche Pfad muž wieder gesetzt
		 * werden!
		 */
		if (closeagain)
			setFullPath(myPath);

		if (cmd)
		{
			CommInfo.cmd = execPrg;
			CommInfo.cmdArgs.cmdspec = cmd;
		}
		else
			CommInfo.cmd = neverMind;
		
		retcode = callMupfel();
		
		if (closeagain)
		{
			WindInfo *wp = wnull.nextwind;
			
			while (wp)
			{
				wp->update &= ~KILLREDRAW;
				wp = wp->nextwind;
			}

			if (waitkey)
			{
				int i;
				
				GrafMouse(M_OFF, NULL);

				printf(NlsStr(T_PRESSKEY));
				
				WaitKeyButton();
				
				for (i = 0; i < strlen(NlsStr(T_PRESSKEY)); ++i)
				{
					printf("\b \b");
				}

				GrafMouse(M_ON, NULL);
			}
			closeWindow(mwp->handle, FALSE);
		}
		else if (old_top_window > 0)
		{
			/* Das Console-Fenster war offen, aber nicht oben.
			 * Also bringen wir das vorherige Top-Fenster wieder
			 * dorthin, wo es war.
			 */
			DoTopWindow (old_top_window);
		}
		
	}
	else
		venusErr(NlsStr(T_CONOPEN));
#else
	(void)waitkey;
	if (cmd)
		retcode = *cmd;
	retcode = executer(GEM_START|WCLOSE_START,
					TRUE,"$SHELL",NULL);
#endif
	if(retcode < 0)
		sysError(retcode);

	return retcode;
}

#if MERGED
void doConParm(void)
{
	WindInfo *wp;
	
	if (fontDialog() && ((wp = getMupfWp()) != NULL))
	{
		if(show.aligned)
			wp->windx = charAlign(wp->windx);
		wind_set(wp->handle,WF_CURRXYWH,wp->windx,wp->windy,
				wp->windw,wp->windh);
		wind_get(wp->handle,WF_WORKXYWH,&wp->workx,&wp->worky,
				&wp->workw,&wp->workh);	
		moveMWindow(wp);
	}
}
#endif

static void switchGeneralOptions(word entry)
{
	switch(entry)
	{
		case DISK:				/* install Diskicon */
			doInstDialog();
			break;
		case APPL:
			applDialog();
			break;
		case RULEOPT:
			if(editIconRule())
			{
				allrewindow(SHOWTYPE);
				rehashDeskIcon();
			}
			break;
		case TOMUPFEL:
			doMupfel(NULL, 0);
			break;
		case SAVEWORK:
			if(venusChoice(NlsStr(T_SAVE)))
			{
				makeConfInfo();
				writeInfoDatei(FINFONAME, TRUE);
			}
			break;
		case PDIVERSE:
			if (doDivOptions())
				allFileChanged(TRUE);
			break;
		case PDARSTEL:
			doShowOptions();
			break;
		case PGESPRAE:
			doGeneralOptions();
			break;
		case PPROGRAM:
			doFinishOptions();
			break;
		case PCONSOLE:
#if MERGED
			doConParm();
#endif
			break;
	}
}

/* kmmere dich um die Menueintr„ge */
word doMenu(word mtitel, word mentry, word kstate)
{
	word fwind;

	switch(mtitel)
	{
		case DESK:
			if(mentry==DESKINFO)
			{
				doAboutInfo();
			}
			break;
		case FILES:
			switch(mentry)
			{
				case OPEN:
					simDclick(kstate);
					break;
				case GETINFO:
					getInfoDialog();
					break;
				case FOLDER:
					doNewFolder();
					break;
				case WINDCLOS:
					wind_get(0, WF_TOP, &fwind);
					if ((fwind != 0) && getwp(fwind) != NULL)
						deleteWindow(fwind);
					break;
				case CLOSE:
					wind_get(0, WF_TOP, &fwind);
					if((fwind!=0) && getwp(fwind) != NULL)
						closeWindow(fwind, FALSE);
					break;
				case CYCLEWIN:
					cycleWindow();
					break;
				case DOUNDO:
					doUndo();
					break;
				case SELECALL:
					SelectAllInTopWindow();
					break;
				case INITDISK:
					initDisk();
					break;
			}
			break;
		case SHOW:
			switch(mentry)
			{
				case BYNOICON:
					if(show.showtext || (!show.normicon))
					{
						menu_text(pmenu,SORTTYPE,sorticonstring);
						if(show.showtext)
							menu_icheck(pmenu,BYTEXT,FALSE);
						else
							menu_icheck(pmenu,BYSMICON,FALSE);
						menu_icheck(pmenu,mentry,TRUE);
						show.showtext = FALSE;
						show.normicon = TRUE;
						allrewindow(SHOWTYPE|VESLIDER);
					}
					break;
				case BYSMICON:
					if(show.showtext || show.normicon)
					{
						menu_text(pmenu, SORTTYPE, sorticonstring);
						if(show.showtext)
							menu_icheck(pmenu,BYTEXT,FALSE);
						else
							menu_icheck(pmenu,BYNOICON,FALSE);
						menu_icheck(pmenu,mentry,TRUE);
						show.showtext = FALSE;
						show.normicon = FALSE;
						allrewindow(SHOWTYPE|VESLIDER);
					}
					break;
				case BYTEXT:
					if(!show.showtext)
					{
						menu_text(pmenu,SORTTYPE,sorttypstring);
						if(show.normicon)
							menu_icheck(pmenu,BYNOICON,FALSE);
						else
							menu_icheck(pmenu,BYSMICON,FALSE);
						menu_icheck(pmenu,mentry,TRUE);
						show.showtext = TRUE;
						allrewindow(SHOWTYPE|VESLIDER);
					}
					break;
				case SORTNAME:
				case SORTDATE:
				case SORTTYPE:
				case SORTSIZE:
				case UNSORT:
					if(mentry!=show.sortentry)
					{
						menu_icheck(pmenu,show.sortentry,FALSE);
						menu_icheck(pmenu,mentry,TRUE);
						show.sortentry = mentry;
					
						allrewindow((mentry == SORTTYPE)? SHOWTYPE:0);
					}
					break;
				case ORDEDESK:
					orderDeskIcons();
					break;
				case WILDCARD:
		    			if (wnull.nextwind != NULL)
		    				doWildcard(wnull.nextwind);
	    			break;

			}
			break;
		case OPTIONS:
			switchGeneralOptions(mentry);
			break;
	}
	menu_tnormal(pmenu,mtitel,1);	
	if(mentry == QUIT) return 0;
	return 1;
}

/*
 * static void windEntries(word enabled)
 * change Window dependend menuentries
 */
static void windEntries(word enabled, WindInfo *nwp)
{
	menu_ienable(pmenu, WINDCLOS, enabled);
	menu_ienable(pmenu, CLOSE, enabled);

	if (enabled && nwp)
		menu_ienable(pmenu, FOLDER, (nwp->kind == WK_FILE));
	else
	{
		menu_ienable(pmenu, FOLDER, enabled);
	}
}

static void kindChanged(word kind)
{
	menu_ienable(pmenu,FOLDER,(kind == WK_FILE));
	menu_ienable(pmenu,WILDCARD,(kind == WK_FILE));
}
/*
 * static void iconEntries(word enabled,WindInfo *wp,word objnr)
 * change Icon dependend menuentries and
 * 
 */
static void iconEntries(word thereare, word onlyone,
						WindInfo *wp, word objnr)
{
	IconInfo *pii;
	word isFloppy;
	
	menu_ienable(pmenu, OPEN, onlyone && (objnr > 0));
	menu_ienable(pmenu, GETINFO, thereare);

	if(onlyone)
	{
		switch (wp->kind)
		{
			case WK_DESK:
				pii = getIconInfo(&wp->tree[objnr]);
				isFloppy = (pii->type == DI_FILESYSTEM)
							&& (pii->path[0] < 'C');
				menu_ienable(pmenu,INITDISK,isFloppy);
									/* it is a floppy disk? */
				break;
			default:
				menu_ienable(pmenu,INITDISK,FALSE);
				break;
		}
	}
	else
	{
		menu_ienable(pmenu,INITDISK,FALSE);
	}
}


/*
 * void manageMenu(void)
 * cares about selectable and disabled menu entries
 */
void manageMenu(void)
{
	static word lastwindstate = FALSE;
	static word lastwindanz = FALSE;
	static word lastkind = -1;
	static WindInfo *topwp = NULL;
	WindInfo *wp;
	word windanz, windstate;
	word thereare, onlyone, objnr;

	if (wnull.nextwind == NULL)
	{
		menu_ienable(pmenu, WILDCARD, FALSE);
		lastkind = -1;
	}
	
	if(topwp != wnull.nextwind)
	{
		topwp = wnull.nextwind;

		if (topwp != NULL && (topwp->kind == WK_FILE))
			setFullPath (topwp->path);

		if (topwp && (lastkind != topwp->kind))
		{
			kindChanged (topwp->kind);
			lastkind = topwp->kind;
		}
	}

	windstate = (wnull.nextwind != NULL);

	if (windstate != lastwindstate)		/* state has changed */
	{
		windEntries (windstate, wnull.nextwind);
		lastwindstate = windstate;
		if (windstate)
			lastkind = 0;
	}
	
	windanz = (windstate && (wnull.nextwind->nextwind != NULL));
	if(windanz != lastwindanz)
	{
		menu_ienable (pmenu, CYCLEWIN, windanz);
		lastwindanz = windanz;
	}
	
	onlyone = getOnlySelected (&wp, &objnr);
	thereare = thereAreSelected (&wp);
	iconEntries(thereare, onlyone, wp, objnr);
}

