/*
 * @(#) Gemini\Venus.c
 * @(#) Stefan Eissing, 08. Juni 1991
 *
 * description: main modul for the desktop venus
 *
 */

#include <vdi.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\evntevnt.h>
#include <nls\nls.h>
#include <tos.h>

#include "vs.h"
#include "venus.h"
#include "menu.h"
#include "window.h"
#include "fileutil.h"
#include "filewind.h"
#include "redraw.h"
#include "scroll.h"
#include "init.h"
#include "venuserr.h"
#include "myalloc.h"
#include "keylogi.h"
#include "dispatch.h"
#include "util.h"
#include "infofile.h"
#include "erase.h"
#include "iconinst.h"
#include "icondrag.h"
#include "iconrule.h"
#include "stand.h"
#include "windstak.h"
#include "greeting.h"
#include "filedraw.h"
#include "overlay.h"
#include "message.h"
#if MERGED
#include "mvwindow.h"
#include "conselec.h"
#endif

char venusSCCSid[] = "@(#) " PGMNAME " (C) S. Eissing & G. Steffens "__DATE__;


WindInfo wnull;							/* Struktur fr Window 0 */
ShowInfo show;							/* Wie angezeigt wird */
DeskInfo NewDesk = {NULL,0,0,0,0,0,		/* Desktophintergrund */
					0,0,0,0,0,0,0,0};	/* und allgemeines */

char bootpath[MAXLEN];					/* Pfad beim Programmstart */
word deskx,desky,deskw,deskh;			/* Desktopkoordinaten */
word apid;								/* Application Id */
word phys_handle;						/* Handle von graf_handle */
word wchar,hchar,wbox,hbox;				/* Charakterinformationen */
word handle;							/* eigener VDI-Handle */
word work_in[11] = {1,1,1,1,1,1,1,1,1,1,2};
word work_out[57],pxy[256];	/* VDI Parameter Arrays */
OBJECT *pmenu,*pcopyinfo,*pstdbox;
OBJECT *pshowbox,*pwildbox, *pconselec;
OBJECT *prulebox,*pinstbox,*prenamebox;
OBJECT *pcopybox,*pnamebox,*perasebox;
OBJECT *pfoldbox,*pfileinfo,*pfoldinfo;
OBJECT *pdrivinfo,*papplbox,*pttpbox;
OBJECT *pchangebox,*pshredbox,*ptrashbox;
OBJECT *pscrapbox,*piconfile,*ppopopbox;
OBJECT *pfrmtbox,*pspecinfo,*poptiobox;
OBJECT *pinitbox,*popendbox,*pdivbox;
OBJECT *pfontbox, *pnogdos, *pweditbox;
OBJECT *pruleedit, *pappledit, *pshortcut;
OBJECT *pcolorbox, *pfmtbox, *pnewdesk;
OBJECT *ptotalinf;
char *sorttypstring, *sorticonstring;

/* Handle des zuletzt erkannten TOP-Windows. Ist nach einem Event
 * ein anderes Fensters als dieses TOP, dann werden die Parameter
 * des neuen TOP-Windows gesetzt und LastTopWindow auf selbiges
 * gesetzt.
 */
short LastTopWindowHandle = -1;

word deskAnz,normAnz,miniAnz;

char wildpattern[5][WILDLEN] =
			{"*.[CH]",
			 "*.[AP]??",
			 "*.TOS",
			 "*.TTP",
			 "*.[AB][CD]?"};			/* 5 wildcards fr windows */
char version[] = "Version 1.26  "__DATE__;

#if STANDALONE
char *cpCommand = "noalias cp -d \'";	/* Kommando zum kopieren */
char *mvCommand = "noalias mv \'";		/*     "     "  verschieben */
char *rmCommand = "noalias rm \'";		/*     "     "  l”schen */
#endif

word itsover = FALSE;					/* overlay exit */

FONTWORK filework;		/* fr filedraw */

#if MERGED
word mupfGem = FALSE;
#else
struct CommInfo CommInfo;	/* only needed in standalone version */
#endif

/* internal texts
 */
