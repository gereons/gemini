/*
 * @(#) Gemini\infofile.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: handle the venus.inf file
 */

#include <stdlib.h>
#include <string.h> 
#include <tos.h> 
#include <flydial\flydial.h> 
#include <nls\nls.h> 

#include "vs.h"
#include "menu.rh"
#include "sorttyp.rh"
#include "sorticon.rh"

#include "infofile.h"
#include "myalloc.h"
#include "venuserr.h"
#include "util.h"
#include "redraw.h"
#include "window.h"
#include "iconinst.h"
#include "fileutil.h"
#include "windstak.h"
#include "applmana.h"
#include "iconrule.h"
#include "iconhash.h"
#include "memfile.h"
#include "stand.h"
#include "filedraw.h"
#include "message.h"
#if MERGED
#include "mvwindow.h"
#endif

store_sccs_id(infofile);

/* externals
 */
extern word deskx,desky,deskw,deskh;
extern ShowInfo show;
extern DeskInfo NewDesk;
extern WindInfo wnull;
extern char bootpath[];
extern OBJECT *pmenu;
extern char wildpattern[5][WILDLEN];
extern char *sorttypstring, *sorticonstring;

/* interal texts
 */
#define NlsLocalSection "G.infofile"
enum NlsLocalText{
T_FORMAT,		/*Unbekanntes Format in der Datei %s!*/
T_CREATE,		/*Kann die Datei %s nicht anlegen!*/
T_NOBUFFER,		/*Es ist nicht genug Speicher vorhanden, um 
den Status zu sichern!*/
T_ERROR,		/*Fehler beim Schreiben von %s!*/
};

/*
 * L„nge des zu allozierenden Buffers fr sprintf
 */
#define BUFFLEN		1024

/* internals
 */
static char tmpline[CONF_LEN];
static ConfInfo *firstconf = NULL;

static void freeConfInfo(void)
{
	ConfInfo *aktconf;
	
	while(firstconf)
	{
		aktconf = firstconf->nextconf;
		free(firstconf->line);			/* string freigeben */
		free(firstconf);				/* structure freigeben */
		firstconf = aktconf;
	}
}

static void setShowInfo(ShowInfo *newshow)
{
	if(show.showtext)
	{
		menu_icheck(pmenu,BYTEXT,FALSE);
	}
	else
	{
		menu_icheck(pmenu,(show.normicon)? BYNOICON:BYSMICON,FALSE);
	}
	menu_icheck(pmenu,show.sortentry,FALSE);

	memcpy(&show,newshow,sizeof(ShowInfo));
	if(show.showtext)
	{
		menu_icheck(pmenu,BYTEXT,TRUE);
	}
	else
	{
		menu_icheck(pmenu,(show.normicon)? BYNOICON:BYSMICON,TRUE);
	}

	menu_text(pmenu, SORTTYPE, 
		(show.showtext)? sorttypstring : sorticonstring);

	if(show.sortentry < SORTNAME
		|| show.sortentry > UNSORT)
		show.sortentry = SORTNAME;
	menu_icheck(pmenu,show.sortentry,TRUE);
	
#if MERGED
	setInMWindow(newshow);
#endif
	
	allrewindow(SHOWTYPE|HOSLIDER|VESLIDER);
}

static word execScrapInfo(char *line)
{
	char *cp;
	word obx,oby;
	word shortcut = 0;
	char color = 0x10;
	char name[MAX_FILENAME_LEN];
	
	strtok(line,"@\n");
	if((cp = strtok(NULL,"@\n")) != NULL)
	{
		obx = atoi(cp);
		if((cp = strtok(NULL,"@\n")) != NULL)
		{
			oby = atoi(cp);
			if((cp = strtok(NULL,"@\n")) != NULL)
			{
				strcpy(name,cp);
				
				if ((cp = strtok(NULL,"@\n")) != NULL)
					shortcut = atoi(cp);
					
				if ((cp = strtok(NULL,"@\n")) != NULL)
					color = (char) atoi(cp);
					
				obx = checkPromille(obx,0);
				oby = checkPromille(oby,0);
				obx = (word)scale123(deskw,obx,1000L);
				oby = (word)scale123(deskh,oby,1000L);
				instScrapIcon(obx, oby, name, shortcut, color);
				return TRUE;
			}
		}
	}
	return FALSE;
}

