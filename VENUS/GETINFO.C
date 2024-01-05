/*
 * @(#) Gemini\Getinfo.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: handles dialogue for the 'get Info...' option
 * 				in venus
 *
 */

#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "fileinfo.rh"
#include "foldinfo.rh"
#include "drivinfo.rh"
#include "iconfile.rh"
#include "specinfo.rh"
#include "totalinf.rh"
#include "getinfo.h"
#include "util.h"
#include "fileutil.h"
#include "venuserr.h"
#include "select.h"
#include "redraw.h"
#include "iconinst.h"
#include "applmana.h"
#include "scut.h"
#include "window.h"
#include "undo.h"
#include "iconrule.h"
#include "files.h"
#include "stand.h"

store_sccs_id(getinfo);

/* externals
 */
extern WindInfo wnull;
extern DeskInfo NewDesk;
extern word wchar;
extern OBJECT *pfileinfo, *pfoldinfo, *pdrivinfo;
extern OBJECT *piconfile, *pspecinfo, *ptotalinf;

/* internal texts
 */
#define NlsLocalSection "G.getinfo"
enum NlsLocalText{
T_FILENAME,		/*Sie mssen der Datei einen Namen geben!*/
T_ATTRIB,		/*Kann die Attribute (Schreibschutz) der 
Datei nicht „ndern!*/
T_FOLDER,		/*Sie mssen dem Ordner einen Namen geben!*/
T_DISKERR,		/*Kann keine vernnftigen Daten von dem Laufwerk 
lesen! Bitte berprfen Sie das Medium!*/
T_FOLDABOVE,	/*Dieses Object repr„sentiert den Ordner, der 
ber dem gerade aktiven Ordner liegt.*/
T_RENAME,		/*Kann die Datei nicht zu %s umbenennen!*/
T_ILLDRIVE,		/*Das Laufwerk %c: ist GEMDOS nicht bekannt!*/
T_ILLNAME,		/*Der Name %s ist nicht gltig, da er die 
Zeichen *?: nicht enthalten darf!*/
FILE_COUNT_ERR,	/*Fehler beim Lesen der Datei-Informationen!*/
};

/*
 * static void infoFileDialog(WindInfo *wp, FileInfo *pf)
 *
 * information about a file shown in a dialog
 */