#define NlsLocalSection "G.Venus.c"
enum NlsLocalText{
T_CONWINDOW,	/*Kann das Console-Fenster nicht initialisieren!*/
T_MUPFEL,	/*Es ist keine Mupfel installiert! Operationen 
wie Kopieren oder L”schen sind nicht m”glich!*/
T_NORSC,		/*Konnte die Resource-Datei %s nicht laden!*/
T_RSC,		/*Es ist ein Fehler in der Resource-Datei %s aufgetreten!*/
T_APPLINIT,	/*[3][Das AES scheint mich nicht zu m”gen!][Abbruch]*/
T_VDI,		/*Kann keine virtuelle Workstation ”ffnen!*/
T_BUTT,		/*[Oh*/
T_ASKQUIT,	/*Wollen Sie %s wirklich verlassen?*/
};

/* Event-Struktur fr evnt_event()
 */
static MEVENT E;

/* kmmere dich um alle Haupt-Events
 */
void multi_event(void)
{
	word evnttype;
	word messbuff[8];
	word ok,fwind;
	WindInfo *wp;

	E.e_flags = MU_BUTTON | MU_MESAG | MU_KEYBD;
	E.e_time = 0L;
	E.e_bclk = 2;
	E.e_bmsk = 1;
	E.e_bst = 1;
	E.e_mepbuf = messbuff;
	ok = 1;
	GrafMouse(ARROW,NULL);

	while(ok)
	{
		UpdateWindowData();
		manageMenu();	/* (de)select Menuentries */

		WindUpdate(END_UPDATE);
		evnttype = evnt_event(&E);

		WindUpdate(BEG_UPDATE);

		wind_get(0, WF_TOP, &fwind);
		if (fwind != LastTopWindowHandle)
		{
			LastTopWindowHandle = fwind;
			SetTopWindowInfo(LastTopWindowHandle);
		}
		
		if (evnttype & MU_MESAG)
		{
			ok = HandleMessage(messbuff, E.e_ks);
		}

		if (evnttype & MU_BUTTON)
		{
			fwind = wind_find(E.e_mx,E.e_my);
			wp = getwp(fwind);
			if (wp != NULL)
			{
				if (E.e_br == 2)			/* Doppelklick */
				{
#if MERGED
					if (wp->kind == WK_MUPFEL)
					{
						WindUpdate(BEG_MCTRL);
						ConSelection(wp, E.e_mx, E.e_my, E.e_ks, TRUE);
						WindUpdate(END_MCTRL);
					}
					else
#endif
						doDclick(E.e_mx, E.e_my, E.e_ks);
				}
				else
				{
					if((E.e_my < wp->worky)
						&&(wp->kind == WK_FILE))
					{
						/* W„hle neuen Wildcard, etc
						 */
						FileWindowSpecials (wp, E.e_mx, E.e_my);
					}
					else
					{
						switch (wp->kind)
						{
							case WK_DESK:
							case WK_FILE:
								WindUpdate(BEG_MCTRL);
								doIcons(wp, E.e_mx, E.e_my, E.e_ks);
								WindUpdate(END_MCTRL);
								break;
							case WK_MUPFEL:
#if MERGED
								WindUpdate(BEG_MCTRL);
								ConSelection(wp, E.e_mx, E.e_my, E.e_ks, FALSE);
								WindUpdate(END_MCTRL);
#endif
								break;
						}
					}
				}
			}
		}
		
		if ((evnttype & MU_MESAG) && (messbuff[0] == 0x4710))	
								/* Tastatur-Nachricht */
		{
			evnttype |= MU_KEYBD;
			E.e_ks = messbuff[3];
			E.e_kr = messbuff[4];
		}
		
		if (evnttype & MU_KEYBD)
		{
			ok = doKeys(E.e_kr, E.e_ks);
		}
		
		if (doOverlay(TMPINFONAME))
		{
			ok = FALSE;
			itsover = TRUE;
		}
		else
			if (!ok && NewDesk.askQuit)
				ok = !venusChoice(NlsStr(T_ASKQUIT), PGMNAME);
	}
}

