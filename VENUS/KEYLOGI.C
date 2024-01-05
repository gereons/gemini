/*
 * @(#) Gemini\Keylogi.c
 * @(#) Stefan Eissing, 17. April 1991
 *
 * description: functions to dispatch keyboard events
 *
 */

#include <ctype.h>
#include <string.h>
#include <tos.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "menu.rh"

#include "scancode.h"
#include "keylogi.h"
#include "redraw.h"
#include "window.h"
#include "util.h"
#include "fileutil.h"
#include "scroll.h"
#include "menu.h"
#include "venuserr.h"
#include "message.h"
#include "stand.h"
#include "scut.h"

store_sccs_id(keylogi);

/* externals
 */
extern WindInfo wnull;
extern word apid;
extern OBJECT *pmenu;

/* internal texts
 */
#define NlsLocalSection "G.keylogi"
enum NlsLocalText{
T_HELP,		/*Hilfe zu den einzelnen Funktionen finden Sie in 
den Dokumenten GEMINI.DOC, VENUS.DOC und MUPFEL.DOC. Falls diese 
Dateien fehlen, wenden Sie sich bitte an die Autoren.*/
};
/* internals
 */
 
typedef struct
{
	word key;				/* auslîsender Key */
	word mess0;				/* Nachrichtennummer */
	word mess3;				/* Nachrichtenwort 3 */
	word mess4;				/* dito			   4 */
	char killrep;			/* number of further kills of this key */
	char console;			/* ist auch fÅr console fenster */
} KeyMessage;

/*
 * Strukturen mit Shortcuts fÅr Alternate-, Control- und ohne diese
 * beiden Tasten.
 */
static KeyMessage altShortCuts[] = 
{
	{'a',		MN_SELECTED,	OPTIONS,APPL,		0,1},
	{'d',		MN_SELECTED,	OPTIONS,DISK,		0,1},
	{'f',		MN_SELECTED,	OPTIONS,RULEOPT,	0,1},
	{'g',		MN_SELECTED,	OPTIONS,PGESPRAE,	0,1},
	{'i',		MN_SELECTED,	OPTIONS,PDARSTEL,	0,1},
	{'m',		MN_SELECTED,	OPTIONS,TOMUPFEL,	0,1},
	{'o',		MN_SELECTED,	SHOW,	ORDEDESK,	0,1},
	{'p',		MN_SELECTED,	OPTIONS,PPROGRAM,	0,1},
	{'s',		MN_SELECTED,	OPTIONS,SAVEWORK,	0,1},
	{'v',		MN_SELECTED,	OPTIONS,PDIVERSE,	0,1},
	{'w',		MN_SELECTED,	OPTIONS,PCONSOLE,	0,1},
};

static KeyMessage normalShortCuts[] = 
{
	{UNDO,		MN_SELECTED,	FILES,	DOUNDO,		1,1},
	{BACKSPACE,	MN_SELECTED,	FILES,	CLOSE,		0,0},
	{DELETE,	MN_SELECTED,	FILES,	WINDCLOS,	10,0},
	{CUR_UP,	WM_ARROWED,		0,		WA_UPLINE,	0,0},
	{CUR_DOWN,	WM_ARROWED,		0,		WA_DNLINE,	0,0},
	{SHFT_CU,	WM_ARROWED,		0,		WA_UPPAGE,	0,0},
	{SHFT_CD,	WM_ARROWED,		0,		WA_DNPAGE,	0,0},
	{HOME,		WM_VSLID,		0,		0,			0,0},
	{SHFT_HOME,	WM_VSLID,		0,		1000,		0,0},
	{'\t',		MN_SELECTED,	FILES,	GETINFO,	0,1},
};

static KeyMessage controlShortCuts[] = 
{
	{'A',	MN_SELECTED,	FILES,	SELECALL,	1,1},
	{'B',	MN_SELECTED,	SHOW,	BYNOICON,	10,1},
	{'C',	WM_CLOSED,		0,		0,			10,0},
	{'D',	MN_SELECTED,	FILES,	WINDCLOS,	10,1},
	{'E',	MN_SELECTED,	FILES,	INITDISK,	0,1},
	{'F',	WM_FULLED,		0,		0,			10,1},
	{'H',	MN_SELECTED,	SHOW,	SORTNAME,	10,1},
	{'I',	MN_SELECTED,	FILES,	GETINFO,	0,1},
	{'J',	MN_SELECTED,	SHOW,	SORTDATE,	10,1},
	{'K',	MN_SELECTED,	SHOW,	SORTSIZE,	10,1},
	{'L',	MN_SELECTED,	SHOW,	SORTTYPE,	10,1},
	{'N',	MN_SELECTED,	FILES,	FOLDER,		0,1},
	{'O',	MN_SELECTED,	FILES,	OPEN,		10,1},
	{'P',	MN_SELECTED,	SHOW,	WILDCARD,	0,1},
	{'Q',	MN_SELECTED,	FILES,	QUIT,		0,1},
	{'S',	MN_SELECTED,	SHOW,	BYSMICON,	10,1},
	{'T',	MN_SELECTED,	SHOW,	BYTEXT,		10,1},
	{'U',	MN_SELECTED,	FILES,	WINDCLOS,	10,1},
	{'W',	MN_SELECTED,	FILES,	CYCLEWIN,	10,1},
	{'X',	MN_SELECTED,	SHOW,	UNSORT,		10,0},
	{'Z',	MN_SELECTED,	OPTIONS,TOMUPFEL,	0,0},
};


