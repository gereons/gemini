/*
 * @(#) Gemini\Window.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: functions to handle windows
 *
 */

#include <string.h>
#include <limits.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "stdbox.rh"

#include "stand.h"
#include "window.h"
#include "fileutil.h"
#include "util.h"
#include "myalloc.h"
#include "undo.h"
#include "files.h"
#include "venuserr.h"
#include "windstak.h"
#include "select.h"
#include "redraw.h"
#include "accwind.h"
#include "message.h"
#if MERGED
#include "mvwindow.h"
#include "..\mupfel\environ.h"
#endif

store_sccs_id(window);

#define NO_OBJECT	-1

/* externs 
*/
extern word deskx,desky,deskw,deskh;
extern word apid,wbox,hbox;
extern ShowInfo show;
extern DeskInfo NewDesk;
extern OBJECT *pmenu,*pstdbox;
extern char bootpath[];
extern WindInfo wnull;

/*
 * internal texts 
 */
#define NlsLocalSection "G.window"
enum NlsLocalText{
T_INFONORM,			/* %d %s, %ld %s*/
T_INFONORMSELECTED,	/* %d %s mit %ld %s selektiert*/
T_INFOEMPTY,		/* Leeres Verzeichnis*/
T_OBJECT,			/*Objekt*/
T_OBJECTS,			/*Objekte*/
T_BYTE,				/*Byte*/
T_BYTES,			/*Bytes*/
T_NODRIVE,			/*Kann kein Fenster îffnen, da das Laufwerk 
%c: GEMDOS nicht bekannt ist!*/
T_NOLABEL,			/*Kann das Label von Laufwerk %c: nicht lesen!*/
T_NOFOLDER,			/*Kann kein Fenster îffnen, da der Ordner 
%s nicht existiert!*/
T_NOWINDOW,			/*Kann kein Fenster îffnen, da das AES keine
 weiteren Fenster zur VerfÅgung stellt! Sie
 sollten ein unbenutzes Fenster schlieûen.*/
};

/* get Pointer from list of Windows */
WindInfo *getwp(word whandle)
{
	WindInfo *p = &wnull;
	
	do
	{
		if(p->handle == whandle)
		{
			return p;
		}
		p = p->nextwind;
	} while(p != NULL);			/* bis einmal rum */
	return (WindInfo *)NULL;
}

WindInfo *getMupfWp(void)
{
	WindInfo *p = &wnull;
	
	do
	{
		if(p->kind == WK_MUPFEL)
		{
			return p;
		}
		p = p->nextwind;
	} while(p != NULL);			/* bis einmal rum */
	return (WindInfo *)NULL;
}


/* get unused WindInfo struct (allocate one, if neccessary)
*/
WindInfo *newwp(void)
{
	WindInfo *p = wnull.nextwind;
	
	while(p != NULL)				/* bis einmal rum */
	{
		p = p->nextwind;
	}
	p = (WindInfo *)tmpmalloc(sizeof(WindInfo));	/* allocate */
	if (p != NULL)
	{
		memset(p, 0, sizeof(WindInfo));
		p->nextwind = wnull.nextwind;
		wnull.nextwind = p;
	}
	return p;
}

word freewp(word whandle)
{
	WindInfo *wp = wnull.nextwind;
	WindInfo *prevwp = &wnull;
	
	while(wp != NULL)
	{
		if(wp->handle == whandle)
		{
			prevwp->nextwind = wp->nextwind;
			if (wp->aesname)
				tmpfree(wp->aesname);
			tmpfree(wp);
			return TRUE;
		}
		prevwp = wp;
		wp = wp->nextwind;
	}
	return FALSE;
}

#if MERGED
void SetTOPWIND(void)
{
	WindInfo *wp = wnull.nextwind;
	
	while (wp && (wp->kind != WK_FILE))
		wp = wp->nextwind;
		
	if (wp)
	{
		char str[2*MAXLEN];
		
		sprintf(str, "TOPWIND=%s", wp->path);
		putenv(str);
	}
}
#endif