#if MERGED
word venus(void)
#else
word main(void)
#endif
{
	word ok;
	void *firstResource;
	word msgLoaded = FALSE;
	
	(void)venusSCCSid;
#if STANDALONE
	apid = appl_init();
#else
	apid = _GemParBlk.global[2];
#endif
	if (apid != -1)
	{
		DialInit(Malloc, Mfree);
#if STANDALONE
		greetings();
#endif
	}

	if((apid != -1) && getBootPath(bootpath, MSGFILE))
	{
		GrafMouse(HOURGLASS,NULL);

		addFileName(bootpath, MSGFILE);
		ok = NlsInit(bootpath, Malloc, Mfree);
		stripFileName(bootpath);
		if (ok)
			msgLoaded = TRUE;
		else
			venusErr("Cannot load %s!", MSGFILE);
		
		if (!open_vwork())
		{
			if (msgLoaded)
				DialAlert(ImSqExclamation(), NlsStr(T_VDI),
						0, NlsStr(T_BUTT));
			DialExit();
			NlsExit();
#if STANDALONE
			appl_exit();
#endif
			return 1;
		}
		greetings();
		
		if (ok)
		{
			addFileName(bootpath,ICONRSC);
			ok = initIcons(bootpath);
			stripFileName(bootpath);
			
			if (!ok)
				venusErr(NlsStr(T_NORSC), ICONRSC);
		}
		
		greetings();
		if(ok)
		{
			/* Speichere den Zeiger auf die erste Resource
			 */
			firstResource = *((char **)(&_GemParBlk.global[5]));

			addFileName(bootpath,MAINRSC);
			ok = rsrc_load(bootpath);
			stripFileName(bootpath);
		}
		
		if(ok)
		{
			if(!getTrees())
			{
				venusErr(NlsStr(T_RSC),MAINRSC);
			}
			else
			{
				word tempinfo;

				/* bearbeite die geladenen Icons */
				FixIcons();
				initFileFonts();
#if MERGED
				if (!MWindInit())
				{
					DialAlert(NULL, NlsStr(T_CONWINDOW), 0,
								NlsStr(T_BUTT));
					return 1;
				}
				setInMWindow(&show);
#endif

				greetings ();
				initWindows();
				initNewDesk();
				readInfoDatei(TMPINFONAME, FINFONAME, &tempinfo);
				
				WindUpdate(BEG_UPDATE);
				execConfInfo(TRUE);
				MessInit();
				menu_bar(pmenu,TRUE);

				if(!system(NULL))
					venusErr(NlsStr(T_MUPFEL));
				else if (!tempinfo)
					mupVenus(AUTOEXEC);

				multi_event();

				if ((CommInfo.cmd != overlay) && NewDesk.emptyPaper)
					emptyPaperbasket();
				if (NewDesk.saveState)
				{
					makeConfInfo();
					writeInfoDatei(FINFONAME, FALSE);
				}
					
				MessExit();
				closeAllWind();
				delAccWindows();
				menu_bar(pmenu,FALSE);
				wind_set(0,WF_NEWDESK,NULL);
				form_dial(FMD_FINISH,0,0,0,0,0,0,
							deskx + deskw,desky + deskh);
				WindUpdate(END_UPDATE);
				freeWBoxes();
				freeDeskTree();
				freeTmpBlocks();

				exitFileFonts();
#if MERGED
				MWindExit();
#endif
			}
#if STANDALONE
			freeSysBlocks();
#else
			if (itsover)
				CommInfo.cmd = overlay;
#endif
			rsrc_free();
		}

		if (firstResource)
		{
			*((char **)(&_GemParBlk.global[5])) = firstResource;
			rsrc_free();
		}
		
		if (msgLoaded)
			NlsExit();
			
		v_clsvwk(handle);
		DialExit();
#if STANDALONE
		appl_exit();
#endif
	}
	else
	{
		if (apid == -1)
			form_alert(1,NlsStr(T_APPLINIT));
		else
		{
			DialExit();
			venusErr("Cannot load %s!", MSGFILE);
			appl_exit();
		}
		return 1;
	}
	return 0;
}