static void infoFileDialog(WindInfo *wp, FileInfo *pf)
{
	DIALINFO d;
	word retcode, ok, update = FALSE;
	char *pfliname, *pflisize, *pflidate, *pflitime;
	char tmpstr[MAXLEN], tmp2[MAXLEN];
	char oldname[MAX_FILENAME_LEN];
	
	pfliname = pfileinfo[FLINAME].ob_spec.tedinfo->te_ptext;
	pflisize = pfileinfo[FLISIZE].ob_spec.tedinfo->te_ptext;
	pflidate = pfileinfo[FLIDATE].ob_spec.tedinfo->te_ptext;
	pflitime = pfileinfo[FLITIME].ob_spec.tedinfo->te_ptext;

	strcpy(tmpstr, pf->fullname);
	makeEditName(tmpstr);
	strcpy(pfliname, tmpstr);
	strcpy(oldname, pfliname);		/* alten Namen merken */
	sprintf(pflisize, "%9ld", pf->size);
	dateString(pflidate, pf->date);
	timeString(pflitime, pf->time);

	setSelected(pfileinfo, FILEREON, pf->attrib & FA_RDONLY);
	setSelected(pfileinfo, FILEHIDD, pf->attrib & FA_HIDDEN);
	setSelected(pfileinfo, FILESYST, pf->attrib & FA_SYSTEM);
	setSelected(pfileinfo, FILEARCH, pf->attrib & FA_ARCHIV);
	
	DialCenter(pfileinfo);
	DialStart(pfileinfo, &d);
	DialDraw(&d);
	
	do
	{
		retcode = DialDo(&d, 0) & 0x7FFF;
		setSelected(pfileinfo, retcode, FALSE);
		fulldraw(pfileinfo, retcode);
		ok = (strlen(pfliname) != 0) || (retcode != FLIOK);
		if(!ok)
			venusErr(NlsStr(T_FILENAME));
		if (retcode == FLIOK)
		{
			char name[MAX_FILENAME_LEN];
			
			strcpy(name, pfliname);
			makeFullName(name);
			if(!ValidFileName(name))
			{
				ok = FALSE;
				venusErr(NlsStr(T_ILLNAME), name);
			}
		}

	} while (!ok);
	DialEnd(&d);
	
	if(retcode == FLIOK)
	{
		word newattrib = pf->attrib;
		
		if (isSelected(pfileinfo, FILEREON))
			newattrib |= FA_RDONLY;
		else
			newattrib &= ~FA_RDONLY;
		if (isSelected(pfileinfo, FILEHIDD))
			newattrib |= FA_HIDDEN;
		else
			newattrib &= ~FA_HIDDEN;
		if (isSelected(pfileinfo, FILESYST))
			newattrib |= FA_SYSTEM;
		else
			newattrib &= ~FA_SYSTEM;
		if (isSelected(pfileinfo, FILEARCH))
			newattrib |= FA_ARCHIV;
		else
			newattrib &= ~FA_ARCHIV;
			
		if(strcmp(oldname, pfliname))	/* Name ver„ndert */
		{
			word change = FALSE;
			
			GrafMouse(HOURGLASS, NULL);
			strcpy(oldname, pfliname);
			makeFullName(oldname);
			strcpy(tmpstr, wp->path);
			addFileName(tmpstr, oldname);
			strcpy(tmp2, wp->path);
			addFileName(tmp2, pf->fullname);
			
			if (pf->attrib & FA_RDONLY)
			{
				/* Schreibschutz aus */
				pf->attrib &= ~FA_RDONLY;
				change = TRUE;
			}
			
			if ((change && (setFileAttribut(tmp2, pf->attrib) < 0))
				|| (fileRename(tmp2, tmpstr) != 0))
			{
				venusErr(NlsStr(T_RENAME), oldname);
				strcpy(oldname, pf->fullname);
			}
			else
			{
				fileWasMoved(tmp2, tmpstr);
				ApRenamed(oldname, pf->fullname, wp->path);
				rehashDeskIcon();
			}

			update = TRUE;
		}
		else
			strcpy(oldname, pf->fullname);

		if(pf->attrib != newattrib)
		{
			update = TRUE;
			GrafMouse(HOURGLASS, NULL);
			strcpy(tmpstr, wp->path);
			addFileName(tmpstr, oldname);
			if(setFileAttribut(tmpstr, newattrib) < 0)
				venusErr(NlsStr(T_ATTRIB));
			else
				pf->attrib = newattrib;
		}

		if (update)
		{
			pathUpdate(wp->path, "");
			stringSelect(wp, oldname, TRUE);
		}
	}
	GrafMouse(ARROW, NULL);
}

/*
 * static void infoFolderDialog(WindInfo *wp, FileInfo *pf)
 *
 * information about a folder shown in a dialog
 */
