/*
 * @(#) Gemini\erase.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: functions to kill things
 */

#include <setjmp.h>
#include <string.h>
#include <tos.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "erasebox.rh"

#include "erase.h"
#include "util.h"
#include "fileutil.h"
#include "select.h"
#include "myalloc.h"
#include "venuserr.h"
#include "redraw.h"
#include "iconinst.h"
#include "undo.h"
#include "stand.h"

store_sccs_id(erase);

/* externals
 */
extern DeskInfo NewDesk;
extern WindInfo wnull;
extern OBJECT *perasebox;
extern char *rmCommand;

/* internal texts
 */
#define NlsLocalSection "G.erase"
enum NlsLocalText{
T_ILLFOLDER,	/*Sie wollen gerade den Ordner l”schen, der von 
dem %s ben”tigt wird. Dieser Ordner wird daher 
beim L”schen bersprungen.*/
T_DELFOLDER,	/*Konnte den Ordner %s nicht l”schen! 
Wahrscheinlich enth„lt er noch Dateien.*/
T_USERABORT,	/*Wollen Sie den gesamten L”schvorgang abbrechen?*/
T_MEMABORT,		/*Die Operation muž leider aus Speichermangel 
abgebrochen werden!*/
T_ERRABORT,		/*Die Datei %s konnte nicht gel”scht werden. 
Wollen sie die gesamte Operation abbrechen?*/
T_NOWORK,		/*Es sind keine l”schbaren Objekte zu finden!*/
};

/* internals
 */
static DIALINFO d;
static uword foldcount,filecount;
static long bytestodo;
static jmp_buf jumpenv;
static word verboseDelete = TRUE;

/*
 * static word noRealErase(WindInfo *wp)
 * undrag dragged Disk/File/Folder-Icons and remove them from
 * the Desktop; return if a real Erase is nesseccary
 */
static word noRealErase(WindInfo *wp)
{
	IconInfo *pii;
	word i,realerase,r[4];
	
	if(wp->kind != WK_DESK)		/* not window 0 */
		return FALSE;
		
	realerase = FALSE;
	for(i=wp->tree[0].ob_head;i > 0; i = wp->tree[i].ob_next)
	{
		if((wp->tree[i].ob_type  != (G_USERDEF|DRAG))
			&& (wp->tree[i].ob_type  != (G_ICON|DRAG)))
			continue;
		if ((pii = getIconInfo(&wp->tree[i])) == NULL)
			continue;
			
		switch(pii->type)
		{
			case DI_SHREDDER:
				break;
			case DI_FILESYSTEM:
			case DI_PROGRAM:
			case DI_FOLDER:
				wp->tree[i].ob_type &= ~DRAG;
				objdeselect(wp,i);
				objc_offset(wp->tree,i,&r[0],&r[1]);
				r[2] = wp->tree[i].ob_width;
				r[3] = wp->tree[i].ob_height;

				removeDeskIcon(i);

				buffredraw(wp,r);
				break;
			default:
				realerase = TRUE;
				break;
		}
	}
	flushredraw();	
	return !realerase;
}

/*
 * check if the erased file is on the desktop and
 * remove the desktop icon if that's the case
 */
static void fileWasErased(char *path, int file)
{
	IconInfo *pii;
	word i,prevobj,r[4];
	
	if (!file)
		UndoFolderVanished(path);
		
	for(i=wnull.tree[0].ob_head,prevobj=0;
		i > 0; prevobj=i, i = wnull.tree[i].ob_next)
	{
		if (wnull.tree[i].ob_type  != G_ICON)
			continue;
		if ((pii = getIconInfo(&wnull.tree[i])) == NULL)
			continue;
			
		switch(pii->type)
		{
			case DI_SHREDDER:
			case DI_FILESYSTEM:
				break;
			case DI_SCRAPDIR:
			case DI_TRASHCAN:
			case DI_PROGRAM:
			case DI_FOLDER:
				if (strcmp(path, pii->path)
					|| (file && (pii->type != DI_PROGRAM)))
					continue;
					
				wnull.tree[i].ob_type &= ~DRAG;
				if (isSelected(wnull.tree,i))
				{
					--NewDesk.selectAnz;
					--wnull.selectAnz;
				};
				objc_offset(wnull.tree,i,&r[0],&r[1]);
				r[2] = wnull.tree[i].ob_width;
				r[3] = wnull.tree[i].ob_height;

				removeDeskIcon(i);

				buffredraw(&wnull,r);
				i = prevobj;
				break;
			default:
				break;
		}
	}
	
	if (!file)
	{
		WindInfo *wp;
		
		wp = wnull.nextwind;
		while(wp)
		{
			if ((wp->kind == WK_FILE) 
				&& (strlen(path) > 3)
				&& (strstr(wp->path, path) != NULL))
			{
				strcpy(wp->path, path);
				stripFolderName(wp->path);
				strcpy(wp->title, wp->path);
				wp->update |= NEWWINDOW;
			}
			wp = wp->nextwind;
		}
	}
}

