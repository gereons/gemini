/*
 * @(#) Gemini\Dispatch.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: Dispatch several events 
 *
 */

#include <stddef.h>
#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "ttpbox.rh"
#include "menu.rh"

#include "dispatch.h"
#include "myalloc.h"
#include "util.h"
#include "fileutil.h"
#include "select.h"
#include "venuserr.h"
#include "pexec.h"
#include "redraw.h"
#include "window.h"
#include "applmana.h"
#include "undo.h"
#include "icondrag.h"
#include "menu.h"
#include "getinfo.h"
#include "message.h"
#include "iconinst.h"

/* internal texts
 */
#define NlsLocalSection "G.dispatch"
enum NlsLocalText{
T_TWODISK,	/*Fr diese Operation mssten sie zwei Disketten
 gleichzeitig in ein Laufwerk schieben!*/
T_NESTED,	/*Die Operation wird nicht erlaubt, da die 
Ordner zu tief verschachtelt sind.*/
T_SHREDDER,	/*Der Reižwolf ist immer leer. Das ™ffnen dieses
 Objekts ist daher sinnlos bzw. nicht m”glich!*/
T_NODRIVE,	/*Das Laufwerk %c: ist GEMDOS nicht bekannt!*/
T_NOLABEL,	/*Kann das Label von Laufwerk %c: nicht lesen!
 Ist evtl. keine Diskette/Cartridge eingelegt?*/
T_NOFILE,	/*Kann das Objekt %s nicht starten!*/
T_DELAPPL,	/*Die angemeldete Applikation %s konnte nicht
 gefunden werden! Wollen Sie die entsprechende Regel l”schen?*/
T_NOAPPL,	/*Konnte keine Applikation fr %s finden!*/
T_LOCATION,	/*Kann das Objekt %s nicht finden. Soll das Icon
 entfernt werden, oder wollen Sie danach suchen?*/
T_LOCBUTTON, /*[Entfernen|[Suchen|[Abbruch*/
T_LOCOBJ,	/*Objekt suchen*/
};

store_sccs_id(dispatch);

/* Externs
 */
extern WindInfo wnull;
extern DeskInfo NewDesk;
extern OBJECT *pmenu,*pttpbox;
extern word apid;
extern deskx,desky,deskw,deskh;


/*
 * static char *getCommandLine(const char *name)
 * get a Commandline from user by dialog
 */
static char *getCommandLine(const char *name)
{
	DIALINFO d;
	word retcode;
	size_t len;
	char *pname,*pline;
	char tmp[MAX_FILENAME_LEN],*cp;
	int edit_object = TTPCOMM;
	
	pname = pttpbox[TTPNAME].ob_spec.tedinfo->te_ptext;
	pline = pttpbox[TTPCOMM].ob_spec.tedinfo->te_ptext;
	strcpy(tmp,name);
	makeEditName(tmp);
	strcpy(pname,tmp);
	len = strlen(pline) + 1;
	cp = malloc(len * sizeof(char));
	if(cp == NULL)
		return NULL;

	DialCenter(pttpbox);
	DialStart(pttpbox,&d);
	DialDraw(&d);
	
	retcode = DialDo (&d, &edit_object) & 0x7FFF;
	setSelected(pttpbox,retcode,FALSE);
	
	if(retcode == TTPOK)
	{
		if(*pline == '@')
			*cp = '\0';
		else
			strcpy(cp, pline);
	}
	else
	{
		free(cp);
		cp = NULL;
	}
	DialEnd(&d);

	return cp;
}

/* 
 * Schaut nach, ob das Laufwerk <drive> dasselbe Label wie <label>
 * hat und fordert den Benutzer ggfs. auf das Medium einzulegen.
 * Im Fehlerfall oder bei Abbruch wird FALSE zurckgegeben.
 */
static int checkLabel (char drive, const char *label)
{
	char real_label[MAX_FILENAME_LEN];
	
	if (getLabel (drive - 'A', real_label))
	{
		if (!sameLabel (real_label, label))
		{
			return changeDisk (drive, label);
		}
		return TRUE;
	}
	else
		venusErr (NlsStr(T_NOLABEL), drive);
	
	return FALSE;	
}

/*
 * word startFile(WindInfo *wp,word fobj,word showpath,char *label,
 *					char *path,char *name,char *command)
 * try to start file with commandline
 */