static word execTrashInfo(char *line)
{
	char *cp;
	word obx,oby;
	word shortcut = 0;
	char color = 0x10;
	char name[MAX_FILENAME_LEN];
	
	strtok(line,"@\n");
	if((cp = strtok(NULL,"@\n")) != NULL)
	{
		obx = atoi(cp);
		if((cp = strtok(NULL,"@\n")) != NULL)
		{
			oby = atoi(cp);
			if((cp = strtok(NULL,"@\n")) != NULL)
			{
				strcpy(name,cp);

				if ((cp = strtok(NULL,"@\n")) != NULL)
					shortcut = atoi(cp);
					
				if ((cp = strtok(NULL,"@\n")) != NULL)
					color = (char)atoi(cp);
					
				obx = checkPromille(obx,0);
				oby = checkPromille(oby,0);
				obx = (word)scale123(deskw,obx,1000L);
				oby = (word)scale123(deskh,oby,1000L);
				instTrashIcon(obx, oby, name, shortcut, color);
				return TRUE;
			}
		}
	}
	return FALSE;
}

static word execShredderInfo(char *line)
{
	char *cp;
	word obx,oby;
	char color = 0x10;
	char name[MAX_FILENAME_LEN];
	
	strtok(line,"@\n");
	if((cp = strtok(NULL,"@\n")) != NULL)
	{
		obx = atoi(cp);
		if((cp = strtok(NULL,"@\n")) != NULL)
		{
			oby = atoi(cp);
			if((cp = strtok(NULL,"@\n")) != NULL)
			{
				strcpy(name,cp);

				if((cp = strtok(NULL,"@\n")) != NULL)
					color = (char)atoi(cp);
				
				obx = checkPromille(obx, 0);
				oby = checkPromille(oby, 0);
				
				obx = (word)scale123(deskw, obx, 1000L);
				oby = (word)scale123(deskh, oby, 1000L);
				instShredderIcon(obx, oby, name, color);
				return TRUE;
			}
		}
	}
	return FALSE;
}

static void execDriveInfo(char *line)
{
	word obx,oby,iconNr;
	word shortcut = 0;
	char color = 0x10;
	char drive,name[MAX_FILENAME_LEN],*cp;
	
	
	strtok(line,"@\n");		/* skip first */
	
	if((cp = strtok(NULL,"@\n")) != NULL)
	{
		obx = atoi(cp);
		if((cp = strtok(NULL,"@\n")) != NULL)
		{
			oby = atoi(cp);
			if((cp = strtok(NULL,"@\n")) != NULL)
			{
				iconNr = atoi(cp);
				if((cp = strtok(NULL,"@\n")) != NULL)
				{
					drive = *cp;
					if((cp = strtok(NULL,"@\n")) != NULL)
						strcpy(name,cp);
					else
						*name = '\0';

					if((cp = strtok(NULL,"@\n")) != NULL)
						shortcut = atoi(cp);

					if((cp = strtok(NULL,"@\n")) != NULL)
						color = (char)atoi(cp);

					obx = checkPromille(obx,0);
					oby = checkPromille(oby,0);

					obx = (word)scale123(deskw,obx,1000L);
					oby = (word)scale123(deskh,oby,1000L);
					instDriveIcon(obx, oby, FALSE, iconNr,
								drive, name, shortcut, color);
				}
			}
		}
	}
}

