/*
 * @(#) Gemini\Dialog.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: Dialogstuff for Desktop
 *
 */

#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "copyinfo.rh"
#include "opendbox.rh"
#include "foldbox.rh"
#include "optiobox.rh"

#include "color.h"
#include "dialog.h"
#include "fileutil.h"
#include "redraw.h"
#include "stand.h"
#include "util.h"
#include "venuserr.h"
#include "window.h"

store_sccs_id(dialog);

/* externs
*/
extern word handle;
extern word pxy[128];
extern WindInfo wnull;
extern ShowInfo show;
extern DeskInfo NewDesk;
extern OBJECT *pshowbox,*pcopyinfo,*pfoldbox;
extern OBJECT *poptiobox,*popendbox;
extern char cpCommand,mvCommand,rmCommand;

/* internal texts
 */
#define NlsLocalSection "G.dialog"
enum NlsLocalText{
T_SHAREWARE1,	/*%s ist ein Shareware-Programm. Shareware 
bedeutet, daû Sie dieses Programm fÅr nicht-kommerzielle 
Zwecke frei kopieren und testen dÅrfen. Wenn Sie allerdings 
GEMINI/MUPFEL/VENUS regelmÑûig benutzen, so haben Sie sich an das Konzept von 
Shareware zu halten, indem Sie den Autoren einen Obolus von 
50,- DM entrichten. Dies ist KEIN Public-Domain- oder Freeware-
Programm!*/
T_SHAREWARE2, /*Wenn Sie keine Raubkopie benutzen wollen oder 
an der Weiterentwicklung diese Programms interessiert sind, 
kînnen Sie das Geld auf eins unserer Konten 
Åberweisen:|
|Gereon Steffens,
|%sKto. <entfernt>
|Stefan Eissing,
|%sKto. <entfernt>|
|Danke.*/
T_FOLDER,	/*Der Pfad dieses Fensters ist zu lang, um neue Ordner anzulegen!*/
T_NONAME,	/*Sie sollten einen Namen angeben!*/
T_EXISTS,	/*Ein Objekt dieses Namens existiert schon!*/
T_INVALID,	/*Dies ist kein gÅltiger Name fÅr eine Datei 
oder einen Ordner! Bitte wÑhlen Sie einen anderen Namen.*/
T_NOSPACE,	/*Die Auflîsung ist zu gering, um diesen Dialog darzustellen!*/
};


/*
 * word doAboutInfo(void)
 * Dialog about copyright and shareware information
 */
word doAboutInfo(void)
{
	word retcode;
	DIALINFO d;

	pcopyinfo[BANANA].ob_spec.bitblk->bi_color = ColorMap(YELLOW);
#if MERGED
	setHideTree(pcopyinfo, VENUNAME, TRUE);
	setHideTree(pcopyinfo, GEMINAME, FALSE);
	setHideTree(pcopyinfo, GERENAME, FALSE);
	setHideTree(pcopyinfo, ARNDNAME, FALSE);
#endif

	DialCenter(pcopyinfo);
	DialStart(pcopyinfo,&d);
	DialDraw(&d);

	do
	{
		retcode = DialDo(&d,0) & 0x7FFF;
		setSelected(pcopyinfo,retcode,FALSE);
		if (retcode == GMNIINFO)
		{
			if ((venusInfoFollow(NlsStr(T_SHAREWARE1),PGMNAME) < 0)
				|| (venusInfo(NlsStr(T_SHAREWARE2), ALRIGHT, ALRIGHT) < 0))
			{
				venusErr(NlsStr(T_NOSPACE));
				DialDraw(&d);
			}
		}
		fulldraw(pcopyinfo,retcode);
	} while (retcode == GMNIINFO);

	DialEnd(&d);

	return retcode;
}

/*
 * dialog for a new foldername, and trying to create it
 */