void setWpOnTop(word whandle)
{
	WindInfo *cp, *pp;
	
	if (whandle < 1)
		return;
	
	pp = &wnull;
	cp = pp->nextwind;
	
	while ((cp != NULL) && (cp->handle != whandle))
	{
		pp = cp;
		cp = pp->nextwind;
	}
	
	if ((cp != NULL) && (pp != &wnull))
	{
		pp->nextwind = cp->nextwind;
		cp->nextwind = wnull.nextwind;
		wnull.nextwind = cp;
	}
	SetTOPWIND();
}

/*
 * void cycleWindow(void)
 * setze hinterstes Fenster als neues oberster Fenster, via
 * appl_write() (auch an Accessories)
 */
void cycleWindow(void)
{
	WindInfo *wp;
	word messbuff[8];
	
	wp = &wnull;
	while(wp->nextwind)
		wp = wp->nextwind;

	if(wp->kind != WK_DESK)
	{
		messbuff[0] = WM_TOPPED;
		messbuff[1] = apid;
		messbuff[2] = 0;
		messbuff[3] = wp->handle;
		if (wp->owner != apid)
			appl_write(wp->owner, 16, messbuff);
		else
		{
			HandleMessage(messbuff, 0);
			setWpOnTop(wp->handle);
		}
	}
}

static void calcWindInfo(WindInfo *wp)
{
	word fileanz;
	
	if (wp->update & WINDINFO)
	{
		static const char *t_object = NULL;
		static const char *t_objects = NULL;
		static const char *t_byte = NULL;
		static const char *t_bytes = NULL;
		static const char *t_infonormselected = NULL;
		static const char *t_infonorm = NULL;
		static const char *t_infoempty = NULL;
		
		if (!t_object)
			t_object = NlsStr(T_OBJECT);
		if (!t_objects)
			t_objects = NlsStr(T_OBJECTS);
		if (!t_byte)
			t_byte = NlsStr(T_BYTE);
		if (!t_bytes)
			t_bytes = NlsStr(T_BYTES);
			
		fileanz = wp->fileanz;
		if(wp->files && !strcmp(wp->files->finfo[0].fullname,".."))
			fileanz--;
		
		if (wp->selectAnz)
		{
			if (!t_infonormselected)
				t_infonormselected = NlsStr(T_INFONORMSELECTED);
				
			sprintf(wp->info,t_infonormselected, wp->selectAnz,
				(wp->selectAnz>1)? t_objects : t_object,
				wp->selectSize,(wp->selectSize>1)? t_bytes : t_byte);
		}
		else if (fileanz)
		{
			if (!t_infonorm)
				t_infonorm = NlsStr(T_INFONORM);

			sprintf(wp->info,t_infonorm,
				fileanz,(fileanz>1)? t_objects : t_object,
				wp->dirsize,(wp->dirsize>1)? t_bytes : t_byte);
		}
		else
		{
			if (!t_infoempty)
				t_infoempty = NlsStr(T_INFOEMPTY);

			sprintf(wp->info, t_infoempty);
		}
	}
}