static void execBoxInfo(char *line)
{
	word bx,by,bw,bh;
	char *cp;
	
	strtok(line,"@\n");
	if((cp = strtok(NULL,"@\n")) != NULL)
	{
		bx = atoi(cp);
		if((cp = strtok(NULL,"@\n")) != NULL)
		{
			by = atoi(cp);
			if((cp = strtok(NULL,"@\n")) != NULL)
			{
				bw = atoi(cp);
				if((cp = strtok(NULL,"@\n")) != NULL)
				{
					bh = atoi(cp);

					bx = checkPromille(bx,0);
					by = checkPromille(by,0);
					
					bx = deskx + (word)scale123(deskw,bx,1000L);
					bw = deskx + (word)scale123(deskw,bw,1000L);
					by = desky + (word)scale123(deskh,by,1000L);
					bh = desky + (word)scale123(deskh,bh,1000L);
					pushWindBox(bx,by,bw,bh);
				}
			}
		}
	}
}

static void execPatternInfo(char *line)
{
	char *cp;
	
	strtok(line,"@\n");
	if((cp = strtok(NULL,"@\n")) != NULL)
	{
		strncpy(wildpattern[0],cp,WILDLEN-1);
		wildpattern[0][WILDLEN-1] = '\0';
		if((cp = strtok(NULL,"@\n")) != NULL)
		{
			strncpy(wildpattern[1],cp,WILDLEN-1);
			wildpattern[1][WILDLEN-1] = '\0';
			if((cp = strtok(NULL,"@\n")) != NULL)
			{
				strncpy(wildpattern[2],cp,WILDLEN-1);
				wildpattern[2][WILDLEN-1] = '\0';
				if((cp = strtok(NULL,"@\n")) != NULL)
				{
					strncpy(wildpattern[3],cp,WILDLEN-1);
					wildpattern[3][WILDLEN-1] = '\0';
					if((cp = strtok(NULL,"@\n")) != NULL)
					{
						strncpy(wildpattern[4],cp,WILDLEN-1);
						wildpattern[4][WILDLEN-1] = '\0';
					}
				}
			}
		}
	}
}

void readInfoDatei(const char *fname1, const char *fname2, word *tmp)
{
	MFileInfo *mp;
	ConfInfo *aktconf,myconfig;
	word gotdeficon;
	const char *fname = fname1;
	MFORM form;
	word num;
	
	GrafGetForm(&num, &form);
	GrafMouse(HOURGLASS,NULL);

	freeConfInfo();					/* free (existing?) list */
	myconfig.nextconf = NULL;
	aktconf = &myconfig;
	
	strcpy(tmpline,bootpath);
	addFileName(tmpline,fname);
	
	*tmp = fileExists(tmpline);
	if (!*tmp)
	{
		strcpy(tmpline,bootpath);
		addFileName(tmpline,fname2);
		fname = fname2;
	}
	
	gotdeficon = FALSE;

	if((mp = mopen(tmpline)) != NULL)
	{
		while(mgets(tmpline,CONF_LEN,mp) != NULL)
		{
			tmpline[CONF_LEN-1] = '\0';
			if(tmpline[0] == '#')
			{
				switch(tmpline[1])
				{
					case 'W':				/* Window */
					case 'I':				/* ShowInfo */
					case 'R':				/* IconRule */
					case 'A':				/* Applicationrules */
					case 'P':				/* ProgramIcon */
						aktconf->nextconf = 
								(ConfInfo *)malloc(sizeof(ConfInfo));
						if(aktconf->nextconf)
						{
							aktconf->nextconf->line = 
								malloc(strlen(tmpline)+1);
							if(aktconf->nextconf->line)
							{
								aktconf = aktconf->nextconf;
								strcpy(aktconf->line,tmpline);
								aktconf->nextconf = NULL;
							}
							else		/* malloc failed */
							{
								free(aktconf->nextconf);
								aktconf->nextconf = NULL;
							}
						}
						break;
					case 'E':				/* Eraser */
						gotdeficon = execShredderInfo(tmpline);
						break;
					case 'T':				/* Trashcan */
						gotdeficon = execTrashInfo(tmpline);
						break;
					case 'S':				/* Scrapicon */
						gotdeficon = execScrapInfo(tmpline);
						break;
					case 'D':				/* Driveicon */
						execDriveInfo(tmpline);
						break;
					case 'B':			/* windowbox */
						execBoxInfo(tmpline);
						break;
					case 'M':
						execPatternInfo(tmpline);
						break;
					case 'F':
						execFontInfo(tmpline);
						break;
					case 'V':
						ExecAccInfo(tmpline);
						break;
					default:
						venusErr(NlsStr(T_FORMAT),fname);
						return;
				}
			}
		}
		mclose(mp);
		if (fname == fname1)		/* tempor„re Datei */
		{
			strcpy(tmpline, bootpath);
			addFileName(tmpline, fname1);
			delFile(tmpline);
		}
	}
	if(!gotdeficon)
		addDefIcons();
	firstconf = myconfig.nextconf;
	GrafMouse(num, &form);
}