#if MERGED
/*
 * Schaue nach, ob es eine belegte Funktiontaste ist
 */
word isValidFunctionKey(word kreturn)
{
	word fnr;
	
	if ((kreturn >= F1) && (kreturn <= F10))
	{
		fnr = ((kreturn - F1) / 256) + 1;
	}
	else if ((kreturn >= SHFT_F1) && (kreturn <= SHFT_F10))
	{
		fnr = ((kreturn - SHFT_F1) / 256) + 11;
	}
	else
		return FALSE;
	
	return (CommInfo.fkeys[fnr] != NULL);
}
#endif

static KeyMessage *handleNormalKey (word kreturn)
{
	short i;
	
	for (i=0; i < DIM(normalShortCuts); ++i)
	{
		/* Soll der Scancode auch betrachtet werden ?
		 */
		if ((normalShortCuts[i].key & 0xFF00) != 0)
		{
			if (normalShortCuts[i].key == kreturn)
				return &(normalShortCuts[i]);
		}
		else
		{
			if (normalShortCuts[i].key == (kreturn & 0xFF))
				return &(normalShortCuts[i]);
		}
	}
	return NULL;
}

static KeyMessage *handleControlKey (word kreturn)
{
	short i;
	
	for (i=0; i < DIM(controlShortCuts); ++i)
	{
		if ((controlShortCuts[i].key - '@') == (kreturn & 0xFF))
			return &(controlShortCuts[i]);
	}
	return NULL;
}

static KeyMessage *handleAltKey (word kreturn)
{
	KEYTAB *key_table;
	short i;
	
	key_table = Keytbl ((void *)-1, (void *)-1, (void *)-1);
	kreturn = kreturn >> 8;
	if ((kreturn < 0) || (kreturn > 0x7F))
		return NULL;
		
	for (i=0; i < DIM(altShortCuts); ++i)
	{
		if (altShortCuts[i].key == key_table->unshift[kreturn])
			return &(altShortCuts[i]);
	}
	return NULL;
}

/*
 * manage keyboard events
 */