void calcWindData(WindInfo *wp)
{
	word seen_icons, total_icons, lost_icons;

	calcWindInfo(wp);
	
	if (wp->update & SLIDERRESET)
		wp->xskip = wp->yskip = 0;

	if (wp->kind == WK_FILE)
	{
		makeftree(wp);					/* built Objecttree */
	}

	if (wp->tree == NULL)
	{
		wp->tree = pstdbox;
		wp->objanz = countObjects(wp->tree,BOX);
	}

	if (wp->update & VESLIDER)
	{
		wp->vslpos = 0;
		if(wp->fileanz)
		{
			seen_icons = wp->workh / (wp->obh + 1);
			total_icons = wp->fileanz / wp->xanz;
			if(wp->fileanz % wp->xanz)	/* got a remainder */
				total_icons++;
			lost_icons = total_icons - seen_icons;
			if ((lost_icons > 0)&&(total_icons > 1))
			{
				wp->vslsize = (word)((1000L * seen_icons) / total_icons);
				wp->vslpos = (word)((1000L * wp->yskip) / lost_icons);
			}
			else
			{
				wp->vslsize = 1000;
			} 
		}
		else
			wp->vslsize = 1000;
	}
	if (wp->update & HOSLIDER)
	{
		wp->hslpos = 0;
		if (wp->fileanz > wp->xanz)
		{
			seen_icons = wp->workw / (wp->obw + wp->xdist);
			lost_icons = wp->xanz - seen_icons;
			if ((lost_icons > 0)&&(wp->xanz > 1))
			{
				wp->hslsize = (word)(1000L * seen_icons / wp->xanz);
				wp->hslpos = (wp->xskip)? 1000 : 0;
			}
			else
			{
				wp->hslsize = 1000;
			}
		}
		else
			wp->hslsize = 1000;
	}
}

static word newWindData(word whandle,char *label,char *path,
					char *wcard,char *title,word wx,word wy,
					word ww,word wh,word slpos,word kind,word type)
{
	WindInfo *wp;
	char physlabel[MAX_FILENAME_LEN];
	
	wp = newwp();
	if (wp == NULL)
		return FALSE;
	wp->handle = whandle;
	wp->kind = kind;
	wp->owner = apid;
	wp->type = type;
	wp->dirsize = wp->fileanz = wp->objanz = 0;
	strcpy(wp->wildcard,wcard);
	wp->selectAnz = 0;
	wp->selectSize = 0L;
	wp->xskip = 0;
	wp->yskip = slpos;
	wp->update = 0;

	if (label)
		strcpy(wp->label,label);
	else
		wp->label[0] = '\0';
	
	switch (wp->kind)
	{
		case WK_FILE:
			if (!legalDrive(path[0] - 'A'))
			{
				freewp(wp->handle);			/* structure freigeben */
				venusErr(NlsStr(T_NODRIVE),path[0]);
				return FALSE;
			}
			strncpy(wp->path,path,MAXLEN);
			wp->path[MAXLEN-1] = '\0';
		
			if (!strlen(title))				/* kein Titel da */
				strcpy(wp->title,wp->path);	/* Pfad als Titel */
			else
			{
				strncpy(wp->title,title,MAXLEN);
				wp->title[MAXLEN-1] = '\0';
			}
		
			if (!getLabel(wp->path[0] - 'A', physlabel))
			{
				freewp(wp->handle);		/* structure freigeben */
				venusErr(NlsStr(T_NOLABEL),path[0]);
				return FALSE;
			}
			
			if (*wp->label)
			{
				if (!sameLabel(physlabel,wp->label))
				{
					GrafMouse(ARROW,NULL);
					if (!changeDisk(path[0],wp->label))
					{
						freewp(wp->handle);
						return FALSE;
					}
					GrafMouse(HOURGLASS,NULL);
				}
			}
			else
				strcpy(wp->label,physlabel);
		
			while (!setFullPath(wp->path))
			{
				if (!stripFolderName(wp->path) || !stripFolderName(wp->title))
				{
					freewp(wp->handle);
					venusErr(NlsStr(T_NOFOLDER),path);
					return FALSE;
				}
			}
			break;

		case WK_MUPFEL:
				strncpy(wp->path,path,MAXLEN);
				strcpy(wp->title,title);
			break;
	}

	if (ww && wh)
	{
		wp->windx = wx;
		wp->windy = wy;
		wp->windw = ww;
		wp->windh = wh;
	}
	else if (!popWindBox(&wp->windx,&wp->windy,&wp->windw,&wp->windh))
	{
		wp->update |= POSITION;	/* window muû positioniert werden */
		if (wnull.nextwind->nextwind
			&& (wnull.nextwind->nextwind->kind == WK_FILE))
		{
			wp->windw = wnull.nextwind->nextwind->windw;
			wp->windh = wnull.nextwind->nextwind->windh;
		}
		else
			if (show.showtext)
			{
					wp->windw = deskw/2;
					wp->windh = deskh/2;
			}
			else
			{
					wp->windw = deskw/3;
					wp->windh = deskh;
			}
		
	}
	
	if (show.aligned)
		wp->windx = charAlign(wp->windx);
	
	wind_calc(TRUE,wp->type,wp->windx,wp->windy,wp->windw,wp->windh,
			&wp->workx,&wp->worky,&wp->workw,&wp->workh);	

	switch (wp->kind)
	{	
		case WK_FILE:
			getfiles(wp);						/* built filelist */
			wp->update |= NEWWINDOW;			/* update everything */
			break;
		case WK_MUPFEL:
			wp->update |= WINDNAME;
			break;
	}
	calcWindData(wp);
	return TRUE;
}