static void execInfoLine(const char *line)
{
	ShowInfo myshow;
	char *cp,*tp;

	cp = tmpmalloc((strlen(line)+1) * sizeof(char));
	if(cp)
	{
		strcpy(cp,line);
		strtok(cp,"@\n");
		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.aligned = atoi(tp);
		else
			myshow.aligned = TRUE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.normicon = atoi(tp);
		else
			myshow.normicon = TRUE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.fsize = atoi(tp);
		else
			myshow.fsize = TRUE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.fdate = atoi(tp);
		else
			myshow.fdate = FALSE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.ftime = atoi(tp);
		else
			myshow.ftime = FALSE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.showtext = atoi(tp);
		else
			myshow.showtext = TRUE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.sortentry = atoi(tp);
		else
			myshow.sortentry = SORTNAME;

		if((tp = strtok(NULL,"@\n")) != NULL)
			NewDesk.emptyPaper = atoi(tp);
		else
			NewDesk.emptyPaper = FALSE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			NewDesk.waitKey = atoi(tp);
		else
			NewDesk.waitKey = FALSE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			NewDesk.silentCopy = atoi(tp);
		else
			NewDesk.silentCopy = FALSE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			NewDesk.replaceExisting = atoi(tp);
		else
			NewDesk.replaceExisting = FALSE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.m_cols = atoi(tp);
		else
			myshow.m_cols = 80;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.m_rows = atoi(tp);
		else
			myshow.m_rows = 24;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.m_inv = atoi(tp);
		else
			myshow.m_inv = 0;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.m_font = atoi(tp);
		else
			myshow.m_font = 1;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.m_fsize = atoi(tp);
		else
			myshow.m_fsize = 6;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.m_wx = atoi(tp);
		else
			myshow.m_wx = 100;

		if((tp = strtok(NULL,"@\n")) != NULL)
			myshow.m_wy = atoi(tp);
		else
			myshow.m_wy = 100;

		if((tp = strtok(NULL,"@\n")) != NULL)
			NewDesk.silentRemove = atoi(tp);
		else
			NewDesk.silentRemove = FALSE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			NewDesk.showHidden = atoi(tp);
		else
			NewDesk.showHidden = FALSE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			NewDesk.askQuit = atoi(tp);
		else
			NewDesk.askQuit = FALSE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			NewDesk.saveState = atoi(tp);
		else
			NewDesk.saveState = FALSE;

		if((tp = strtok(NULL,"@\n")) != NULL)
			NewDesk.ovlStart = atoi(tp);
		else
			NewDesk.ovlStart = FALSE;

		tmpfree(cp);
		
		setShowInfo(&myshow);
	}
}