word doKeys (word kreturn, word kstate)
{
	WindInfo *wp, *topwp;
	word gueltig,i,killrepeat,console;
	word messbuff[8],topmup, ok;
	word topwindow, receiver;
	KeyMessage *key_message;
	char c;

	ok = TRUE;
	killrepeat = 0;
	messbuff[1] = apid;
	messbuff[2] = 0;
	console = gueltig = FALSE;
	receiver = apid;

	wind_get(0, WF_TOP, &topwindow);
	topwp = getwp(topwindow);
	topmup = ((topwp) && (topwp->kind == WK_MUPFEL));
	
	c = kreturn & 0xFF;
	if ((c == 0) && (kstate & K_ALT))
		key_message = handleAltKey (kreturn);
	else if (kstate & K_CTRL)
		key_message = handleControlKey (kreturn);
	else
		key_message = handleNormalKey (kreturn);
		
	if (key_message)
	{
		gueltig = TRUE;
		messbuff[0] = key_message->mess0;
		messbuff[3] = key_message->mess3;
		messbuff[4] = key_message->mess4;
		messbuff[5] = 0;
		killrepeat = key_message->killrep;
		console = key_message->console;
	}	
		
	/* Wenn ein Shortcut vorlag (gueltig) und das Console-Fenster
	 * oben ist (topmup), ÅberprÅfe, ob der Shortcut auch bei
	 * der Console gÅltig ist oder durchgelassen werden muû.
	 */
	if (gueltig && topmup)
	{
		if ((kreturn & 0xFF) == (CNTRL_I & 0xFF)) /* Ist ^I oder TAB */
		{
			/* Wenn es Control war und auch bei der Console wirkt,
			 * ist es weiterhin gueltig
			 */
			console = (kstate & K_CTRL) && console;
		}

		gueltig = console;
	}
	
	/* Wenn es ein gÅltiger Shortcut war, erzeuge die entsprechende
	 * Nachricht.
	 */
	if(gueltig)
	{
		switch(messbuff[0])
		{
			case MN_SELECTED:
				/* PrÅfe, ob der MenÅeintrag anwÑhlbar ist
				 */
				gueltig = !isDisabled(pmenu,messbuff[4]);
				/* Wenn ja, invertiere den MenÅtitel
				 */
				if (gueltig)
					menu_tnormal(pmenu,messbuff[3],0);
				break;
			case WM_FULLED:
			case WM_ARROWED:
			case WM_VSLID:
			case WM_HSLID:
			case WM_SIZED:
			case WM_MOVED:
			case WM_NEWTOP:
			case WM_TOPPED:
			case WM_CLOSED:
			case WM_REDRAW:
				/* Wenn eins von unseren Fenstern oben ist...
				 */
				if (topwp)
				{
					if (!messbuff[3])
						messbuff[3] = topwindow;
					gueltig = ((topwp->kind == WK_FILE)
								|| (topwp->kind == WK_MUPFEL)
								|| (topwp->kind == WK_ACC));
					receiver = topwp->owner;
				}
				else
					gueltig = FALSE;
				break;
			default:
				break;
		}

		/* Weiterhin ein gÅltiger Shortcut? Dann bearbeite oder
		 * verschicke die Nachricht je nachdem, an wen sie gerichtet
		 * ist.
		 */
		if (gueltig)
		{
			if (receiver == apid)
				ok = HandleMessage(messbuff, kstate);
			else
				appl_write(receiver, 16, messbuff);

			if(killrepeat > 0)
				killEvents(MU_KEYBD, killrepeat);
			return ok;
		}
	}

#if MERGED
	/* Liegt vielleicht eine belegte Funktionstaste vor?
	 */
	if (isValidFunctionKey(kreturn))
	{
		/* Ja, îffne das Console-Fenster...
		 */
		doMupfel(NULL, 0);
		/* und setzte die Variablen, als wenn es schon vorher
		 * oben gewesen wÑre
		 */
		wind_get(0, WF_TOP, &topwindow);
		topwp = getwp(topwindow);
		topmup = ((topwp) && (topwp->kind == WK_MUPFEL));
	}
#endif

	/* HELP-Taste?
	 */
	if (!topmup && (kreturn == HELP))
	{
		venusInfo(NlsStr(T_HELP));
		return ok;
	}
	else if ((topwp) && (topwp->kind == WK_FILE))
	{
		/* Oberstes Fenster ist eins von unseren Dateifenstern
		 */
		switch (kreturn & 0xFF)
		{
			/* Escape: lies den Inhalt neu ein; erzeuge einen
			 * Mediachange.
			 */
			case 0x1B:
				wind_get (0,WF_TOP, &i);
				if (((wp = getwp (i)) != NULL)
					&& (wp->kind == WK_FILE))
				{
					word tmpnum;
					MFORM tmpform;
					
					GrafGetForm (&tmpnum, &tmpform);
					GrafMouse (HOURGLASS, NULL);
					forceMediaChange (wp->path[0] - 'A');
					pathchanged (wp);
					GrafMouse (tmpnum, &tmpform);
					killrepeat = 20;
				}
				break;
			default:
				/* Ist es ein Shortcut fÅr Icons?
				 */
				if (DoSCut(kstate, kreturn))
				{
					UpdateWindowData();
					return TRUE;
				}
				
				/* Wenn es ein alphanumerisches Zeichen ist, scrolle
				 * an die entsprechende Position...
				 */	
				c = toascii(kreturn);
				if(!(kstate & (K_CTRL|K_ALT)) && (isalnum(c)))
				{
					charScroll(c,kstate);
					killrepeat = 5;
					UpdateWindowData();
								/* scroll window to char c */
				}
				break;
		}
	}
	else if (DoSCut(kstate, kreturn))
	{
		/* Kein eigenes Fenster oben, aber einen Shortcut fÅr
		 * ein Icon ausgefÅhrt...
		 */
		UpdateWindowData();
		return TRUE;
	}
	else if (topmup && !console)
	{
		/* Hier haben wir also bisher nichts gefunden und das
		 * Console-Fenster oben liegen. Also fÅtterm wir die
		 * Mupfel mit dieser Taste...
		 */
#if MERGED
		CommInfo.cmd = feedKey;
		CommInfo.cmdArgs.key = ((long)kreturn & 0xFF00L) << 8;
		CommInfo.cmdArgs.key |= (long)(kreturn &0x00FF);
		
		callMupfel();
		killrepeat = 0;
#else
		menu_tnormal(pmenu,OPTIONS,0);
		messbuff[0] = MN_SELECTED;
		messbuff[3] = OPTIONS;
		messbuff[4] = TOMUPFEL;
		killrepeat  = 5;
		appl_write(apid,16,messbuff);
#endif
	}
	
	/* Sollten Tastaturevents (nachlaufendes Keyboard) gelîscht
	 * werden?
	 */
	if(killrepeat > 0)
		killEvents(MU_KEYBD,killrepeat);
	
	return ok;
}