void setWindData(WindInfo *wp)
{
	if ((wp->type & NAME)&&(wp->update & WINDNAME))
	{
		char *name;
		size_t len;
		
		switch (wp->kind)
		{
		 	case WK_FILE:
				setFullPath(wp->path);
				if (wp->aesname)
				{
					tmpfree(wp->aesname);
					wp->aesname = NULL;
				}
				len = strlen(wp->title) + strlen(wp->wildcard) + 4;
				wp->aesname = tmpmalloc(len);
				if (wp->aesname)
				{
					strcpy(wp->aesname," ");
					strcat(wp->aesname, wp->title);
					if (strcmp(wp->wildcard, "*"))
						addFileName(wp->aesname, wp->wildcard);
					strcat(wp->aesname," ");
					name = wp->aesname;
				}
				else
					name = wp->title;
				break;
			default:
				name = wp->title;
				break;
				
		}
		wind_set(wp->handle, WF_NAME, name);
	}

	if ((wp->type & INFO)&&(wp->update & WINDINFO))
		wind_set(wp->handle,WF_INFO,wp->info);

	if ((wp->type & HSLIDE)&&(wp->update & HOSLIDER))
	{
		wind_set(wp->handle,WF_HSLIDE,wp->hslpos);
		wind_set(wp->handle,WF_HSLSIZE,wp->hslsize);
	}

	if ((wp->type & VSLIDE)&&(wp->update & VESLIDER))
	{
		wind_set(wp->handle,WF_VSLSIZE,wp->vslsize);
		wind_set(wp->handle,WF_VSLIDE,wp->vslpos);
	}
}

void UpdateWindowData(void)
{
	WindInfo *wp = wnull.nextwind;
	
	while (wp)
	{
		if ((wp->update & WINDINFO) 
			&& (wp->kind == WK_FILE || wp->kind == WK_MUPFEL))
		{
			if (wp->type & INFO)
			{
				calcWindInfo(wp);
				wind_set(wp->handle, WF_INFO, wp->info);
				wp->update &= ~WINDINFO;
			}
		}
		
		wp = wp->nextwind;
	}
}

static void centerWindow(WindInfo *wp,word r[4])
{
	word dx,dy;
	
	dx = (r[2] - wp->windw)>>1;	/* center over Rectangle */
	if(r[1] + wp->windh < desky + deskh)
		dy = 0;
	else
		dy = (r[3] - wp->windh)>>1;
	wp->windx = r[0] + dx;
	wp->windy = r[1] + dy;
	if(wp->windx < deskx)	/* align with desk Workarea */
		wp->windx = deskx;
	if(wp->windy < desky)
		wp->windy = desky;
	if(wp->windx > (dx = deskx + deskw - wp->windw))
		wp->windx = dx;
	if(wp->windy > (dy = desky + deskh - wp->windh))
		wp->windy = dy;
}