static void infoFolderDialog(WindInfo *wp, FileInfo *pf)
{
	DIALINFO d;
	uword fileanz, foldanz;
	word retcode, ok, startedit;
	long size;
	char *pfdiname, *pfdisize, *pfdifilenr;
	char *pfditime, *pfdidate, *pfdifoldnr;
	char tmpstr[MAXLEN], tmp2[MAXLEN], oldname[MAX_FILENAME_LEN];
	
	if (tosVersion() < 0x0104)
	{
		pfoldinfo[FDINAME].ob_flags &= ~EDITABLE;
		setHideTree(pfoldinfo, FDICANC, TRUE);
		startedit = 0;
	}
	else
		startedit = FDINAME;
		 
	pfdiname = pfoldinfo[FDINAME].ob_spec.tedinfo->te_ptext;
	pfdisize = pfoldinfo[FDISIZE].ob_spec.tedinfo->te_ptext;
	pfdifilenr = pfoldinfo[FDIFILNR].ob_spec.tedinfo->te_ptext;
	pfdifoldnr = pfoldinfo[FDIFOLNR].ob_spec.tedinfo->te_ptext;
	pfdidate = pfoldinfo[FDIDATE].ob_spec.tedinfo->te_ptext;
	pfditime = pfoldinfo[FDITIME].ob_spec.tedinfo->te_ptext;
	
	GrafMouse(HOURGLASS, NULL);
	size = foldanz = fileanz = 0;
	strcpy(tmpstr, wp->path);
	addFolderName(tmpstr, pf->fullname);
	addFileAnz(tmpstr, &foldanz, &fileanz, &size);
	sprintf(pfdifoldnr, "%5u", foldanz);
	sprintf(pfdifilenr, "%5u", fileanz);
	sprintf(pfdisize, "%9ld", size);
	dateString(pfdidate, pf->date);
	timeString(pfditime, pf->time);
	strcpy(tmpstr, pf->fullname);
	makeEditName(tmpstr);
	strcpy(pfdiname, tmpstr);
	strcpy(oldname, pfdiname);
	GrafMouse(ARROW, NULL);
	
	DialCenter(pfoldinfo);
	DialStart(pfoldinfo, &d);
	DialDraw(&d);
	
	do
	{
		retcode = DialDo(&d, &startedit) & 0x7FFF;
		setSelected(pfoldinfo, retcode, FALSE);
		fulldraw(pfoldinfo, retcode);
		ok = (strlen(pfdiname) != 0) || (retcode != FDIOK);
		if(!ok)
			venusErr(NlsStr(T_FOLDER));
		if (retcode == FDIOK)
		{
			char name[MAX_FILENAME_LEN];
			
			strcpy(name, pfdiname);
			makeFullName(name);
			if(!ValidFileName(name))
			{
				ok = FALSE;
				venusErr(NlsStr(T_ILLNAME), name);
			}
		}

	} while(!ok);

	DialEnd(&d);
	
	if(retcode == FDIOK)
	{
		if(strcmp(oldname, pfdiname))	/* Name ver„ndert */
		{
			GrafMouse(HOURGLASS, NULL);
			strcpy(oldname, pfdiname);
			makeFullName(oldname);
			strcpy(tmpstr, wp->path);
			addFileName(tmpstr, oldname);
			strcpy(tmp2, wp->path);
			addFileName(tmp2, pf->fullname);
			if(fileRename(tmp2, tmpstr) < 0)
				venusErr(NlsStr(T_RENAME), oldname);
			else
			{
				fileWasMoved(tmp2, tmpstr);
				UndoFolderVanished(tmp2);
				pathUpdate(wp->path, "");
				stringSelect(wp, oldname, TRUE);
				rehashDeskIcon();
			}
			GrafMouse(ARROW, NULL);
		}
	}
}

/*
 * static void infoDriveDialog(IconInfo *pii, word objnr)
 *
 * information about a Drive shown in a dialog
 */
