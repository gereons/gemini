/*
 * @(#) Gemini\Copymove.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: functions to copy/move files
 *
 */

#include <setjmp.h>
#include <string.h>
#include <tos.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "copybox.rh"
#include "namebox.rh"
#include "copymove.h"
#include "util.h"
#include "fileutil.h"
#include "myalloc.h"
#include "venuserr.h"
#include "undo.h"
#include "stand.h"

store_sccs_id(cpmove);

/* externals
 */ 
extern OBJECT *pcopybox,*pmovebox,*pnamebox;
extern WindInfo wnull;
extern DeskInfo NewDesk;
extern char *cpCommand;
extern char *mvCommand;

/* internal texts
 */
#define NlsLocalSection "G.copymove"
enum NlsLocalText{
T_DELFOLDER,	/*Konnte den Ordner %s nicht lîschen! 
Wahrscheinlich enthÑlt er noch Dateien.*/
T_USERABORT,	/*Wollen Sie die Operation wirklich abbrechen?*/
T_MEMABORT,		/*Die Operation muû aus Speichermangel 
abgebrochen werden.*/
T_ERRABORT,		/*Kann die Datei %s nicht %s! Wollen Sie 
die gesamte Operation abbrechen?*/
T_COPY,			/*kopieren*/
T_MOVE,			/*verschieben*/
T_RECURSION,	/*Das %s von %s nach %s wÅrde zu einer Endlos
schleife fÅhren! Die Operation wird daher abgebrochen.*/
T_COPIED,		/*Kopieren*/
T_MOVED,		/*Verschieben*/
T_SHREDDER,		/*Der Reiûwolf ist immer leer. Wenn sie gelîschte
 Dateien wiederfinden wollen, mÅssen Sie im Papierkorb suchen.*/
T_NOFOLDER,		/*Kann Dateien/Ordner nicht %s!
 Das Verzeichnis %s existiert nicht.*/
T_NOWORK,		/*Irgendwie gibt es hier nichts zu %s.*/
T_FOLDERR,		/*Kann den Ordner %s nicht anlegen!*/
T_NO_FOLDER_SPACE,	/*Kann den Ordner %s nicht anlegen, da das 
Laufwerk %c: keinen freien Speicherplatz mehr hat!*/
T_NOTFOUND,		/*Die angewÑhlten Objekte konnten nicht gefunden 
werden! %s*/
T_NOTFOUNDFILE,	/*PrÅfen Sie bitte, ob Sie das richtige 
Medium eingelegt haben.*/
T_NOTFOUNDDESK,	/*Schauen Sie bitte nach, ob die Objekte 
noch auf existierende Dateien/Ordner verweisen.*/
T_NONAME,	/*Sie sollten einen Namen angeben!*/
T_INVALID,	/*Dies ist kein gÅltiger Name fÅr eine Datei 
oder einen Ordner! Bitte wÑhlen Sie einen anderen Namen.*/
T_NEED_OTHER_NAME,	/* Es existiert schon eine Datei gleichen Namens.
 Sie mÅssen also einen anderen Namen vergeben!*/
};


/* internals
 */
static jmp_buf jumpenv;
static DIALINFO d;
static uword foldcount,filecount;
static word writeOver,tomove;
static long bytestodo;
static word wasError;
#if STANDALONE
static char *Command;
#endif

/*
 * static word getOtherName(char *name)
 * get other filename (with dialog and other stuff),
 * return new name in 'name' and state of 'Cancel'-Button 
 */
static word getOtherName(char *name)
{
	DIALINFO d;
	word ok;
	char newname[MAX_FILENAME_LEN], oldname[MAX_FILENAME_LEN];
	int edit_object = NNEWNAME;
	int done, do_overwrite, do_abort, retcode;

	pnamebox[NOLDNAME].ob_spec.tedinfo->te_ptext = oldname;
	strcpy (oldname, name);
	makeEditName (oldname);
	
	pnamebox[NNEWNAME].ob_spec.tedinfo->te_ptext = newname;
	strcpy (newname, name);
	makeEditName (newname);
	
	DialCenter (pnamebox);
	DialStart (pnamebox, &d);
	DialDraw (&d);
	
	done = FALSE;
	retcode = FALSE;
	
	while (!done)
	{
		do_overwrite = writeOver;
		do_abort = FALSE;
		
		ok = DialDo (&d, &edit_object) & 0x7FFF;
		
		switch (ok)
		{
			case NOVER:					/* overwrite all */
				do_overwrite = TRUE;
			case NOK:					/* take new name */
				strcpy (name, newname);
				makeFullName (name);
	
				if((*name == '@') || !strlen (name))
				{
					venusErr (NlsStr (T_NONAME));
					break;
				}
				
				if (!ValidFileName (name))
				{
					venusErr (NlsStr (T_INVALID));
					break;
				}
	
				retcode = TRUE;
				done = TRUE;
				break;
				
			case NABORT:
				do_abort = TRUE;
			default:					/* skip this file */
				retcode = FALSE;
				done = TRUE;
				break;
		}

		setSelected (pnamebox, ok, FALSE);
		fulldraw (pnamebox, ok);
	
	}

	DialEnd (&d);
	
	if (do_abort)
		longjmp(jumpenv,2);		/* User aborts copy/move */
	
	writeOver = do_overwrite;
		
	return retcode;
}