static void PosOnRect(WindInfo *wp)
{
	word wind[4],r[4];
	word max = 0;
	word horizontal,new,mx,my,bstate,kstate;
	
	ButtonPressed(&mx,&my,&bstate,&kstate);
	horizontal = (wp->windw > wp->windh);
	
	wind_get(0,WF_FIRSTXYWH,&r[0],&r[1],&r[2],&r[3]);
	while (r[2] && r[3])
	{
		new = (horizontal)? r[2] : r[3];
		if (pointInRect(mx,my,r))
			new /= 16;		/* mîglichst nicht auf Icon */
		if (max<=new)
		{
			max = new;
			wind[0] = r[0]; wind[1] = r[1];
			wind[2] = r[2]; wind[3] = r[3];
		}
		wind_get(0,WF_NEXTXYWH,&r[0],&r[1],&r[2],&r[3]);
	}
	
	if (max <= 0)
	{
		wind[2] = deskw / 2;
		wind[3] = deskh / 2;
		wind[0] = deskx + (wind[2] / 2);
		wind[1] = deskx + (wind[3] / 2);
	}
	centerWindow(wp,wind);
		
	wp->update &= ~POSITION;
}

WindInfo *openWindow(word wx, word wy, word ww, word wh, word slpos,
				char *path, char *wcard, char *title, char *label,
				word kind)
{
	word whandle;
	word wtype;
	WindInfo *wp;

	switch (kind)
	{
		case WK_FILE:
			wtype = NORMWIND;
			break;
		case WK_MUPFEL:
			wtype = MUPFWIND;
			break;
		default:
			return NULL;
	}
		
	GrafMouse(HOURGLASS,NULL);
	if (((whandle = wind_create(wtype,deskx,desky,deskw,deskh)) > 0)
		&&(newWindData(whandle,label,path,wcard,title,wx,wy,ww,wh,
					slpos,kind,wtype)))
	{
		if ((wp = getwp(whandle)) == NULL)
			return NULL;
		
		
		setWindData(wp);

		if (wp->update & POSITION)
			PosOnRect(wp);			/*freie FlÑche in Window 0*/

		wind_open(wp->handle,wp->windx,wp->windy,
					wp->windw,wp->windh);
		wind_get(wp->handle,WF_WORKXYWH,&wp->workx,&wp->worky,
				&wp->workw,&wp->workh);

		wp->tree[0].ob_x = wp->workx;
		wp->tree[0].ob_y = wp->worky;
		storeWOpenUndo(TRUE,wp);
#if MERGED
		if (wp->kind == WK_MUPFEL)
		{
			word r[4];
			
			openMWindow(wp);
			
			r[0] = wp->workx;
			r[1] = wp->worky;
			r[2] = wp->workw;
			r[3] = wp->workh;
			drawMWindow(wp, r);
			
			CommInfo.cmd = windOpen;
			m_mupfel();
			
			wp->update |= KILLREDRAW;
		}
		else
			SetTOPWIND();
#endif			
	}
	else
	{
		if (whandle > 0)				/* wind_create was a success */
			wind_delete(whandle);
		else
			venusErr(NlsStr(T_NOWINDOW));
		wp = NULL;
	}
	GrafMouse(ARROW,NULL);
	return wp;
}

static void sendWindClose(WindInfo *wp)
{
	word messbuff[8];
	
	messbuff[0] = WM_CLOSED;
	messbuff[1] = apid;
	messbuff[2] = 0;
	messbuff[3] = wp->handle;
	
	appl_write(wp->owner, 16, messbuff);
}

/* Schlieûe ein Fenster. Normalerweise wird das Fenster richtig
 * geschlossen und ist wech. Nur bei einem Datei-Fenster wird
 * der Pfad verkÅrzt, solange es geht. Speziell: Ist in dem Titel
 * des Fensters nicht mehr als ein Backslash, wird es auch 
 * geschlossen. Ist <goToParent> TRUE, so wird dasselbe nicht mit
 * dem Titel, sondern mit dem Pfad gemacht.
 */