static void infoDriveDialog(IconInfo *pii, word objnr)
{
	DIALINFO d;
	uword fileanz, foldanz;
	word retcode;
	word shortcut, circle;
	long size;
	long bytesfree, bytesused;
	char *pfdiname, *pfdifree, *pfdiused, *pfdifilenr;
	char *pdrivechar, *pfdifoldnr, *pdrvname;
	char tmpstr[MAXLEN], oldlabel[MAX_FILENAME_LEN];
	int edit_object = DRVNAME;

	pdrivechar = pdrivinfo[DRVDRIVE].ob_spec.tedinfo->te_ptext;
	pfdiname = pdrivinfo[DRVLABEL].ob_spec.tedinfo->te_ptext;
	pfdiused = pdrivinfo[DRVSIZE].ob_spec.tedinfo->te_ptext;
	pfdifree = pdrivinfo[DRVFREE].ob_spec.tedinfo->te_ptext;
	pfdifilenr = pdrivinfo[DRVFILNR].ob_spec.tedinfo->te_ptext;
	pfdifoldnr = pdrivinfo[DRVFOLNR].ob_spec.tedinfo->te_ptext;
	pdrvname = pdrivinfo[DRVNAME].ob_spec.tedinfo->te_ptext;

	GrafMouse(HOURGLASS, NULL);
	strcpy(pdrvname, pii->iconname);
	size = foldanz = fileanz = 0;
	pdrivechar[0] = pii->path[0];
	pdrivechar[1] = '\0';

	if (!legalDrive(pii->path[0] - 'A'))
	{
		GrafMouse(ARROW, NULL);
		venusErr(NlsStr(T_ILLDRIVE), pii->path[0]);
		return;
	}
	if(!getLabel(pii->path[0] - 'A', tmpstr))
	{
		GrafMouse(ARROW, NULL);
		return;
	}
	if(strlen(tmpstr))
	{
		makeEditName(tmpstr);
	}
	strcpy(pfdiname, tmpstr);
	strcpy(oldlabel, tmpstr);
	
	strcpy(tmpstr, pii->path);
	if(!addFileAnz(tmpstr, &foldanz, &fileanz, &size))
	{
		GrafMouse(ARROW, NULL);
		return;
	}
	sprintf(pfdifoldnr, "%5u", foldanz);
	sprintf(pfdifilenr, "%5u", fileanz);
	
	if (!getDiskSpace(pii->path[0] - 'A', &bytesused, &bytesfree))
	{
		GrafMouse(ARROW, NULL);
		venusErr(NlsStr(T_DISKERR));
		return;
	}
	sprintf(pfdiused, "%9ld", bytesused);
	sprintf(pfdifree, "%9ld", bytesfree);
	GrafMouse(ARROW, NULL);
	
	shortcut = pii->shortcut;
	pdrivinfo[DRVSC].ob_spec = SCut2Obspec(shortcut);
	
	DialCenter(pdrivinfo);
	DialStart(pdrivinfo, &d);
	DialDraw(&d);
	
	do
	{
		circle = FALSE;
		retcode = DialDo(&d, &edit_object) & 0x7FFF;
		
		if (retcode == DRVSC0)
		{
			retcode = DRVSC;
			circle = TRUE;
		}
		if ((retcode == DRVSC) || (retcode == DRVSCSC))
		{
			word retcut;
			
			retcut = SCutSelect(pdrivinfo, DRVSC,
								shortcut, pii->shortcut, circle);
			if (retcut != shortcut)
			{
				shortcut = retcut;
				pdrivinfo[DRVSC].ob_spec = SCut2Obspec(shortcut);
				fulldraw(pdrivinfo, ICONSC);
			}
		}

		if (retcode == DRVOK)
		{
			char label[MAX_FILENAME_LEN];
			
			strcpy(label, pfdiname);
			makeFullName(label);
			if ((strlen(label)) && !ValidFileName(label))
			{
				venusErr(NlsStr(T_ILLNAME), label);
				retcode = DRVSC;
				setSelected(pdrivinfo, DRVOK, FALSE);
				fulldraw(pdrivinfo, DRVOK);
			}
		}
	}
	while ((retcode == DRVSC) || (retcode == DRVSCSC));

	setSelected(pdrivinfo, retcode, FALSE);
	fulldraw(pdrivinfo, retcode);
	DialEnd(&d);
	
	if(retcode == DRVOK)
	{
		if(strcmp(pdrvname, pii->iconname))	/* name ver„ndert */
		{
			strcpy(pii->iconname, pdrvname);
			SetIconTextWidth(&pii->ib);
			
			redrawObj(&wnull, objnr);
			flushredraw();
		}
		
		if(strcmp(oldlabel, pfdiname))		/* label ver„ndert */
		{
			char newlabel[MAX_FILENAME_LEN];
			
			GrafMouse(HOURGLASS, NULL);
			strcpy(newlabel, pfdiname);
			makeFullName(newlabel);
#ifdef MERGED
			{
				char *argv[3];
				char drive[2];

				argv[0] = "label";
				drive[0] = pii->path[0];
				drive[1] = '\0';
				argv[1] = drive;
				argv[2] = newlabel;
				retcode = MupfelCommand(LABEL, 3, argv);
			}
#else
			sprintf(tmpstr, "noalias label %c '%s'",
					*pii->path, strlen(newlabel)? newlabel : "-");
			retcode = system(tmpstr);
#endif
			if(retcode != 0)
				sysError(retcode);
			else
			{
				makeFullName (oldlabel);
				WindNewLabel(pii->path[0], oldlabel, newlabel);
				DeskIconNewLabel(pii->path[0], oldlabel, newlabel);
				ApNewLabel(pii->path[0], oldlabel, newlabel);
			}

			GrafMouse(ARROW, NULL);
		}
		
		if (shortcut != pii->shortcut)
		{
			SCutRemove(pii->shortcut);
			SCutInstall(shortcut);
			pii->shortcut = shortcut;
		}
	}
}