/*
 * update the numbers in the dialog box if todraw is true
 */
static void updateCount(word todraw)
{
	if (NewDesk.silentCopy)
		return;
	
	sprintf(pcopybox[CPFOLDNR].ob_spec.tedinfo->te_ptext,
			"%5u",foldcount);
	sprintf(pcopybox[CPFILENR].ob_spec.tedinfo->te_ptext,
			"%5u",filecount);
	sprintf(pcopybox[CPBYTES].ob_spec.tedinfo->te_ptext,
			"%9ld",bytestodo);
	if (todraw)
	{
		fulldraw(pcopybox,CPFOLDNR);
		fulldraw(pcopybox,CPFILENR);
		fulldraw(pcopybox,CPBYTES);
	}
}

/*
 * Display the copy dialog and return wether the user wants
 * to copy the stuff or not.
 */
static word askForCopy(void)
{
	word retcode;
	
	if (NewDesk.silentCopy)
		return TRUE;
	
	GrafMouse(ARROW,NULL);
	strcpy(pcopybox[CPFILE].ob_spec.tedinfo->te_ptext,"@");
	strcpy(pcopybox[CPFOLDER].ob_spec.tedinfo->te_ptext,"@");
	setDisabled(pcopybox,CPABORT,TRUE);
	setHideTree(pcopybox,CPTITLE, tomove);
	setHideTree(pcopybox,MVTITLE,!tomove);
	updateCount(FALSE);
	
	DialCenter(pcopybox);
	DialStart(pcopybox,&d);
	DialDraw(&d);
	
	retcode = DialDo(&d,0) & 0x7FFF;
	
	setSelected(pcopybox,retcode, FALSE);
	if (retcode == CPOK)
	{
		setDisabled(pcopybox,CPABORT,FALSE);
		fulldraw(pcopybox,CPABORT);
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
	if (!NewDesk.silentCopy)
		DialEnd(&d);
}

/*
 * Display the file name in the dialog box
 */
static void displayFileName(const char *name)
{
	char dname[MAX_FILENAME_LEN];
	
	if (NewDesk.silentCopy)
		return;
	strcpy(dname,name);
	makeEditName(dname);
	strcpy(pcopybox[CPFILE].ob_spec.tedinfo->te_ptext,dname);
	fulldraw(pcopybox,CPFILE);
}

/*
 * Display the folder name in the dialog box and clear
 * the file name. 
 */
static void displayFolderName(const char *name)
{
	char dname[MAX_FILENAME_LEN];
	
	if (NewDesk.silentCopy)
		return;
	strcpy(dname,name);
	makeEditName(dname);
	strcpy(pcopybox[CPFILE].ob_spec.tedinfo->te_ptext,"@");
	strcpy(pcopybox[CPFOLDER].ob_spec.tedinfo->te_ptext,dname);
	fulldraw(pcopybox,CPFILE);
	fulldraw(pcopybox,CPFOLDER);
}

/*
 * Tough to understand: we need to prevent an overwrite
 * of existing files if writeover is false. But we should
 * never copy a file onto itself.
 */
static word checkValidTargetFile(char *fpath, char *tpath)
{
	word ok = TRUE;
	char name[MAX_FILENAME_LEN],oldname[MAX_FILENAME_LEN];
	
	/* öberschreiben und verschiedene Dateien und Ziel kein
	 * Verzeichnis?
	 */
	if (writeOver && strcmp(fpath,tpath) && !isDirectory(tpath))
		return TRUE;
		
	/* Der Zielpfad ist nicht gÅltig!
	 */
	if (!getBaseName(name,tpath))
		return FALSE;
	
	/* Solange wie die Zieldatei schon existiert und der User
	 * auf OK gedrÅckt hat...
	 */
	while(fileExists(tpath) && ok)
	{
		GrafMouse(ARROW,NULL);
		
		/* merke dir den alten Namen
		 */
		strcpy(oldname,name);
		
		/* Dialog zum öberspringen, OK, Abbruch, Erstzen.
		 * ergibt nur TRUE bei Ersetzen und OK.
		 */
		ok = getOtherName(name);
		GrafMouse(HOURGLASS,NULL);
		
		/* Bastele den neuen Zielpfad
		 */
		stripFileName(tpath);
		addFileName(tpath,name);
		
		/* nicht öberschreiben und neuer Namen -> neu ÅberprÅfen
		 */
		if (!writeOver && strcmp(oldname,name))
			continue;
		
		/* Hier sind wir entweder beim öberschreiben oder der Name
		 * ist gleich geblieben. Dann brauchen wir nur noch zu
		 * testen, ob Quelle und Ziel verschieden sind
		 */
		if (ok && strcmp(fpath,tpath) && !isDirectory(tpath))
			break;
	}
	return ok;
}

/*
 * Make the folder tpath; if its already there ask the
 * user wether he wants to use it or what the heck he
 * wants at all.
 */
static word makeValidTargetFolder (char *tpath)
{
	word ok = FALSE;
	char name[MAX_FILENAME_LEN], oldname[MAX_FILENAME_LEN];
	
	if (!getBaseName (name, tpath))
		return FALSE;

	stripFolderName (tpath);
	addFileName (tpath, name);
	
	while (TRUE)
	{
		int existing, is_folder;
		
		existing = fileExists (tpath);
		is_folder = existing && isDirectory (tpath);
		
		/* Wenn der Ordner existiert und wir Åberschreiben, ist
		 * alles in Ordnung.
		 */
		if (is_folder && writeOver)
		{
			stripFileName (tpath);
			addFolderName (tpath, name);
			return TRUE;
		}
		
		/* Wenn nichts dergleichen existiert und das Anlegen des
		 * Ordners schiefgeht, zeigen wir eine Fehlermeldung und
		 * geben auf.
		 */
		if (!existing)
		{
			DISKINFO di;
			const char *err;
			
			if (Dcreate (tpath) == 0)
			{
				stripFileName (tpath);
				addFolderName (tpath, name);
				return TRUE;
			}
				
			if (!Dfree (&di, tpath[0] - 'A' + 1) && di.b_free == 0L)
				err = NlsStr (T_NO_FOLDER_SPACE);
			else
				err = NlsStr (T_FOLDERR);
				
			venusErr (err, tpath, tpath[0]);
			longjmp (jumpenv, 2);
		}
		
		/* Wenn wir hier ankommen, so existiert ein Objekt gleichen
		 * Namens und wir geben dem Benutzer die Gelegenheit, einen
		 * neuen Namen zu wÑhlen.
		 */
		while (TRUE)
		{
			strcpy (oldname, name);

			GrafMouse (ARROW, NULL);
			ok = getOtherName (name);
			GrafMouse (HOURGLASS, NULL);
			
			stripFileName (tpath);
			addFileName (tpath, name);

			/* Wenn eine Datei gleichen Namens existiert, dann *muû*
			 * ein neuer Name vergeben werden.
			 */
			if (ok && !is_folder && !strcmp (oldname, name))
				venusErr (NlsStr (T_NEED_OTHER_NAME));
			else
				break;
		}
		
		/* Wenn öberspringen gewÑhlt wurde, oder in einen 
		 * existierenden Ordner kopiert werden soll, verlasse diese
		 * Schleife.
		 */
		if (!ok || (is_folder && !strcmp (oldname, name)))
			break;
	}
	
	stripFileName (tpath);
	addFolderName (tpath, name);
	return ok;
}

/*
 * While moving files/folders we need to delete a moved folder
 * afterwards.
 */
static void cleanUpFolder(char *name, char *fpath, char *tpath)
{
	if (!tomove)
		return;
	
	fileWasMoved(fpath,tpath);
	UndoFolderVanished(fpath);
	
	stripFolderName(fpath);
	addFileName(fpath,name);
	if (Ddelete(fpath) < 0 && !wasError)
	{
		venusErr(NlsStr(T_DELFOLDER),fpath);
		GrafMouse(HOURGLASS,NULL);
		wasError = TRUE;
	}
	stripFileName(fpath);
	addFolderName(fpath,name);
}

/*
 * kopiere das file name aus dem Ordner fpath in den
 * Ordner tpath
 */
static void cpFile(char *name, char *fpath, char *tpath, long size)
{
	if (escapeKeyPressed())
	{
		if (venusChoice(NlsStr(T_USERABORT)))
			longjmp(jumpenv,2);
		else
			GrafMouse(HOURGLASS,NULL);
	}
	
	displayFileName(name);
	addFileName(tpath,name);
	addFileName(fpath,name);
	
	if (checkValidTargetFile(fpath,tpath))
	{
		word retcode;
#if STANDALONE
		size_t len;
		char *sysCommand;
		
		len = strlen(Command) + strlen(fpath) + strlen(tpath) + 6;

		if ((sysCommand = tmpmalloc(len)) == NULL)
		{
			venusErr(NlsStr(T_MEMABORT));
			longjmp(jumpenv,2);
		}
		strcpy(sysCommand,Command);
		strcat(sysCommand,fpath);
		strcat(sysCommand,"\' \'");
		strcat(sysCommand,tpath);
		strcat(sysCommand,"\'");
		
		retcode = system(sysCommand);
		tmpfree(sysCommand);

#else
		word memfailed;
		
		retcode = CallMupfelFunction(tomove? MV : CP, 
								fpath, tpath, &memfailed);

		if (memfailed)
		{
			venusErr(NlsStr(T_MEMABORT));
			longjmp(jumpenv,2);
		}
#endif
		if (retcode)
		{
			sysError(retcode);
			
			if ((filecount>1 || foldcount) 
				&& venusChoice(NlsStr(T_ERRABORT), fpath,
				 tomove? NlsStr(T_MOVE) : NlsStr(T_COPY)))
			{
				longjmp(jumpenv,1);
			}
			else
			{
				GrafMouse(HOURGLASS,NULL);
				wasError = TRUE;
			}
		}

		if (!wasError && tomove)
			fileWasMoved(fpath,tpath);
	}
	stripFileName(fpath);
	stripFileName(tpath);
	--filecount;
	bytestodo -= size;
	updateCount(TRUE);
}

/*
 * kopiere alle Objekte vom Ordner fpath in den Ordner tpath
 */
static void cpInsideFolder (char *name, char *fpath, char *tpath)
{
	DTA *saveDta, myDta;
	char filename[MAX_FILENAME_LEN];
	static void cpFolder(char *name, char *fpath, char *tpath);
	
	if (escapeKeyPressed())
	{
		if (venusChoice(NlsStr(T_USERABORT)))
			longjmp(jumpenv,2);
		else
			GrafMouse(HOURGLASS,NULL);
	}
	
	saveDta = Fgetdta();
	Fsetdta(&myDta);

	addFileName(fpath,"*.*");	
	if (!Fsfirst(fpath, FA_NOLABEL))
	{
		stripFileName(fpath);
		do
		{
			strncpy (filename, myDta.d_fname, MAX_FILENAME_LEN - 1);
			filename[MAX_FILENAME_LEN - 1] = '\0';
		
			if (myDta.d_attrib & FA_FOLDER)
			{
				if (!strcmp (filename, ".")
					|| !strcmp (filename, ".."))
					continue;
					
				cpFolder (filename, fpath, tpath);
				displayFolderName (name);
			}
			else if (!(myDta.d_attrib & FA_LABEL))
			{
				cpFile (filename, fpath, tpath, myDta.d_length);
			}
			
		}
		while (!Fsnext ());
	}
	else
		stripFileName (fpath);
		
	Fsetdta (saveDta);
}

/*
 * kopiere den Ordner name im Ordner fpath in den Ordner tpath.
 * name wird dabei im Ordner tpath angelegt.
 */
static void cpFolder (char *name, char *fpath, char *tpath)
{
	displayFolderName (name);
	addFolderName (fpath, name);
	
	if (strstr (tpath, fpath) == tpath)	/* Fano condition */
	{
		venusErr (NlsStr (T_RECURSION), 
				tomove? NlsStr (T_MOVED) : NlsStr (T_COPIED), 
				fpath, tpath);
		longjmp (jumpenv, 2);
	}
	
	addFolderName (tpath, name);
	
	if (makeValidTargetFolder (tpath))
	{
		cpInsideFolder (name, fpath, tpath);
		cleanUpFolder (name, fpath, tpath);
	}
	else
	{
		uword fileanz = 0,foldanz = 0;
		long size = 0L;
		
		if (addFileAnz (fpath, &foldanz, &fileanz, &size))
		{
			bytestodo -= size;
			foldcount -= foldanz;
			filecount -= fileanz;
		}
	}
	--foldcount;
	updateCount (TRUE);
	stripFolderName (fpath);
	stripFolderName (tpath);
}

/* 
 * kopiere alle Objekte in, die mit DRAG markiert sind,
 * aus dem File-Window fwp nach tpath
 */
static void cpFileWindow(WindInfo *fwp, char *fpath, char *tpath)
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
					cpFolder(bucket->finfo[i].fullname, fpath, tpath);
					displayFolderName("");
				}
				else
				{
					cpFile(bucket->finfo[i].fullname, fpath, tpath,
							bucket->finfo[i].size);
				}
			}
		}
	
		bucket = bucket->nextbucket;
	}
}