static void execWindLine(const char *line)
{
	word wx, wy, ww, wh, slpos, kind;
	char wpath[MAXLEN], title[MAXLEN];
	char wcard[WILDLEN], label[MAX_FILENAME_LEN], *cp, *tp;
	
	cp = tmpmalloc(strlen(line) + 1);
	if(cp)
	{
		wx=wy=ww=wh=slpos=0;		/* default */
		kind = WK_FILE;
		strcpy(cp,line);
		strtok(cp,"@\n");			/* skip first token */
		
		if((tp = strtok(NULL,"@\n")) != NULL)
			wx = atoi(tp);

		if((tp = strtok(NULL,"@\n")) != NULL)
			wy = atoi(tp);

		if((tp = strtok(NULL,"@\n")) != NULL)
			ww = atoi(tp);

		if((tp = strtok(NULL,"@\n")) != NULL)
			wh = atoi(tp);

		if((tp = strtok(NULL,"@\n")) != NULL)
			slpos = atoi(tp);

		if((tp = strtok(NULL,"@\n")) != NULL)
			kind = atoi(tp);

		if((tp = strtok(NULL,"@\n")) != NULL)
			strcpy(wpath,tp);
		else
		{
			tmpfree(cp);
			return;
		}

		if((tp = strtok(NULL,"@\n")) != NULL)
			strcpy(wcard,tp);

		if((tp = strtok(NULL,"@\n")) != NULL)
			strcpy(title,tp);
		else
			*title = '\0';

		if((tp = strtok(NULL,"@\n")) != NULL)
			strcpy(label,tp);
		else
			*label = '\0';

		tmpfree(cp);

		wx = checkPromille(wx,0);
		wy = checkPromille(wy,0);
		wx = deskx + (word)scale123(deskw,wx,1000L);
		ww = deskx + (word)scale123(deskw,ww,1000L);
		wy = desky + (word)scale123(deskh,wy,1000L);
		wh = desky + (word)scale123(deskh,wh,1000L);
		if (kind == WK_MUPFEL)
		{
#if MERGED
			word dx,dy;
			
			SizeOfMWindow(&dx,&dy,&ww,&wh);
#endif
		}
#if STANDALONE
		if (kind != WK_MUPFEL)
#endif
		if (!openWindow(wx,wy,ww,wh,slpos,
				wpath,wcard,title,label,kind))
		{
			pushWindBox(wx,wy,ww,wh);
		}
	}
}

static void execPrgLine(const char *line)
{
	word obx,oby,normicon,isfolder,ok;
	word shortcut = 0;
	char path[MAXLEN],name[MAX_FILENAME_LEN];
	char label[MAX_FILENAME_LEN],*cp,*tp;
	
	cp = tmpmalloc((strlen(line)+1) * sizeof(char));
	if(cp)
	{
		obx=oby=0;
		ok = TRUE;
		strcpy(cp,line);
		strtok(cp,"@\n");		/* skip first */
		
		if((tp = strtok(NULL,"@\n")) != NULL)
			obx = atoi(tp);

		if((tp = strtok(NULL,"@\n")) != NULL)
			oby = atoi(tp);

		if((tp = strtok(NULL,"@\n")) != NULL)
			normicon = atoi(tp);
			
		if((tp = strtok(NULL,"@\n")) != NULL)
			isfolder = atoi(tp);
			
		if((tp = strtok(NULL,"@\n")) != NULL)
			strcpy(path,tp);
		else
			ok = FALSE;
						
		if((tp = strtok(NULL,"@\n")) != NULL)
			strcpy(name,tp);
		else
			ok = FALSE;

		if((tp = strtok(NULL,"@\n")) != NULL)
		{
			strcpy(label, tp);
			if (!strcmp(label, " "))
				*label = '\0';
		}
		else
			*label = '\0';
		
		if((tp = strtok(NULL,"@\n")) != NULL)
			shortcut = atoi(tp);
		
		tmpfree(cp);

		if(ok)
		{
			obx = checkPromille(obx,0);
			oby = checkPromille(oby,0);
			obx = (word)scale123(deskw,obx,1000L);
			oby = (word)scale123(deskh,oby,1000L);
			instPrgIcon(obx, oby, FALSE, normicon, isfolder,
							path, name, label, shortcut);
		}
	}
}