static void doIconInfo(IconInfo *pii, word objnr)
{
	DIALINFO d;
	word retcode, shortcut, circle;
	long old_spec;
	size_t len;
	char fname[MAX_FILENAME_LEN], label[MAX_FILENAME_LEN];
	char path[MAXLEN];
	int edit_object = ICONNAME;

	setHideTree(piconfile, ICONALIA, pii->type == DI_FOLDER);
	setHideTree(piconfile, ICONFOLD, pii->type != DI_FOLDER);
	
	strcpy(fname, pii->iconname);
	piconfile[ICONNAME].ob_spec.tedinfo->te_ptext = fname;
	old_spec = piconfile[ICONPATH].ob_spec.index;
	
	strcpy(path, pii->path);
	len = piconfile[ICONPATH].ob_width / wchar;
	setHideTree(piconfile, ICONETC, len >= strlen(path));
	path[len] = '\0';

	piconfile[ICONPATH].ob_spec.free_string = path;
	strcpy(label, pii->label);
	makeEditName(label);
	piconfile[ICONLABL].ob_spec.tedinfo->te_ptext = label;
	setHideTree(piconfile, ICONLABL, strlen(label) == 0);
	setHideTree(piconfile, ICONNOLA, strlen(label) != 0);

	shortcut = pii->shortcut;
	piconfile[ICONSC].ob_spec = SCut2Obspec(shortcut);
		
	DialCenter(piconfile);
	DialStart(piconfile, &d);
	DialDraw(&d);
	
	do
	{
		circle = FALSE;
		retcode = DialDo(&d, &edit_object) & 0x7FFF;
		
		if (retcode == ICONSC0)
		{
			retcode = ICONSC;
			circle = TRUE;
		}
		if ((retcode == ICONSC) || (retcode == ICONSCSC))
		{
			word retcut;
			
			retcut = SCutSelect(piconfile, ICONSC,
									shortcut, pii->shortcut, circle);
			if (retcut != shortcut)
			{
				shortcut = retcut;
				piconfile[ICONSC].ob_spec = SCut2Obspec(shortcut);
				fulldraw(piconfile, ICONSC);
			}
		}
	}
	while ((retcode == ICONSC) || (retcode == ICONSCSC));
	
	setSelected(piconfile, retcode, FALSE);
	
	piconfile[ICONPATH].ob_spec.index = old_spec;
	
	DialEnd(&d);
	if(retcode == ICONOK)
	{
		if (shortcut != pii->shortcut)
		{
			SCutRemove(pii->shortcut);
			SCutInstall(shortcut);
			pii->shortcut = shortcut;
		}
		if(strlen(fname) && strcmp(pii->iconname, fname))
		{
			strcpy(pii->iconname, fname);
			SetIconTextWidth(&pii->ib);
			
			redrawObj(&wnull, objnr);
			flushredraw();
		}
	}
}

static void doSpecialInfo(IconInfo *pii, word objnr)
{
	DIALINFO d;
	uword fileanz, foldanz;
	word retcode;
	word shortcut, circle;
	long size;
	char *pname, *psize, *pfilenr;
	char *pfoldnr;
	char tmpstr[MAXLEN];
	int edit_object = SPINAME;
	
	pname = pspecinfo[SPINAME].ob_spec.tedinfo->te_ptext;
	psize = pspecinfo[SPISIZE].ob_spec.tedinfo->te_ptext;
	pfilenr = pspecinfo[SPIFILES].ob_spec.tedinfo->te_ptext;
	pfoldnr = pspecinfo[SPIFOLDE].ob_spec.tedinfo->te_ptext;
	
	GrafMouse(HOURGLASS, NULL);
	size = foldanz = fileanz = 0;
	strcpy(tmpstr, pii->path);
	if(!addFileAnz(tmpstr, &foldanz, &fileanz, &size))
	{
		GrafMouse(ARROW, NULL);
		return;
	}
	sprintf(pfoldnr, "%5u", foldanz);
	sprintf(pfilenr, "%5u", fileanz);
	sprintf(psize, "%9ld", size);
	strcpy(pname, pii->iconname);
	
	shortcut = pii->shortcut;
	pspecinfo[SPISC].ob_spec = SCut2Obspec(shortcut);
	
	GrafMouse(ARROW, NULL);
	
	DialCenter(pspecinfo);
	DialStart(pspecinfo, &d);
	DialDraw(&d);
	
	do
	{
		circle = FALSE;
		retcode = DialDo (&d, &edit_object) & 0x7FFF;
		
		if (retcode == SPISC0)
		{
			retcode = SPISC;
			circle = TRUE;
		}
		if ((retcode == SPISC) || (retcode == SPISCSC))
		{
			word retcut;
			
			retcut = SCutSelect(pspecinfo, SPISC, 
									shortcut, pii->shortcut, circle);
			if (retcut != shortcut)
			{
				shortcut = retcut;
				pspecinfo[SPISC].ob_spec = SCut2Obspec(shortcut);
				fulldraw(pspecinfo, SPISC);
			}
		}
	}
	while ((retcode == SPISC) || (retcode == SPISCSC));
	
	setSelected(pspecinfo, retcode, FALSE);
	fulldraw(pspecinfo, retcode);
	DialEnd(&d);
	
	if(retcode == SPIOK)
	{
		if (shortcut != pii->shortcut)
		{
			SCutRemove(pii->shortcut);
			SCutInstall(shortcut);
			pii->shortcut = shortcut;
		}

		if(strcmp(pii->iconname, pname))	/* Name ver„ndert */
		{
			strcpy(pii->iconname, pname);
			SetIconTextWidth(&pii->ib);
			
			redrawObj(&wnull, objnr);
			flushredraw();
		}
	}
}