/*
 * update the numbers in the dialog box if todraw is true
 */
static void updateCount(word todraw)
{
	if (NewDesk.silentRemove)
		return;
	
	sprintf(perasebox[ERFOLDNR].ob_spec.tedinfo->te_ptext,
			"%5u",foldcount);
	sprintf(perasebox[ERFILENR].ob_spec.tedinfo->te_ptext,
			"%5u",filecount);
	sprintf(perasebox[ERBYTES].ob_spec.tedinfo->te_ptext,
			"%9ld",bytestodo);
	if (todraw)
	{
		fulldraw(perasebox,ERFOLDNR);
		fulldraw(perasebox,ERFILENR);
		fulldraw(perasebox,ERBYTES);
	}
}

/*
 * Display the erase dialog and return wether the user wants
 * to erase the stuff or not.
 */
static word askForErase(void)
{
	word retcode;
	
	if (NewDesk.silentRemove)
		return TRUE;

	GrafMouse(ARROW,NULL);
	strcpy(perasebox[ERFILE].ob_spec.tedinfo->te_ptext,"@");
	strcpy(perasebox[ERFOLDER].ob_spec.tedinfo->te_ptext,"@");
	setDisabled(perasebox,ERABORT,TRUE);
	updateCount(FALSE);
	
	DialCenter(perasebox);
	DialStart(perasebox,&d);
	DialDraw(&d);
	
	retcode = DialDo(&d,0) & 0x7FFF;
	
	setSelected(perasebox,retcode, FALSE);
	if (retcode == EROK)
	{
		setDisabled(perasebox,ERABORT,FALSE);
		fulldraw(perasebox,ERABORT);
		GrafMouse(HOURGLASS,NULL);
		return TRUE;
	}
	return FALSE;
}

/*
 * Clean up things if dialog was used.
 */
static void EndDialog(void)
{
	if (!NewDesk.silentRemove)
		DialEnd(&d);
}

/*
 * Display the file name in the dialog box
 */
static void displayFileName(const char *name)
{
	char dname[MAX_FILENAME_LEN];
	
	if (NewDesk.silentRemove)
		return;
	strcpy(dname,name);
	makeEditName(dname);
	strcpy(perasebox[ERFILE].ob_spec.tedinfo->te_ptext,dname);
	fulldraw(perasebox,ERFILE);
}

/*
 * Display the folder name in the dialog box and clear
 * the file name. 
 */
static void displayFolderName(const char *name)
{
	char dname[MAX_FILENAME_LEN];
	
	if (NewDesk.silentRemove)
		return;
	strcpy(dname,name);
	makeEditName(dname);
	strcpy(perasebox[ERFILE].ob_spec.tedinfo->te_ptext,"@");
	strcpy(perasebox[ERFOLDER].ob_spec.tedinfo->te_ptext,dname);
	fulldraw(perasebox,ERFILE);
	fulldraw(perasebox,ERFOLDER);
}

static word isValidFolder(char *fpath)
{
	IconInfo *pii;
	char *iname;
	word retcode = TRUE;
	
	pii = getIconInfo(&wnull.tree[NewDesk.scrapNr]);
	if (pii)
	{
		/*
		 * check if someone wants to erase the scrapdir
		 */
		if (!strcmp(fpath,pii->path))
		{
			iname = pii->iconname;
			retcode = FALSE;
		}
	}

	pii = getIconInfo(&wnull.tree[NewDesk.trashNr]);
	if (pii)
	{
		/*
		 * check if someone wants to erase the trashdir
		 */
		if (!strcmp(fpath,pii->path))
		{
			iname = pii->iconname;
			retcode = FALSE;
		}
	}
	
	if (!retcode)
	{
		venusErr(NlsStr(T_ILLFOLDER),iname);
		GrafMouse(HOURGLASS,NULL);
		verboseDelete = FALSE;
		return FALSE;
	}
	return TRUE;
}