word startFile(WindInfo *wp,word fobj,word showpath,char *label,
				char *path,char *name,char *command)
{
	word retcode,tofree = FALSE;
	char *mycomm;
	word startmode,doit = TRUE;
	char filename[MAXLEN] = "";
	char *cp, prglabel[MAX_FILENAME_LEN];
	word wasApplRule = FALSE;
	word started = FALSE;
	
	GrafMouse(HOURGLASS,NULL);
	mycomm = command;			/* erstmal auf dasselbe zeigen */

	deselectObjects(0);
	if (wp)
		objselect(wp,fobj);
	flushredraw();

	cp = strrchr(name,'.');
	if(cp)
		cp++;				/* pointer to extension */

	if(cp && isExecutable(name))
	{

		if (checkLabel (path[0], label))
		{
			strcpy(filename,path);
			addFileName(filename,name);
			getStartMode(name, &startmode);
			if((startmode & TTP_START) && (mycomm == NULL))
			{
				GrafMouse(ARROW,NULL);
				mycomm = getCommandLine(name);
				GrafMouse(HOURGLASS,NULL);
				if(mycomm == NULL)
				{
					GrafMouse(ARROW,NULL);
					return FALSE;		/* Cancel gedrckt */
				}
				else
					tofree = TRUE;
			}
		}
	}
	else if(cp && !strcmp(cp,"ACC"))	/* is'n Acccessory */
	{
		started = StartAcc(wp,fobj,name,command);
		*filename = '\0';			/* don't start anything */
	}
	else
	{
		if (getApplForData(name, filename, prglabel, &startmode))
		{						/* zust„ndiges Programm */
			size_t len;
			
			if ((filename[1] != ':')
				|| ((path[0] != filename[0])
				|| sameLabel(prglabel,label)))
			{
				if (filename[1] == ':')
				{
					doit = checkLabel (filename[0], prglabel);
				}
				
				if(doit)
				{
					/* got programm from application-rules */
					wasApplRule = TRUE;
					
					len = strlen(path) + strlen(name) + 2;
					if(command)
						len += strlen(command) + 1;
					
					mycomm = malloc(len * sizeof(char));
					tofree = (mycomm != NULL);
					if(tofree)
					{
						strcpy(mycomm, "\'");
						strcat(mycomm, path);
						strcat(mycomm, name);
						strcat(mycomm, "\'");
					
						if(command)
						{
							strcat(mycomm, " ");
							strcat(mycomm, command);
						}
					}
				}
				else
					filename[0] = '\0';	/* don't execute */
			}
			else
			{
				venusErr(T_TWODISK);
				filename[0] = '\0'; /* don't execute */
			}
		}
		else
		{
			if (startmode == 0)
				venusErr(NlsStr(T_NOAPPL), name);
			filename[0] = '\0'; /* don't execute */
		}
	}
	
	if(strlen(filename))
	{
		if(isAccessory(filename))
		{
			started = StartAcc(wp,fobj,filename,mycomm);
		}
		else if((filename[1] != ':') || fileExists(filename))
		{
			if (wp)
				doFullGrowBox(wp->tree,fobj);
	
			started = TRUE;
			retcode = executer(startmode,showpath,filename,mycomm);
	
			if(retcode < 0)
				sysError(retcode);

		}
		else
		{
			if (wasApplRule)
			{
				if (venusChoice(NlsStr(T_DELAPPL), filename))
				{
					char *cp;

					cp = strrchr(filename, '\\');
					if (!cp)
						cp = filename;
					else
						++cp;
					if (!removeApplInfo(cp))
						venusDebug("Konnte applinfo nicht l”schen!");
				}
			}
			else
				venusErr(NlsStr(T_NOFILE), filename);
		}
	}
	
	if(tofree)
	{
		free(mycomm);
		mycomm = NULL;
	}
	GrafMouse(ARROW,NULL);
	
	return started;
}

word ClickInfo(WindInfo *wp, word object, word kstate)
{
	if ((kstate & K_CTRL) && ((kstate & K_ALT) == 0))
	{
		deselectObjects(0);
		objselect(wp,object);
		flushredraw();
		menu_tnormal(pmenu, FILES, FALSE);
		getInfoDialog();
		menu_tnormal(pmenu, FILES, TRUE);
		return TRUE;
	}
	return FALSE;
}

/*
 * manage doubleclick on object fobj in window wp
 */