static void showTotalInfo(WindInfo *wp)
{
	DIALINFO d;
	uword fileanz, foldanz;
	word retcode;
	long bytes;
	char tmppath[MAXLEN];
	
	if((wp->kind == WK_FILE)			/* we're in a real window */
		&& wp->files->finfo[0].flags.selected
		&& (!strcmp(wp->files->finfo[0].fullname, "..")))
	{
		/* Dies sollte eigentlich eine Routine in select.c sein */
		FileInfo *pf = &(wp->files->finfo[0]);
		
		if (pf == getfinfo(&wp->tree[1]))
		{
			objdeselect(wp, 1); /* deselect: no move or copy */
			flushredraw();
		}
		else
		{
			pf->flags.selected = 0;
			wp->selectSize -= pf->size;
			--wp->selectAnz;
			--NewDesk.selectAnz;
			wp->update |= WINDINFO;
		}
	}
	if (wp->kind == WK_FILE)
		strcpy (tmppath, wp->path);
	else
		tmppath[0] = '\0';
	
	GrafMouse (HOURGLASS, NULL);
	MarkDraggedObjects(wp);
	retcode = countMarkedFiles (wp, tmppath, &foldanz, 
						&fileanz, &bytes);
	UnMarkDraggedObjects(wp);

	if (!retcode || (bytes < 0L))
	{
		if (bytes < 0L)
			venusErr (NlsStr(FILE_COUNT_ERR));
		GrafMouse (ARROW, NULL);
		return;
	}
	
	sprintf (ptotalinf[TIFOLDNR].ob_spec.tedinfo->te_ptext,
			"%5u", foldanz);
	sprintf (ptotalinf[TIFILENR].ob_spec.tedinfo->te_ptext,
			"%5u", fileanz);
	sprintf (ptotalinf[TIBYTES].ob_spec.tedinfo->te_ptext,
			"%9ld", bytes);

	GrafMouse (ARROW,NULL);
	
	DialCenter (ptotalinf);
	DialStart (ptotalinf, &d);
	DialDraw (&d);
	
	retcode = DialDo (&d, 0) & 0x7FF;
	setSelected (ptotalinf, retcode, FALSE);

	DialEnd(&d);	
	GrafMouse (ARROW, NULL);
}

/*
 * void getInfoDialog(void)
 *
 * main routine to handle 'get Info...'
 */
void getInfoDialog(void)
{
	WindInfo *wp;
	IconInfo *pii;
	FileInfo *pf;
	word objnr;
	
	if (!thereAreSelected (&wp))
		return;
	
	if (!getOnlySelected (&wp, &objnr))
	{
		/* Es ist mehr als 1 Objekt selektiert.
		 * Es wird die Gesamtzahl von Dateien, etc. angezeigt
		 */
		showTotalInfo(wp);
	}
	else if(wp->kind == WK_FILE)
	{
		pf = GetSelectedFileInfo(wp);
		if (!pf)
			return;
		
		if(pf->attrib & FA_FOLDER)
		{
			if(strcmp(pf->fullname, ".."))
				infoFolderDialog(wp, pf);
			else
				venusInfo(NlsStr(T_FOLDABOVE));
		}
		else
		{
			infoFileDialog(wp, pf);
		}
	}
	else if (wp->kind == WK_DESK)	/* window 0 */
	{
		pii = getIconInfo(&wp->tree[objnr]);
		
		switch(pii->type)
		{
			case DI_FILESYSTEM:
				infoDriveDialog(pii, objnr);
				break;
			case DI_TRASHCAN:
			case DI_SCRAPDIR:
				doSpecialInfo(pii, objnr);
				break;
			case DI_SHREDDER:
				doShredderDialog(pii, objnr);
				break;
			case DI_PROGRAM:
			case DI_FOLDER:
				doIconInfo(pii, objnr);
				break;
			default:
				venusDebug("unknown Icontype!");
		}
	}
}