void doNewFolder (void)
{
	DIALINFO d;
	TEDINFO *pti;
	WindInfo *wp;
	word ok, retcode;
	char name[MAX_FILENAME_LEN], path[MAXLEN];
	char tmpsave[MAX_FILENAME_LEN], *pfoldname;
	int edit_object = FONAME;
	int wind_handle;
	
	wind_get (0, WF_TOP, &wind_handle);
	
	if ((wind_handle < 0) 
		|| ((wp = getwp (wind_handle)) == NULL)
		|| (wp->kind != WK_FILE))
	{
		venusDebug("This function should be disabled!");
		return;
	}
	
	if (strlen (wp->path) >= MAXLEN-28)
	{
		venusErr (NlsStr (T_FOLDER));
		return;
	}
	pti = pfoldbox[FONAME].ob_spec.tedinfo;
	pfoldname = pti->te_ptext;
	strcpy (tmpsave, pfoldname);
	
	DialCenter (pfoldbox);
	DialStart (pfoldbox, &d);
	DialDraw (&d);
	
	ok = TRUE;
	while (ok)
	{
		int redraw_object;
		
		retcode = DialDo (&d, &edit_object) & 0x7FFF;
		redraw_object = retcode;
		
		switch (retcode)
		{
			case FOCANCEL:
				ok = FALSE;
				break;
			case FOOK:
				if((*pfoldname == '@') || !strlen (pfoldname))
				{
					venusErr (NlsStr (T_NONAME));
					break;
				}
				
				if (!ValidFileName (pfoldname))
				{
					venusErr (NlsStr (T_INVALID));
					break;
				}
				
				GrafMouse (HOURGLASS, NULL);
				strcpy (name, pfoldname);
				makeFullName (name);
				strcpy (path, wp->path);
				addFileName (path, name);
				
				if (dirCreate (path) < 0)
					venusErr (NlsStr (T_EXISTS));
				else
					ok = FALSE;
					
				GrafMouse (ARROW, NULL);
		}

		setSelected (pfoldbox, redraw_object, FALSE);
		fulldraw (pfoldbox, redraw_object);
	}
	
	DialEnd (&d);
	
	strcpy (pfoldname, tmpsave);
	if(retcode == FOOK)
	{
		pathUpdate (wp->path, "");
	}
}

/*
 * void doGeneralOptions(void)
 * Dialog for general options
 */
void doGeneralOptions(void)
{
	word retcode;
	DIALINFO d;

	DialCenter(poptiobox);
	DialStart(poptiobox,&d);
	
	setSelected(poptiobox,OPTEXIST,NewDesk.replaceExisting);
	setSelected(poptiobox,CPVERB,!NewDesk.silentCopy);
	setSelected(poptiobox,RMVERB,!NewDesk.silentRemove);
	
	DialDraw(&d);

	retcode = DialDo(&d,0) & 0x7FFF;
	setSelected(poptiobox,retcode,FALSE);

	DialEnd(&d);

	if(retcode == OPTIOK)
	{
		NewDesk.replaceExisting = isSelected(poptiobox,OPTEXIST);
		NewDesk.silentCopy = !isSelected(poptiobox,CPVERB);
		NewDesk.silentRemove = !isSelected(poptiobox,RMVERB);
	}
}

void doFinishOptions(void)
{
	word retcode;
	DIALINFO d;

	DialCenter(popendbox);
	DialStart(popendbox,&d);
	
	setSelected(popendbox,OPTEMPTY,NewDesk.emptyPaper);
	setSelected(popendbox,OPTWAIT,NewDesk.waitKey);
	setSelected(popendbox,OPTQUEST,NewDesk.askQuit);
	setSelected(popendbox,OPTSAVE,NewDesk.saveState);
	setSelected(popendbox,OPTOVL,NewDesk.ovlStart);
	
	DialDraw(&d);

	retcode = DialDo(&d,0) & 0x7FFF;
	setSelected(popendbox,retcode,FALSE);

	DialEnd(&d);

	if(retcode == OPENDOK)
	{
		NewDesk.emptyPaper = isSelected(popendbox,OPTEMPTY);
		NewDesk.waitKey = isSelected(popendbox,OPTWAIT);
		NewDesk.askQuit = isSelected(popendbox,OPTQUEST);
		NewDesk.saveState = isSelected(popendbox,OPTSAVE);
		NewDesk.ovlStart = isSelected(popendbox,OPTOVL);
	}
}