static void Dclickon (WindInfo *wp, word fobj, word kstate)
{
	FileInfo *pf;
	
	pf = getfinfo (&wp->tree[fobj]);		/* Pointer on FileInfo */

	if (pf->attrib & FA_FOLDER)					/* is directory */
	{
		deselectObjects (0);
		objselect (wp, fobj);
		flushredraw ();
		GrafMouse (HOURGLASS, NULL);

		if (strcmp (pf->fullname, ".."))
		{
			if (strlen (wp->path) >= MAXLEN-28)
			{
				venusErr (NlsStr (T_NESTED));
				GrafMouse (ARROW, NULL);
				return;
			}

			if ((kstate & K_ALT) && ((kstate & K_CTRL) == 0))
			{
				char path[MAXLEN], title[MAXLEN];
				
				strcpy (path, wp->path);
				addFolderName (path, pf->fullname);
				strcpy (title, pf->fullname);
				strcat (title, "\\");
				openWindow (0, 0, 0, 0, 0, path,wp->wildcard,
								title, wp->label, WK_FILE);
			}
			else
			{
				storePathUndo (wp, pf->fullname, TRUE);
				addFolderName (wp->path, pf->fullname);
				addFolderName (wp->title, pf->fullname);
				pathchanged (wp);
				setFullPath (wp->path);
			}
		}
		else
		{
			if ((kstate & K_ALT) && ((kstate & K_CTRL) == 0))
			{
				char path[MAXLEN], fname[MAX_FILENAME_LEN];
				WindInfo *newwp;
				
				if (!getBaseName (fname, wp->path))
					fname[0] = '\0';

				strcpy (path, wp->path);
				stripFolderName (path);
				
				newwp = openWindow (0, 0, 0, 0, 0, path, wp->wildcard,
							path, wp->label, WK_FILE);

				if (newwp && strlen (fname))
					stringSelect (newwp, fname, FALSE);
			}
			else
			{
				closeWindow (wp->handle, TRUE);
			}
		}
		
		GrafMouse (ARROW, NULL);
	}
	else
	{
		startFile (wp, fobj, FALSE, wp->label, wp->path,
					pf->fullname, NULL);
	}
}

int getNewLocation(char *name, word isfolder)
{
	char tmp[1024];
	word retcode;
	
	sprintf(tmp, NlsStr(T_LOCATION), name);

	retcode = DialAlert(ImSqExclamation(), tmp, 0, 
				NlsStr(T_LOCBUTTON));
	
	if (retcode == 2)		/* Abbruch */
		return TRUE;
	else if (retcode == 0)	/* Entfernen */
		return FALSE;
	else					/* Suchen */
	{
		word ok;
		char filename[MAX_FILENAME_LEN];
		
		strcpy(tmp, name);
		
		if (isfolder)
		{
			stripFolderName(tmp);
			strcpy(filename, "*.*");
		}
		else
		{
			getBaseName(filename, tmp);
			stripFileName(tmp);
		}
		addFileName(tmp, "*.*");
		
		if (GotGEM14())
		{
			retcode = fsel_exinput(tmp, filename, &ok, 
						(char *)NlsStr(T_LOCOBJ));
		}
		else
			retcode = fsel_input(tmp, filename, &ok);
		
		if (retcode && ok)
		{
			stripFileName(tmp);
			if (!isfolder)
				addFileName(tmp, filename);
			
			if (strlen(tmp) > MAXLEN)
				venusErr(NlsStr(T_NESTED));
			else
				strcpy(name, tmp);
		}
		return TRUE;
	}
}

static void removeIcon(WindInfo *wp, word fobj)
{
	word r[4];
	
	objdeselect(wp, fobj);
	objc_offset(wp->tree, fobj, &r[0], &r[1]);
	r[2] = wp->tree[fobj].ob_width;
	r[3] = wp->tree[fobj].ob_height;
	removeDeskIcon(fobj);
	buffredraw(wp, r);
	flushredraw();
}

