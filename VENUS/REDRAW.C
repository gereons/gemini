/*
 * @(#) Gemini\redraw.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: functions to redraw windows
 */

#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "redraw.h"
#include "venuserr.h"
#include "files.h"
#include "window.h"
#include "stand.h"
#include "fileutil.h"
#include "filedraw.h"
#if MERGED
#include "mvwindow.h"
#endif

store_sccs_id(redraw);

/* externals
*/
extern WindInfo wnull;
extern DeskInfo NewDesk;
extern ShowInfo show;
extern word deskx, desky, deskw, deskh;

/* internal texts
 */
#define NlsLocalSection		"Gmni.redraw"
enum NlsLocalText{
T_NOFOLDER,		/*Kann den Pfad %s nicht setzen! Das entsprechende
 Fenster wird geschlossen.*/
T_NOLABEL,		/*Kann das Label von Laufwerk %c: nicht lesen!
 Ist evtl. keine Diskette/Cartridge eingelegt?*/
};
					

/* internals
*/
static word currw[4];				/* local rectangle for buffered */
static WindInfo *currwp = NULL;		/* redraw */

/*
 * word rc_intersect(const word p1[4], word p2[4])
 * return if rectabgles intersect and store intersected
 * rectangle in p2
 */
word rc_intersect(const word p1[4], word p2[4])
{
	register word z1,z2;
	register word tx,ty,tw,th;
	
	z1 = p1[0] + p1[2];
	z2 = p2[0] + p2[2];
	tw = min(z1,z2);
	
	z1 = p1[1] + p1[3];
	z2 = p2[1] + p2[3];
	th = min(z1,z2);
	
	tx = max(p1[0], p2[0]);
	ty = max(p1[1], p2[1]);

	p2[0] = tx;
	p2[1] = ty;
	p2[2] = tw - tx;
	p2[3] = th - ty;

	return ((tw > tx) && (th > ty));
}

char VDIRectIntersect(word r1[4], word r2[4])
{
	word h_intersect, v_intersect;

	h_intersect = ! ((r1[2]<r2[0]) || (r1[0]>r2[2]));
	v_intersect = ! ((r1[3]<r2[1]) || (r1[1]>r2[3]));
	
	if (h_intersect && v_intersect)
	{
		if (r1[0]>r2[0]) r2[0] = r1[0];
		if (r1[2]<r2[2]) r2[2] = r1[2];
		if (r1[1]>r2[1]) r2[1] = r1[1];
		if (r1[3]<r2[3]) r2[3] = r1[3];
		return TRUE;
	}
	else
		return FALSE;
}

/*
 * word pointInRect(word px,word py,word r[4])
 * check if point px,py is in rectangle r
 */
word pointInRect(word px,word py,word r[4])
{
	px -= r[0]; py -= r[1];
	return ((px>0)&&(px<r[2])&&(py>0)&&(py<r[3]));
}

/*
 * static void mergerect(word r[4])
 * merge currw with r into currw
 */
static void mergerect(word r[4])
{
	word x1,y1,x2,y2;
	
	x1 = min(currw[0],r[0]);
	y1 = min(currw[1],r[1]);
	x2 = max(currw[0]+currw[2],r[0]+r[2]);
	y2 = max(currw[1]+currw[3],r[1]+r[3]);
	currw[0] = x1;
	currw[1] = y1;
	currw[2] = x2 - x1;
	currw[3] = y2 - y1;
}

/*
 * void flushredraw(void)
 * flush redraw which was buffered via buffredraw
 */
void flushredraw(void)
{
	if(currwp != NULL)
	{
		redraw(currwp,currw);
		currwp = NULL;
	}
}

/*
 * void buffredraw(WindInfo *wp,word w[4])
 * same as redraw(wp,r), but it buffers the redraws until
 * wp->handle changes or flushredraw is called.
 */
void buffredraw(WindInfo *wp,word w[4])
{
	if(currwp == NULL)
	{
		currw[0] = w[0];
		currw[1] = w[1];
		currw[2] = w[2];
		currw[3] = w[3];
		currwp = wp;
	}
	else
	{
		if(wp != currwp)
		{
			flushredraw();
			currw[0] = w[0];
			currw[1] = w[1];
			currw[2] = w[2];
			currw[3] = w[3];
			currwp = wp;
		}
		else
			mergerect(w);
	}
}

/*
 * void redraw(WindInfo *wp, word r1[4])
 * redraw rectangle r1 in window wp->handle
 */
void redraw(WindInfo *wp, word r1[4])
{
	OBJECT *tree;
	word r2[4];
	
	if(wp == NULL)
		return;
	
	if (wp->update & KILLREDRAW)
	{
		wp->update &= ~KILLREDRAW;
		return;
	}
	
	/* Mit dem Bildschirm klippen
	 */
	r2[0] = deskx;
	r2[1] = desky;
	r2[2] = deskx + deskw;
	r2[3] = desky + deskh;
	if (!rc_intersect(r2, r1))
		return;

	switch(wp->kind)
	{
		case WK_FILE:
			if (show.showtext)
			{
				optFileDraw(wp,r1);
				break;
			}
		case WK_DESK:
			tree = wp->tree;
			wind_get(wp->handle,WF_FIRSTXYWH,
						&r2[0],&r2[1],&r2[2],&r2[3]);
			while(r2[2]+r2[3])
			{
				if(rc_intersect(r1,r2))
				{
					objc_draw(tree,0,MAX_DEPTH,
								r2[0],r2[1],r2[2],r2[3]);
				}
				wind_get(wp->handle,WF_NEXTXYWH,
							&r2[0],&r2[1],&r2[2],&r2[3]);
			}
			break;
		case WK_MUPFEL:
#if MERGED
			redrawMWindow(wp,r1);
#endif
			break;
	}
}