static void cleanUpFolder(char *name, char *fpath)
{
	word wasErased = FALSE;
	
	stripFolderName(fpath);
	addFileName(fpath,name);
	if (Ddelete(fpath) < 0)
	{
		if (verboseDelete)
		{
			venusErr(NlsStr(T_DELFOLDER),fpath);
			GrafMouse(HOURGLASS,NULL);
			verboseDelete = FALSE;
		}
	}
	else
		wasErased = TRUE;
	stripFileName(fpath);
	addFolderName(fpath,name);
	if (wasErased)
		fileWasErased(fpath, FALSE);
}

static void eraseFile(char *name, char *fpath, long size)
{
	word retcode;
	
	if (escapeKeyPressed())
	{
		if (venusChoice(NlsStr(T_USERABORT)))
			longjmp(jumpenv,2);
		else
			GrafMouse(HOURGLASS,NULL);
	}
	
	displayFileName(name);
	addFileName(fpath,name);
	
#if STANDALONE
	{
		size_t len;
		char *sysCommand;
		len = strlen(rmCommand) + strlen(fpath) + 4;
	
		if ((sysCommand = tmpmalloc(len)) == NULL)
		{
			venusErr(NlsStr(T_MEMABORT));
			longjmp(jumpenv,2);
		}
		strcpy(sysCommand,rmCommand);
		strcat(sysCommand,fpath);
		strcat(sysCommand,"\'");
		retcode = system(sysCommand);
		tmpfree(sysCommand);
	}
#else
	{
		word memfailed;

		retcode = CallMupfelFunction(RM, fpath, NULL, &memfailed);
	
		if (memfailed)
		{
			venusErr(NlsStr(T_MEMABORT));
			longjmp(jumpenv,2);
		}
	}
#endif
	
	if (retcode)
	{
		sysError(retcode);
		
		if ((filecount>1 || foldcount)
			&& venusChoice(NlsStr(T_ERRABORT), fpath))
		{
			longjmp(jumpenv,1);
		}
		else
			GrafMouse(HOURGLASS,NULL);
	}
	else
		fileWasErased(fpath, TRUE);

	stripFileName(fpath);
	--filecount;
	bytestodo -= size;
	updateCount(TRUE);
}

static void eraseInsideFolder (char *name, char *fpath)
{
	DTA *saveDta, myDta;
	char filename[MAX_FILENAME_LEN];
	static void eraseFolder (char *name, char *fpath);
	
	if (escapeKeyPressed ())
	{
		if (venusChoice (NlsStr (T_USERABORT)))
			longjmp (jumpenv, 2);
		else
			GrafMouse (HOURGLASS, NULL);
	}
	
	saveDta = Fgetdta ();
	Fsetdta (&myDta);

	addFileName (fpath, "*.*");	
	if (!Fsfirst (fpath, FA_NOLABEL))
	{
		stripFileName (fpath);
		do
		{
			strncpy (filename, myDta.d_fname, MAX_FILENAME_LEN - 1);
			filename[MAX_FILENAME_LEN - 1] = '\0';
		
			if (myDta.d_attrib & FA_FOLDER)
			{
				if (!strcmp (filename, ".")
					|| !strcmp (filename, ".."))
					continue;
					
				eraseFolder (filename, fpath);
				displayFolderName (name);
			}
			else if (!(myDta.d_attrib & FA_LABEL))
			{
				eraseFile (filename, fpath, myDta.d_length);
			}
			
		}
		while (!Fsnext ());
	}
	else
		stripFileName (fpath);
		
	Fsetdta (saveDta);
}

static void eraseFolder(char *name, char *fpath)
{
	displayFolderName(name);
	addFolderName(fpath,name);
	
	if (isValidFolder(fpath))
	{
		eraseInsideFolder(name,fpath);
		cleanUpFolder(name,fpath);
	}
	else
	{
		uword fileanz = 0,foldanz = 0;
		long size = 0L;
		
		if (addFileAnz(fpath,&foldanz,&fileanz,&size))
		{
			bytestodo -= size;
			foldcount -= foldanz;
			filecount -= fileanz;
		}
	}
	--foldcount;
	updateCount(TRUE);
	stripFolderName(fpath);
}