void iconDclick(WindInfo *wp, word fobj, word kstate)
{
	IconInfo *pi;
	word drive;
	char title[MAXLEN] = "";
	char fname[MAX_FILENAME_LEN], path[MAXLEN];
	char selname[MAX_FILENAME_LEN] = "";

	if(wp->kind == WK_DESK)
	{
		deselectObjects(0);
		objselect(wp,fobj);
		flushredraw();
		
		GrafMouse(HOURGLASS,NULL);

		pi = getIconInfo(&wp->tree[fobj]);
		switch(pi->type)
		{
			case DI_SHREDDER:
				venusInfo(NlsStr(T_SHREDDER));
				return;
			case DI_FOLDER:
				if (checkLabel (pi->path[0], pi->label))
				{
					if (!isDirectory(pi->path))
					{
						if (getNewLocation(pi->path, TRUE))
							rehashDeskIcon();
						else
							removeIcon(wp, fobj);
						*path = '\0';
						break;
					}
				}
				else
				{
					*path = '\0';
					break;
				}
			case DI_SCRAPDIR:
			case DI_TRASHCAN:
				if ((kstate & K_ALT) && ((kstate & K_CTRL) == 0))
				{
					strcpy(title,pi->path);
					stripFolderName(title);
					strcpy(path,title);
					getBaseName(selname, pi->path);
				}
				else
				{
					strcpy(title,pi->iconname);
					strcat(title,"\\");
					strcpy(path,pi->path);
				}
				break;
			case DI_FILESYSTEM:
				drive = (char)pi->path[0] - 'A';
				if(!legalDrive(drive))
				{
					venusErr(NlsStr(T_NODRIVE),pi->path[0]);
					return;
				}
				pi->path[1] = '\0';
				strcat(pi->path,":\\");
				strcpy(path,pi->path);
				if (!getLabel(drive,pi->label))
				{
					venusErr(NlsStr(T_NOLABEL), path[0]);
					*path = '\0';
				}
				break;
			case DI_PROGRAM:
				if (checkLabel (pi->path[0], pi->label))
				{
					if (!fileExists(pi->path))
					{
						if (getNewLocation(pi->path, FALSE))
							rehashDeskIcon();
						else
							removeIcon(wp, fobj);
						*path = '\0';
						break;
					}
				}
				else
				{
					*path = '\0';
					break;
				}
				strcpy(path,pi->path);
				getBaseName(fname, path);
				stripFileName(path);
				
				if ((kstate & K_ALT) && ((kstate & K_CTRL) == 0))
				{
					strcpy(title,path);
					strcpy(selname, fname);
				}
				else	/* start the program */
				{
					startFile(wp, fobj, TRUE, pi->label, path,
							fname, NULL);
					return;
				}
				break;
			default:
				venusDebug("unknown icon.");
				break;
		}
		if(strlen(path))
		{
			WindInfo *newwp;
			
			newwp = openWindow(0, 0, 0, 0, 0, path, "*", title,
								pi->label, WK_FILE);
			if (newwp && strlen(selname))
				stringSelect(newwp, selname, FALSE); 
		}
		GrafMouse(ARROW,NULL);
	}
	else
		Dclickon(wp, fobj, kstate);
}



/*
 * void doDclick(word mx,word my, word kstate)
 * manage doubleclick on position mx,my
 */
void doDclick(word mx,word my, word kstate)
{
	word fwind, fobj;
	WindInfo *wp;
	
	fwind = wind_find(mx,my);
	if((wp = getwp(fwind)) != NULL)
	{
		if (wp->kind == WK_MUPFEL)
		{
			if (!isDisabled(pmenu, TOMUPFEL))
			{
				word messbuff[8];
				
				menu_tnormal(pmenu,OPTIONS,0);
				messbuff[0] = MN_SELECTED;
				messbuff[1] = apid;
				messbuff[2] = 0;
				messbuff[3] = OPTIONS;
				messbuff[4] = TOMUPFEL;
				appl_write(apid,16,messbuff);
			}
			return;
		}

		switch (wp->kind)
		{
			case WK_DESK:
				fobj = FindObject(wp->tree, mx, my);
				break;
			case WK_FILE:
				fobj = objc_find(wp->tree, 0, MAX_DEPTH, mx, my);
				break;
			default:
				fobj = -1;
				break;
		}
		if (fobj <= 0)
			return;
			
		if (ClickInfo(wp, fobj, kstate))
			return;
			
		switch(wp->tree[fobj].ob_type)
		{
			case G_ICON:
				if(isOnIcon(mx,my,wp->tree,fobj))
				{
					iconDclick(wp,fobj,kstate);
				}
				break;
			case G_USERDEF:
				Dclickon(wp,fobj,kstate);
				break;
		}
	}
	GrafMouse(ARROW,NULL);
}

/*
 * simulate a doubleclick; called by selecting open
 */
void simDclick(word kstate)
{
	WindInfo *wp;
	word objnr,posx,posy;
	
	kstate &= ~K_CTRL;
	
	if(getOnlySelected(&wp,&objnr) && (objnr > 0))
	{
		objc_offset(wp->tree,objnr,&posx,&posy);
		
		switch (wp->tree[objnr].ob_type)
		{
			case G_ICON:
				iconDclick(wp, objnr, kstate);
				break;
			case G_USERDEF:
				Dclickon(wp, objnr, kstate);
				break;
		}
		GrafMouse(ARROW,NULL);
	}
}