void redrawObj(WindInfo *wp, word objnr)
{
	word r[4];
	OBJECT *tree;
	
	tree = wp->tree;
	objc_offset(tree, objnr, &r[0], &r[1]);
	r[2] = tree[objnr].ob_width;
	r[3] = tree[objnr].ob_height;
	buffredraw(wp,r);
}

/*
 * void rewindow(WindInfo *wp,word upflag)
 * update all things marked in upflag in Window wp->handle
 */
void rewindow(WindInfo *wp,word upflag)
{
	word oldupdate;
	word r[4];
	
	oldupdate = wp->update;
	wp->update = upflag;		/* neue Updates setzen */
	if (!(wp->update & NOWINDDRAW))
	{
		calcWindData(wp);
		setWindData(wp);
		if ((wp->kind != WK_MUPFEL) || (upflag & MUPFELTOO))
		{
			wind_get(wp->handle, WF_WORKXYWH, 
				&r[0], &r[1], &r[2], &r[3]);
			redraw(wp,r);
		}
		else
		{
			/* Das Console-Fenster ist nicht gemeint -> rette
			 * den Status (wegen KILLREDRAW)
			 */
			wp->update = oldupdate;
		}
	}
}

/*
 * void allrewindow(word upflag)
 * update things marked in upflag in all Windows
 * except Window 0
 */
void allrewindow(word upflag)
{
	WindInfo *wp;
	word num;
	MFORM form;
	
	wp = wnull.nextwind;
	GrafGetForm(&num, &form);
	GrafMouse(HOURGLASS,NULL);
	while(wp != NULL)
	{
		rewindow(wp,upflag);
		wp = wp->nextwind;		
	}
	GrafMouse(num, &form);
}

/*
 * get files in wp->path and display them in window wp->handle
 */
void pathchanged (WindInfo *wp)
{
	if (wp->kind == WK_FILE)
	{
		char tmp[MAXLEN];
		word num;
		MFORM form;
		char label[MAX_FILENAME_LEN];
		
		GrafGetForm (&num, &form);
		GrafMouse (HOURGLASS, NULL);
		strcpy (tmp, wp->path);
		
		if (!getLabel (wp->path[0] - 'A', label))
		{
			venusErr (NlsStr (T_NOLABEL), wp->path[0]);
		}
		else
		{
			while (!isDirectory (wp->path))
			{
				if (!stripFolderName (wp->title)
					|| !stripFolderName (wp->path))
				{
					venusErr (NlsStr (T_NOFOLDER), tmp);
					deleteWindow (wp->handle);
					GrafMouse (num, &form);
					return;
				}
			}
			if(getfiles (wp))
				rewindow (wp, NEWWINDOW|SLIDERRESET);
			else
				rewindow(wp, NEWWINDOW|NOWINDDRAW);

			if (strcmp (label, wp->label))
			{
				WindNewLabel (wp->path[0], wp->label, label);
			}
		}
		GrafMouse (num, &form);
	}
#if MERGED
	SetTOPWIND ();
#endif
}

/*
 * void fileChanged(WindInfo *wp)
 * update Window when files in wp->path have changed
 * (keeps sliderposition)
 */
void fileChanged(WindInfo *wp)
{
	if (wp->kind == WK_FILE)
	{
		word num;
		MFORM form;
		
		GrafGetForm(&num, &form);
		GrafMouse(HOURGLASS,NULL);

		if (!isDirectory (wp->path))
			pathchanged (wp);
		else
		{
			if(getfiles(wp))
				rewindow(wp,NEWWINDOW);
			else
				rewindow(wp,NEWWINDOW|NOWINDDRAW);
		}
		GrafMouse(num, &form);
	}
}

/*
 * void allFileChanged(word todraw)
 * read files for all windows and
 * update them (keeping slider position)
 * but don't do any redraw on screen
 */
void allFileChanged(word todraw)
{
	WindInfo *wp;
	word num;
	MFORM form;
	
	wp = wnull.nextwind;
	GrafGetForm(&num, &form);
	GrafMouse(HOURGLASS,NULL);
	while(wp != NULL)
	{
		if((wp->kind == WK_FILE) && getfiles(wp))
		{
			if(todraw)
				rewindow(wp,NEWWINDOW);
			else
			{
				wp->update = NEWWINDOW;		/* neue Updates setzen */
				calcWindData(wp);	/* neue Daten berechnen */
			}
		}
		wp = wp->nextwind;		
	}

#if MERGED
	/* wir haben alles neu eingelesen, also kein Update durch
	 * CommInfo.dirty mehr
	 */
	CommInfo.dirty = 0;
#endif

	GrafMouse(num, &form);
}