void execConfInfo(word todraw)
{
	ConfInfo *aktconf;
	word gotrules = FALSE;
	word num;
	MFORM form;
	
	GrafGetForm(&num, &form);
	GrafMouse(HOURGLASS,NULL);
	
	freeIconRules();		/* Liste der Iconrules freigeben */
	freeApplRules();		/* Liste der applicationrules freigeben */
	aktconf = firstconf;
	
	while(aktconf != NULL)
	{
		if(aktconf->line[0] == '#')
		{
			switch(aktconf->line[1])
			{
				case 'I':
					execInfoLine(aktconf->line);
					break;
				case 'R':		/* Iconrules */
					addIconRule(aktconf->line);
					gotrules = TRUE;
					break;
				case 'A':		/* Applicationrules */
					addApplRule(aktconf->line);
					break;
				case 'P':				/* Programicon */
					execPrgLine(aktconf->line);
					break;
			}
		}
		aktconf = aktconf->nextconf;
	}

	if(!gotrules)
		insDefIconRules();
	builtIconHash();		/* Hashtable for Icon Rules */

	if(todraw)
		form_dial(FMD_FINISH,0,0,0,0,
					deskx,desky,deskx+deskw,desky+deskh);
	
	aktconf = firstconf;
	
	/* Arbeite alle Nachrichten, die bis hierhin angefallen sind
	 * ab. Ich hoffe, daž ich auch alle VA_PROTO, etc. hier schon
	 * bekommen, damit die Accs ihre Fenster zuerst ”ffnen.
	 */
	HandlePendingMessages ();
	
	while(aktconf)
	{
		if((aktconf->line[0] == '#')
			&& (aktconf->line[1] == 'W'))
		{
			execWindLine(aktconf->line);
		}
		aktconf = aktconf->nextconf;
	}

	freeConfInfo();			/* liste wieder freigeben */
#if MERGED
	/* Windows sind alle uptodate, also kein Update wegen dirty
	 */
	CommInfo.dirty = 0;
#endif
	
	GrafMouse(num, &form);
}

static ConfInfo *makeConfWp(ConfInfo *aktconf, WindInfo *wp,
					char *buffer)
{
	word wx,wy,ww,wh;

	if (wp != NULL)
	{
		aktconf = makeConfWp (aktconf, wp->nextwind, buffer);
		
		aktconf->nextconf = (ConfInfo *)malloc(sizeof(ConfInfo));
		if (aktconf->nextconf != NULL)
		{
			wx = (word)scale123 (1000L, wp->windx - deskx, deskw);
			ww = (word)scale123 (1000L, wp->windw - deskx, deskw);
			wy = (word)scale123 (1000L, wp->windy - desky, deskh);
			wh = (word)scale123 (1000L, wp->windh - desky, deskh);

			sprintf(buffer,
					"#W@%d@%d@%d@%d@%d@%d@%s@%s@%s@%s",
					wx, wy, ww, wh, wp->yskip, wp->kind, wp->path,
					wp->wildcard, wp->title, wp->label);

			aktconf->nextconf->line = malloc (strlen (buffer) + 1);
			if(aktconf->nextconf->line)
			{
				aktconf = aktconf->nextconf;
				aktconf->nextconf = NULL;
				strcpy (aktconf->line, buffer);
			}
			else
			{							/* malloc failed */
				free(aktconf->nextconf);
				aktconf->nextconf = NULL;
			}
			
		}
	}
	
	return aktconf;
}

/* Buffer fr sprintf()
 */
#define BUFFER_LEN		1024