void closeWindow (word whandle, word goToParent)
{
	WindInfo *wp;
	char *firstslash, *lastslash;
	char *cp;
	
	wp = getwp (whandle);
	if (!wp)
		return;

	switch (wp->kind)
	{
		case WK_FILE:
			firstslash = strchr (wp->title,'\\');
			lastslash = strrchr (wp->title,'\\');
			
			if (firstslash == lastslash)
			{
				if (goToParent)
				{
					strcpy (wp->title, wp->path);
					firstslash = strchr (wp->title, '\\');
					lastslash = strrchr (wp->title, '\\');
				}
				
				if (firstslash == lastslash)
				{
					deleteWindow (wp->handle);
					return;
				}
			}
			
			lastslash[0] = '\0';
			cp = strrchr (wp->title, '\\') + 1;
			storePathUndo (wp, cp, FALSE);	/* store undo cd .. */
			lastslash[0] = '\\';
			
			stripFolderName (wp->title);
			stripFolderName (wp->path);
			NewDesk.selectAnz -= wp->selectAnz;
			wp->selectAnz = 0;
			wp->selectSize = 0L;
			pathchanged (wp);
			break;

		case WK_MUPFEL:
			deleteWindow (wp->handle);
			break;

		case WK_ACC:
			sendWindClose (wp);
			break;

		default:
			break;
	}
}

/* Meldet ein Fenster ab, gibt Speicher frei, etc. Wenn <phys_del>
 * TRUE ist, wird das Fenster auch beim AES abgemeldet.
 */
static void delWindow (WindInfo *wp, word phys_del)
{
	storeWOpenUndo (FALSE, wp);
	NewDesk.selectAnz -= wp->selectAnz;
	wp->selectAnz = 0;
	wp->selectSize = 0L;

	switch (wp->kind)
	{
		case WK_FILE:
			pushWindBox (wp->windx, wp->windy, wp->windw, wp->windh);
			freefiles (wp);
			freeftree (wp);
			break;
		case WK_MUPFEL:
#if MERGED
			CommInfo.cmd = windClose;
			m_mupfel ();
			closeMWindow (wp);
			break;
#endif
		default:
			phys_del = FALSE;
			break;
	}
	if (phys_del)
	{
		wind_close (wp->handle);
		wind_delete (wp->handle);
	}

	freewp (wp->handle);
	if ((phys_del) && (wnull.nextwind == NULL))
	{							/* kein offenes Fenster mehr */
		setDrive (bootpath[0]);	/* Laufwerk auf Boot-Laufwerk */
	}							/* wegen érger mit system()  */
}

/* Front-End fÅr delWindow, das auf der WindInfo-Struktur zu
 * dem Fenster <whandle> operiert. Das Fenster wird richtig
 * gelîscht, auch wenn es ein mit VA-Protokoll angemeldetes
 * Accessory-Fenster ist (nur wird dann eine Message geschickt)
 */
void deleteWindow (word whandle)
{
	WindInfo *wp;
	
	wp = getwp (whandle);
	if (wp != NULL)
	{
		switch (wp->kind)
		{
			case WK_MUPFEL:
			case WK_FILE:
				delWindow (wp, TRUE);
				break;
			case WK_ACC:
				sendWindClose (wp);
				break;
		}
	}
}

/*
 * void sizeWindow(word whandle,word r[4])
 * Ñndere die Grîûe des Fenster und lasse die Icons
 * neu hineinflieûen; sendet bei Verkleinerung des Fensters
 * eine Redraw-Nachricht
 */