/*
 * kopiere alle Objekte, die mit DRAG markiert sind
 * vom Desk-Window nach tpath
 */
static void cpDeskWindow(WindInfo *fwp, char *fpath, char *tpath)
{
	word objnr;
	IconInfo *pii;
	char name[MAX_FILENAME_LEN];
	long size;
	
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
			strcpy(name,"");
			
		switch(pii->type)
		{
			case DI_FILESYSTEM:
			case DI_TRASHCAN:
			case DI_SCRAPDIR:
				displayFolderName(name);
				cpInsideFolder(name,fpath,tpath);
				displayFolderName("@");
				if (tomove)
					buffPathUpdate(fpath);
				break;
			case DI_PROGRAM:
				size = getFileSize(fpath);
				stripFileName(fpath);
				displayFolderName("@");
				cpFile(name,fpath,tpath,size);
				if (tomove)
					buffPathUpdate(fpath);
				break;
			case DI_FOLDER:
				stripFolderName(fpath);
				cpFolder(name,fpath,tpath);
				displayFolderName("@");
				if (tomove)
					buffPathUpdate(fpath);
				break;
			case DI_SHREDDER:
				venusInfo(NlsStr(T_SHREDDER));
				GrafMouse(HOURGLASS,NULL);
				break;
		}
	}
}