/* Schreibe die aktuelle Konfiguration in ConfInfo-Strukturen
 */
void makeConfInfo (void)
{
	ConfInfo myconfig, *aktconf;
	word num;
	MFORM form;
	char buffer[BUFFER_LEN];
	
	GrafGetForm (&num, &form);
	GrafMouse (HOURGLASS, NULL);

	freeConfInfo ();				/* free (existing?) list */
	myconfig.nextconf = NULL;
	aktconf = &myconfig;

	myconfig.nextconf = (ConfInfo *)malloc (sizeof (ConfInfo));
	if (myconfig.nextconf != NULL)
	{
#if MERGED
		getInMWindow(&show);
#endif
		sprintf (buffer, "#I@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d"
				"@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d",
				show.aligned, show.normicon, show.fsize, show.fdate,
				show.ftime, show.showtext, show.sortentry,
				NewDesk.emptyPaper, NewDesk.waitKey,
				NewDesk.silentCopy, NewDesk.replaceExisting,
				show.m_cols, show.m_rows, show.m_inv, show.m_font,
				show.m_fsize, show.m_wx, show.m_wy, 
				NewDesk.silentRemove, NewDesk.showHidden,
				NewDesk.askQuit, NewDesk.saveState, NewDesk.ovlStart);
				
		aktconf->nextconf->line = malloc (strlen (buffer) + 1);
		if (aktconf->nextconf->line)
		{
			aktconf = aktconf->nextconf;
			aktconf->nextconf = NULL;
			strcpy (aktconf->line, buffer);
		}
		else
		{							/* malloc failed */
			free (aktconf->nextconf);
			aktconf->nextconf = NULL;
		}
	}
	
	aktconf = makeIconConf (aktconf, buffer);	/* Iconrules */
	aktconf = makeApplConf (aktconf, buffer);	/* Applicationrules */
	aktconf = makeConfWp (aktconf, wnull.nextwind, buffer);

	firstconf = myconfig.nextconf;

	GrafMouse (num, &form);
}

static word writeWildPattern(word fhandle, char *buffer)
{
	sprintf(buffer,"#M@%s@%s@%s@%s@%s",wildpattern[0],
									 wildpattern[1],
									 wildpattern[2],
									 wildpattern[3],
									 wildpattern[4]);
	return Fputs(fhandle, buffer);
}

void writeInfoDatei(const char *fname, word update)
{
	word fhandle;
	ConfInfo *aktconf;
	char *buffer;
	word num;
	MFORM form;
	
	GrafGetForm(&num, &form);
	GrafMouse(HOURGLASS,NULL);

	aktconf = firstconf;

	strcpy(tmpline,bootpath);
	addFileName(tmpline,fname);
	
	buffer = tmpmalloc(BUFFLEN);
	if (!buffer)
	{
		venusErr(NlsStr(T_NOBUFFER));
		return;
	}
	
	if((fhandle = Fcreate(tmpline, 0)) > 0)
	{
		word ok = TRUE;
		
		while (ok && (aktconf != NULL))
		{
			ok = Fputs(fhandle, aktconf->line);
			aktconf = aktconf->nextconf;
		}

		if (ok)
			ok = writeFontInfo(fhandle, buffer);
		if (ok)
			ok = writeWildPattern(fhandle, buffer);
		if (ok)
			ok = writeBoxes(fhandle, buffer);
		if (ok)							/* stack von Windowgr”žen */
			ok = writeDeskIcons(fhandle, buffer);
		if (ok)
			ok = WriteAccInfos(fhandle, buffer);

		Fclose(fhandle);
		
		if (!ok)
			venusErr(NlsStr(T_ERROR), fname);
			
		if (update)
			pathUpdate(bootpath,"");
	}
	else
		venusErr(NlsStr(T_CREATE),fname);

	tmpfree(buffer);
	freeConfInfo();
	GrafMouse(num, &form);
}