void sizeWindow (word whandle, word r[4])
{
	WindInfo *wp;
	word messbuff[8];
	
	wp = getwp (whandle);
	if ((wp == NULL) || (wp->kind != WK_FILE))
		return;
	
	/* Erzwinge eine Mindestgrîûe des Fensters
	 */	
	if (r[2] < (3 * wbox))
		r[2] = 3 * wbox;
	if (r[3] < 3 * hbox)
		r[3] = 3 * hbox;

	/* FÅhre die tatsÑchliche Verschiebung aus
	 */
	moveWindow (whandle,r);
	
	/* Wenn sich an der Anzahl der Spalten im Fenster etwas geÑndert
	 * hat, mÅssen wir immer das Fenster komplett neu zeichnen. Das
	 * kînnen wir nur durch eine Nachricht an uns selbst erzwingen.
	 */
	if (SizeFileTree (wp))
	{
		messbuff[0] = WM_REDRAW;
		messbuff[1] = apid;
		messbuff[2] = 0;
		messbuff[3] = wp->handle;
		messbuff[4] = wp->workx;
		messbuff[5] = wp->worky;
		messbuff[6] = wp->workw;
		messbuff[7] = wp->workh;
		appl_write (wp->owner, 16, messbuff);
	}
}

/* Das Rechteck <r> bestimmt die neuen Ausmaûe des Fensters <whandle>
 * Das Fenster wird auf die neuen Ausmaûe gesetzt und die alten
 * Ausmaûe werden im Undo-Buffer gesichert.
 */
void moveWindow (word whandle, word r[4])
{
	WindInfo *wp;
	
	wp = getwp(whandle);

	if (wp != NULL)
	{
		if (wp->kind == WK_ACC)
			return;
	
		if (show.aligned)
			r[0] = charAlign (r[0]);
			
		wind_set (whandle, WF_CURRXYWH, r[0], r[1], r[2], r[3]);
		storeWMoveUndo (wp,(wp->windw != r[2] || wp->windh != r[3]));
		
		wp->windx = r[0];
		wp->windy = r[1];
		wp->windw = r[2];
		wp->windh = r[3];
		
		wind_get (whandle, WF_WORKXYWH, &wp->workx, &wp->worky,
				&wp->workw, &wp->workh);
				
		switch (wp->kind)
		{
			case WK_FILE:
				wp->tree[0].ob_x = wp->workx;
				wp->tree[0].ob_y = wp->worky;
				break;
			case WK_MUPFEL:
#if MERGED
				moveMWindow (wp);
#endif
				break;
		}
	}
}

/* Das Fenster <whandle> wird auf volle Grîûe gebracht. Ist es
 * bereits auf voller Grîûe, wird die in <oldxywh> gemerkte Grîûe
 * wiederhergestellt.
 */
void fullWindow (word whandle)
{
	WindInfo *wp;
	word r[4];
	
	wp = getwp (whandle);
	if (wp == NULL)
		return;

#if MERGED
	if (wp->kind == WK_MUPFEL)
	{
		storeWFullUndo (wp);
		FullMWindow (wp);
		return;
	}
#endif

	if (wp->kind != WK_FILE)
		return;

	if (wp->oldw)			/* gleich null -> volle Grîûe */
	{
		r[0] = wp->oldx;
		r[1] = wp->oldy;
		r[2] = wp->oldw;
		r[3] = wp->oldh;
		wp->oldw = 0;
	}
	else
	{
		r[0] = deskx;
		r[1] = desky;
		r[2] = deskw;
		r[3] = deskh;
		if (show.aligned)
		{
			word pommes;
			
			pommes = charAlign (r[0]);
			r[2] -= (pommes - r[0]);
			r[0] = pommes;
		}
		wind_get (whandle, WF_CURRXYWH, &wp->oldx, &wp->oldy,
									&wp->oldw, &wp->oldh);
	}
	sizeWindow (whandle, r);
}

/* Schlieût alle eigenen Fenster. Wenn <wind_new()> zur VerfÅgung
 * steht, werden diese nicht richtig beim AES geschlossen, um
 * unnîtiges Neuzeichnen von Fenstern, die sowieso geschlossen
 * werden zu vermeiden.
 */