/* 
 * l”sche alle Objekte in, die mit DRAG markiert sind,
 * aus dem File-Window fwp
 */
static void eraseFileWindow(WindInfo *fwp, char *fpath)
{
	uword i;
	FileBucket *bucket;
	
	/* walk children */
	
	bucket = fwp->files;
	
	while (bucket)
	{
		for (i = 0; i < bucket->usedcount; ++i)
		{
			if (bucket->finfo[i].flags.dragged
				& bucket->finfo[i].flags.selected)
			{
				if (bucket->finfo[i].attrib & FA_FOLDER)
				{
					verboseDelete = TRUE;
					eraseFolder(bucket->finfo[i].fullname, fpath);
					displayFolderName("@");
				}
				else
					eraseFile(bucket->finfo[i].fullname, fpath,
							bucket->finfo[i].size);
			}
		}
	
		bucket = bucket->nextbucket;
	}
}

/*
 * l”sche alle Objekte, die mit DRAG markiert sind
 * vom Desk-Window 
 */
static void eraseDeskWindow(WindInfo *fwp, char *fpath)
{
	word objnr;
	IconInfo *pii;
	char name[MAX_FILENAME_LEN];
	
	/* walk children */
	
	for (objnr = fwp->tree[0].ob_head; 
		objnr > 0;
		objnr = fwp->tree[objnr].ob_next)
	{
		if (fwp->tree[objnr].ob_type != (G_ICON|DRAG))
			continue;

		pii = getIconInfo(&fwp->tree[objnr]);
		if (!pii)
			continue;
		
		strcpy(fpath,pii->path);
		if (!getBaseName(name,fpath))
			continue;
			
		switch(pii->type)
		{
			case DI_TRASHCAN:
			case DI_SCRAPDIR:
				displayFolderName(name);
				verboseDelete = TRUE;
				eraseInsideFolder(name,fpath);
				displayFolderName("@");
				buffPathUpdate(fpath);
				break;
			case DI_FILESYSTEM:
			case DI_PROGRAM:
			case DI_FOLDER:
			case DI_SHREDDER:
				break;
		}
	}
}

/*
 * erase with DRAG marked files in window fwp and use fpath
 * for these files. return if something has happend.
 */
word doErase(WindInfo *fwp, char *fpath)
{
	DTA *saveDta;
	
	if (noRealErase(fwp))
		return FALSE;
	
	GrafMouse(HOURGLASS,NULL);
	if (!countMarkedFiles(fwp,fpath,&foldcount,&filecount,&bytestodo))
	{
		GrafMouse(ARROW,NULL);
		return FALSE;
	}
	if (foldcount==0 && filecount==0)
	{
		venusInfo(NlsStr(T_NOWORK));
		GrafMouse(ARROW,NULL);
		return FALSE;
	}
	if (!askForErase())
	{
		EndDialog();
		return FALSE;
	}
	
	saveDta = Fgetdta();
	if (setjmp(jumpenv) == 0)
	{
		switch(fwp->kind)
		{
			case WK_FILE:
				eraseFileWindow(fwp,fpath);
				break;
			case WK_DESK:
				eraseDeskWindow(fwp,fpath);
				break;
			default:
				break;
		}
	}
	
	Fsetdta(saveDta);
	EndDialog();
	flushredraw();
	flushPathUpdate();
	
	GrafMouse(ARROW,NULL);
	return TRUE;
}

/*
 * void emptyPaperbasket(void)
 * empty the TRASHDIR
 */
void emptyPaperbasket(void)
{
	IconInfo *pii;
	word savestate;
	char path[MAXLEN], name[MAX_FILENAME_LEN];
	
	pii = getIconInfo(&NewDesk.tree[NewDesk.trashNr]);
	if(pii)
	{
		strcpy(path,pii->path);
		if (!getBaseName(name,path))
			return;
		
		savestate = NewDesk.silentRemove;
		NewDesk.silentRemove = TRUE;
		if(setjmp(jumpenv) == 0)
			eraseInsideFolder(name,path);
		
		NewDesk.silentRemove = savestate;
		pathUpdate(pii->path,"");
	}
}