/*
 * main entry for copying files/folders.
 */
word doCopy(WindInfo *fwp, char *fpath, char *tpath, word move)
{
	DTA *saveDta;
	char *savefpath,*savetpath;

	tomove = move;
#if STANDALONE
	Command = tomove? mvCommand : cpCommand;
#endif
	writeOver = NewDesk.replaceExisting;
	wasError = FALSE;
	
	if (!isDirectory(tpath))
	{
		venusErr(NlsStr(T_NOFOLDER),
			tomove? NlsStr(T_MOVE) : NlsStr(T_COPY), tpath);
		return FALSE;
	}
	
	GrafMouse(HOURGLASS,NULL);
	if (!countMarkedFiles(fwp,fpath,&foldcount,&filecount,&bytestodo))
	{
		GrafMouse(ARROW,NULL);
		return FALSE;
	}
	if (filecount==0 && foldcount==0)
	{
		venusErr(NlsStr(T_NOWORK),
			tomove? NlsStr(T_MOVE) : NlsStr(T_COPY));
		GrafMouse(ARROW, NULL);
		return FALSE;
	}
	
	if (bytestodo < 0)
	{
		venusErr(NlsStr(T_NOTFOUND), 
			(fwp->kind == WK_DESK)? NlsStr(T_NOTFOUNDDESK) 
			: NlsStr(T_NOTFOUNDFILE));
		GrafMouse(ARROW, NULL);
		return FALSE;
	}
	
	if (!askForCopy())
	{
		EndDialog();
		return FALSE;
	}
	
	if ((savefpath=tmpmalloc(strlen(fpath)+1)) != NULL)
		strcpy(savefpath,fpath);
	if ((savetpath=tmpmalloc(strlen(tpath)+1)) != NULL)
		strcpy(savetpath,tpath);

	saveDta = Fgetdta();
	if (setjmp(jumpenv) == 0)
	{
		switch(fwp->kind)
		{
			case WK_FILE:
				cpFileWindow(fwp,fpath,tpath);
				break;
			case WK_DESK:
				cpDeskWindow(fwp,fpath,tpath);
				break;
			default:
				break;
		}
	}
	
	writeOver = NewDesk.replaceExisting;
	Fsetdta(saveDta);
	EndDialog();
	flushPathUpdate();
	
	if (savefpath)
	{
		strcpy(fpath,savefpath);
		tmpfree(savefpath);
	}
	if (savetpath)
	{
		strcpy(tpath,savetpath);
		tmpfree(savetpath);
	}
	GrafMouse(ARROW,NULL);
	return TRUE;
}