void closeAllWind (void)
{
	WindInfo *wp, *savwp;
	word anz = 0, savekind, x, y, w, h, phys_del;
	
	phys_del = !GotGEM14 ();

	wp = wnull.nextwind;

	while (wp != NULL)
	{
		anz++;
		savekind = wp->kind;
		savwp = wp->nextwind;
		
		delWindow (wp, phys_del);

		switch (savekind)
		{
			case WK_FILE:
				popWindBox (&x, &y, &w, &h);
				break;
		}
		wp = savwp;
	}
	
	killEvents (MU_MESAG, 2 * anz);	/* kill redraw-events (hope so) */
}

/*
 * Schlieût alle noch offenen Fenster, auch wenn sie uns nicht
 * gehîren. Dies ist natÅrlich unter einem MultiGEM absolut falsch.
 * Nur, wenn es eine gleichzeitig aktive Applikation gibt, mÅssen
 * wir so vorgehen.
 */
void delAccWindows (void)
{
	word topwind;
	char path[MAXLEN];
	
	/* Rette zur Sicherheit den Pfad, und stelle ihn nachher wieder
	 * her. Dies ist notwendig, da andere Routinen denken kînnten,
	 * es sei ein anderes Fenster oben, und dann selbst den Pfad neu
	 * setzen. Dies ist aber beim Starten von externen TOS-Programmen
	 * absolut nicht erwÅnscht.
	 */
	getFullPath (path);

	if (GotGEM14 ())
	{
		/* Wenn wir wind_new haben, rÑumen wir damit auf. Das geht
		 * am schnellsten.
		 */
		wind_new ();
		WindRestoreControl ();
	}
	else
	{
		/* Kein wind_new(). Wir machen es mÅhsam von Hand und holen
		 * uns nacheinander das Handle des obersten Fensters und
		 * schlieûen es.
		 */
		wind_get (0, WF_TOP, &topwind);
		while (topwind > 0)
		{
			if (getwp (topwind) != NULL)
			{
				deleteWindow (topwind);
			}
			else
			{
				wind_close (topwind);
				wind_delete (topwind);
			}
			wind_get (0, WF_TOP, &topwind);
		}
	}
	
	setFullPath (path);
}

#if MERGED
word openMupfelWindow (void)
{
	word wx, wy, ww, wh;

	SizeOfMWindow (&wx, &wy, &ww, &wh);	
	return (openWindow (wx, wy, ww, wh, 0, " ", " ", " Console ",
			" ", WK_MUPFEL) != NULL);
}
#endif

/* Ein Laufwerk ist mit einem neuen Label versehen worden. Durchsuche
 * die Liste von WindInfo-Strukturen nach betroffenen Fenstern
 * (irgendwie so betroffen) und Ñndere es, falls nîtig.
 */
void WindNewLabel (char drive, const char *oldlabel, 
						const char *newlabel)
{
	WindInfo *wp;
	
	wp = wnull.nextwind;
	while (wp != NULL)
	{
		switch (wp->kind)
		{
			case WK_FILE:
				if ((wp->path[0] == drive)
					&& (!strcmp (wp->label, oldlabel)))
				{
					strcpy (wp->label, newlabel);
				}
				break;
		}
		wp = wp->nextwind;
	}
}

/* Setze die Informationen, die zum Fenster <topwindow> gehîren,
 * da das nun das oberste Fenster sein wird.
 */
void SetTopWindowInfo (word topwindow)
{
	WindInfo *wp;
	
	if ((wp = getwp (topwindow)) != NULL)
	{
		if (wp->kind == WK_FILE)
			setFullPath (wp->path);
			
		setWpOnTop (wp->handle);
	}
}

/* éhnlich wie SetTopWindowInfo, nur wird auch das Fenster <whandle>
 * mittels des AES als oberstes Fenster installiert.
 */
void DoTopWindow (word whandle)
{
	SetTopWindowInfo (whandle);
	wind_set (whandle, WF_TOP);
}
