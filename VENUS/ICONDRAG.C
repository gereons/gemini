/* 
 * @(#) Gemini\icondrag.c
 * @(#) Stefan Eissing, 21. Mai 1991
 *
 *	Iconstuff for Desktop-Icons (Dragging, Marking, Movin')
 */

#include <stdlib.h>
#include <vdi.h>
#include <string.h>
#include <limits.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "util.h"
#include "redraw.h"
#include "window.h"
#include "select.h"
#include "myalloc.h"
#include "venuserr.h"
#include "draglogi.h"
#include "message.h"
#include "wintool.h"
#include "stand.h"
#if MERGED
#include "mvwindow.h"
#endif
store_sccs_id(icondrag);

/* extern */
extern word deskx, desky, deskw, deskh;
extern word handle, pxy[256];			/* VDI Handle */
extern WindInfo wnull;
extern DeskInfo NewDesk;

/* internal texts
 */
#define NlsLocalSection "G.icondrag"
enum NlsLocalText{
T_GMEM,		/*Es ist nicht genug Speicher vorhanden!*/
T_MYWIND,	/*Auf dieses Fenster k”nnen Sie keine Objekte 
ziehen! Deshalb bleiben die Objekte dort, wo sie waren.*/
};


/*
 * Gibt zurck, ob der Punkt (mx, my) sich auf dem Icon
 * objnr im Tree tree befindet. Bercksichtigt Maske und
 * Textfahne des Objektes.
 * ACHTUNG: Diese Version funktioniert nur mit Objekten in
 * der ersten Ebene des Baumes!
 * 
 */
word isOnIcon(word mx, word my, OBJECT *tree, word objnr)
{
	ICONBLK *picon;
	
	picon = tree[objnr].ob_spec.iconblk;
	mx -= tree[0].ob_x + tree[objnr].ob_x;
	my -= tree[0].ob_y + tree[objnr].ob_y;

	if ((mx >= picon->ib_xicon)
		&&(mx < picon->ib_xicon + picon->ib_wicon)
		&&(my >= picon->ib_yicon)
		&&(my < picon->ib_yicon + picon->ib_hicon))
	{
/*		word *mask;
		
		mx -= picon->ib_xicon;
		my -= picon->ib_yicon;
		mask = picon->ib_pmask + (my * (picon->ib_wicon / 16))
				+ (mx / 16);
		mx = 0x8000 >> (mx % 16);
		return *mask & mx;
*/
		return TRUE;
	}
	else
	{
		return ((mx >= picon->ib_xtext)
			&&(mx < picon->ib_xtext + picon->ib_wtext)
			&&(my >= picon->ib_ytext)
			&&(my < picon->ib_ytext + picon->ib_htext));
	}
}

/*
 * static word isOnObject(word mx, word my, OBJECT *tree, word objnr)
 * decides if mx, my is on object objnr in tree
 */ 
static word isOnObject(word mx, word my, OBJECT *tree, word objnr)
{
	word obx, oby;
	
	switch(tree[objnr].ob_type)
	{
		case G_ICON:
				return isOnIcon(mx, my, tree, objnr);
		case G_USERDEF:
			objc_offset(tree, objnr, &obx, &oby);
			return ((mx >= obx)
					&&(mx < (obx + tree[objnr].ob_width))
					&&(my >= oby)
					&&(my < (oby + tree[objnr].ob_height)));
		default:
			return FALSE;
	}
}

/*
 * ermittelt das Icon an Position x, y. Befinden sich mehrere
 * Icons an der Position, so wird das zuletzt gefundene
 * zurckgegeben, da von unten nach oben sortiert wird.
 */
word FindObject(OBJECT *tree, word x, word y)
{
	word i, fobj = -1;
	
	for (i = tree[0].ob_head; i > 0; i = tree[i].ob_next)
	{
		if ((tree[i].ob_type == G_ICON) && isOnIcon(x, y, tree, i))
			fobj = i;
	}
	
	return fobj;
}

/*
 * static word isFolder(long obspec)
 * decide if object in a window is a folder
 */
static word isFolder(OBJECT *po)
{
	FileInfo *myinfo;
	
	myinfo = getfinfo(po);
	return (myinfo->attrib & FA_FOLDER);
}

/*
 * static void builtGhosts(WindInfo *wp, word deftype,
 *						word ghosts[], word *anz, word max[4])
 * built an array of rectangles for objects of type deftype
 * for anz objects and calculate the size of a box all ghosts
 * will fit in.
 */
static void builtGhosts(WindInfo *wp, word deftype,
						word ghosts[], word *anz, word max[4])
{
	register word ri;
	word i, j, obx, oby;
	OBJECT *tree;
	ICONBLK *picon;
	
	setBigClip(handle);
	*anz = j = 0;
	tree = wp->tree;
	max[0] = deskx + deskw - 1;
	max[1] = desky + deskh - 1;
	max[2] = deskx;
	max[3] = desky;

	for(i=wp->tree[0].ob_head; i>0; i=wp->tree[i].ob_next)
	{
		if((isSelected(tree, i))
			&&(tree[i].ob_type == deftype)	/* selektiertes Icon */
			&& !(isHidden(tree, i)))
		{
			objc_offset(tree, i, &obx, &oby);
			if(max[0] > obx)
				max[0] = obx;
			if(max[1] > oby)
				max[1] = oby;
			if(max[2] < (ri = obx + tree[i].ob_width - 1))
				max[2] = ri;
			if(max[3] < (ri = oby + tree[i].ob_height - 1))
				max[3] = ri;
			switch(deftype)
			{
				case G_ICON:
					*anz += 8;
					picon = tree[i].ob_spec.iconblk;
					ghosts[j++] = obx + picon->ib_xicon;
					ghosts[j++] = oby + picon->ib_yicon;
					ghosts[j++] = picon->ib_wicon;
					ghosts[j++] = picon->ib_hicon;
					ghosts[j++] = obx + picon->ib_xtext;
					ghosts[j++] = oby + picon->ib_ytext;
					ghosts[j++] = picon->ib_wtext;
					ghosts[j++] = picon->ib_htext;
					break;
				case G_USERDEF:
					*anz += 4;
					ghosts[j++] = obx;
					ghosts[j++] = oby;
					ghosts[j++] = tree[i].ob_width;
					ghosts[j++] = tree[i].ob_height;
					break;
			}
		}
	}
	MarkDraggedObjects(wp);		/* markiere zum Draggen */
}

/*
 * void drawRect(word r[4])
 * draw a rectangle on the screen, writing mode has to be set before
 */
void drawRect(word r[4])
{
	RastDotRect(handle, r[0], r[1], r[2], r[3]);
}

/* 
 * word dragIcons(word fromx, word fromy, word *tox, word *toy,
 *				word *towind, word *toobj, word *kfinalstate)
 * let user drag icons and return last mouseposition, on which
 * window and onject icons were dragged and the final state of
 * the leyboard
 */
static word dragIcons(word fromx, word fromy, word *tox, word *toy,
				word *towind, word *toobj, word *kfinalstate)
{
	word *ghosts;		    /* Speicherbereich fr Ghosts*/
	word ghostanz;			/* Anzahl der Ghosts */
	word maxbox[4];			/* gr”žte Ausmaže */
	word r[4];				/* redrawbox */
	word mxsav, mysav;		/* saven der Mausposition */
	word lastwind = -1;		/* Window, wo wir zuletzt waren */
	word lastobj = -1;		/* Object, das zuletzt selectiert war */
	word fwind, fobj;		/* Was wir so finden */
	word i, dx, dy, mx, my, rx, ry, bstate, kstate, pointanz;
	word deftype, fromwind, oldtoggle, newselect; 
	FileInfo *pf;
	WindInfo *wp, *lastwp;

	fromwind = wind_find(fromx, fromy);
	if(((wp = getwp(fromwind)) == NULL) 
		|| (wp->kind == WK_MUPFEL)
		|| (wp->kind == WK_ACC))
	{
		*tox = fromx;
		*toy = fromy;
		*towind = -1;
		*toobj = -1;
		return FALSE;
	}

	if (wp->kind == WK_DESK)
		fobj = FindObject(wp->tree, fromx, fromy);
	else
		fobj = objc_find(wp->tree, 0, MAX_DEPTH, fromx, fromy);
	if(fobj == -1)
	{
		*tox = fromx;
		*toy = fromy;
		*towind = -1;
		*toobj = -1;		
		return FALSE;
	}

	if((wp->kind == WK_FILE)			/* we're in a real window */
		&& wp->files->finfo[0].flags.selected)
	{
		/* Dies sollte eigentlich eine Routine in select.c sein */
		pf = &(wp->files->finfo[0]);
		if(!strcmp(pf->fullname, ".."))	/* It's the .. folder */
		{
			if(wp->selectAnz == 1)	/* the only selected */
			{
				*tox = fromx;		/* return */
				*toy = fromy;
				*towind = -1;
				*toobj = -1;		
				return FALSE;
			}
			else
			{
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
		}
	}

	switch(wp->tree[fobj].ob_type)
	{
		case G_ICON:
			deftype = G_ICON;
			pointanz = 10;
			break;
		case G_USERDEF:
			deftype = G_USERDEF;
			pointanz = 5;
			break;
		default:
			*tox = fromx;
			*toy = fromy;
			*towind = -1;
			*toobj = -1;
			return FALSE;
	}

	ghosts = tmpmalloc(wp->objanz * pointanz * 2 * sizeof(word));
	if (ghosts == NULL)
	{
		venusErr(NlsStr(T_GMEM));
		return FALSE;
	}
	builtGhosts(wp, deftype, ghosts, &ghostanz, maxbox); /* Ghosts berechnen */

	UpdateWindowData();
	
	GrafMouse(M_OFF, NULL);
	GrafMouse(FLAT_HAND, NULL);

	vswr_mode(handle, MD_XOR);
	vsl_color(handle, 1);
	vsf_interior(handle, FIS_HOLLOW);
	for (i = 0; i < ghostanz; i += 4)
		drawRect(&ghosts[i]);

	GrafMouse(M_ON, NULL);
	mxsav = fromx;							/* Mausposition sichern */
	mysav = fromy;
	maxbox[0] = deskx + (fromx - maxbox[0]);	/* Box fr Mausbewegung */
	maxbox[1] = desky + (fromy - maxbox[1]);
	maxbox[2] = deskx + deskw + (fromx - maxbox[2]) - 1;
	maxbox[3] = desky + deskh + (fromy - maxbox[3]) - 1;
	
	do 
	{
		ButtonPressed(&mx, &my, &bstate, &kstate);
		
		rx = mx; 
		ry = my;			/* wir brauchen mx noch */
		
		if(rx < maxbox[0])
			rx = maxbox[0];
		else if(rx > maxbox[2])		/* nur Bewegung in Box zulassen */
			rx = maxbox[2];
		if(ry < maxbox[1])
			ry = maxbox[1];
		else if(ry > maxbox[3])
			ry = maxbox[3];
		dx = rx - mxsav;
		dy = ry - mysav;
		mxsav = rx; mysav = ry;
		oldtoggle = newselect = FALSE;
		
		fwind = wind_find(mx, my);
		
		if (((wp = getwp(fwind)) != NULL) 
			&& ((wp->kind == WK_FILE) || (wp->kind == WK_DESK)))
		{
			if (wp->kind == WK_DESK)
				fobj = FindObject(wp->tree, mx, my);
			else
				fobj = objc_find(wp->tree, 0, MAX_DEPTH, mx, my);
			if (fobj != -1)
			{
				switch (wp->tree[fobj].ob_type)
				{
					case G_ICON:
						if (!isOnIcon(mx, my, wp->tree, fobj)
							|| ((wp->kind == WK_FILE)
								&&(!isFolder(&wp->tree[fobj]))))
							fobj = -1;
						break;
					case G_USERDEF:
						if ((wp->kind == WK_FILE)
							&&(!isFolder(&wp->tree[fobj])))
							fobj = -1;
						break;
					default:
						fobj = -1;
						break;
				}
			}
		}
		else
			fobj = -1;				/* nicht unser Window */
			
		if((lastwind != -1)&&(lastobj != -1))
		{
			lastwp = getwp(lastwind);
			switch(lastwp->tree[lastobj].ob_type & 0x00FF)
			{
				case G_ICON:
				case G_USERDEF:
					oldtoggle = ((!isOnObject(mx, my, lastwp->tree, lastobj))
								||(lastwind != fwind));
					break;
			}
		}
		if((fwind != -1)
			&&(fobj != -1)
			&&(lastwind!=fwind||lastobj!=fobj))
		{
			if(!(wp->tree[fobj].ob_type & DRAG))
				switch(wp->tree[fobj].ob_type & 0x00FF)
				{
					case G_ICON:
					case G_USERDEF:
							newselect = TRUE;
							oldtoggle = (lastobj != -1);
						break;
					default: fobj = -1;
						break;
				}
		}
		if(dx||dy||oldtoggle||newselect)	/* Maus wurde bewegt */
		{
			GrafMouse(M_OFF, NULL);
			vswr_mode(handle, MD_XOR);
			vsl_color(handle, 1);
			vsf_interior(handle, FIS_HOLLOW);
			for(i=0;i<ghostanz;i+=4)
			{
				drawRect(&ghosts[i]);
				ghosts[i] += dx;		/* Ghosts verschieben */
				ghosts[i+1] += dy;
			}
			if(oldtoggle)
			{
				lastwp->tree[lastobj].ob_state &= ~SELECTED;
				NewDesk.selectAnz--;
				lastwp->selectAnz--;
				objc_offset(lastwp->tree, lastobj, &r[0], &r[1]);
				r[2] = lastwp->tree[lastobj].ob_width;
				r[3] = lastwp->tree[lastobj].ob_height;
				redraw(lastwp, r);
			}
			if(newselect)
			{
				wp->tree[fobj].ob_state |= SELECTED;
				NewDesk.selectAnz++;
				wp->selectAnz++;
				objc_offset(wp->tree, fobj, &r[0], &r[1]);
				r[2] = wp->tree[fobj].ob_width;
				r[3] = wp->tree[fobj].ob_height;
				redraw(wp, r);
			}
			
			if(oldtoggle||newselect)
			{
				lastwind = fwind;
				lastobj = fobj;	
			}

			vswr_mode(handle, MD_XOR);
			vsl_color(handle, 1);
			vsf_interior(handle, FIS_HOLLOW);
			for(i=0;i<ghostanz;i+=4)
				drawRect(&ghosts[i]);
			GrafMouse(M_ON, NULL);
		}
	} while(bstate & 0x0001);
	
	GrafMouse(M_OFF, NULL);
	vswr_mode(handle, MD_XOR);
	vsl_color(handle, 1);
	vsf_interior(handle, FIS_HOLLOW);
	for(i=0;i<ghostanz;i+=4)
		drawRect(&ghosts[i]);
	GrafMouse(ARROW, NULL);
	GrafMouse(M_ON, NULL);

	if((fromwind==0)&&(lastobj==-1))	/* Moving only on Window 0 */
	{
		*tox = mxsav;			
		*toy = mysav;
	}
	else
	{
		*tox = mx;				/* return where we were */
		*toy = my;
	} 
	*towind = fwind;
	*toobj = fobj;
	*kfinalstate = kstate;
	tmpfree(ghosts);
	return TRUE;
}

static void drawRect2(word x1, word y1, word x2, word y2)
{
	word w, h;
	word r[4], windrect[4];
	word counter = 0;

	if (x1 < x2)
	{
		r[0] = x1;
		r[2] = x2;
	}
	else
	{
		r[0] = x2;
		r[2] = x1;
	}
	if (y1 < y2)
	{
		r[1] = y1;
		r[3] = y2;
	}
	else
	{
		r[1] = y2;
		r[3] = y1;
	}
	
	w = r[2] - r[0] + 1;	
	h = r[3] - r[1] + 1;
	
	while (WT_GetRect(counter++, windrect))
	{
		if (VDIRectIntersect(r, windrect))
		{
			vs_clip(handle, 1, windrect);
			RastDotRect(handle, r[0], r[1], w, h);
		}
	}
}

#undef max
#undef min

static word max(word x, word y)
{
	return x>y?x:y;
}

static word min(word x, word y)
{
	return x<y?x:y;
}

static void rubberBox(WindInfo *wp, word mx, word my, word r[4])
{
	word kstate, bstate, curx, cury, oldx, oldy;
	word maxbox[4];
	word num;
	MFORM form;
	
	/* Speichere Rechteckliste von Fenster wp
	 */
	WT_BuildRectList(wp->handle);
	
	vswr_mode(handle, MD_XOR);
	vsl_color(handle, BLACK);
	vsf_interior(handle, FIS_HOLLOW);
	
	setBigClip(handle);
	oldx = mx+1;
	oldy = my+1;
	maxbox[0] = wp->workx;
	maxbox[1] = wp->worky;
	maxbox[2] = wp->workx + wp->workw - 1;
	maxbox[3] = wp->worky + wp->workh - 1;

	GrafGetForm(&num, &form);
	GrafMouse(M_OFF, NULL);
	GrafMouse(POINT_HAND, NULL);
	drawRect2(mx, my, oldx, oldy);
	GrafMouse(M_ON, NULL);
	do
	{
		ButtonPressed(&curx, &cury, &bstate, &kstate);
		if (oldx == curx && oldy == cury)
			continue;

		if(curx < maxbox[0])
		{
			curx = maxbox[0];
		}
		else if(curx > maxbox[2])/* nur Bewegung in Box zulassen */
		{
			curx = maxbox[2];
		}
		if(cury < maxbox[1])
		{
			cury = maxbox[1];
		}
		else if(cury > maxbox[3])
		{
			cury = maxbox[3];
		}

		if ((curx - oldx) || (cury - oldy))
		{
			GrafMouse(M_OFF, NULL);
			drawRect2(mx, my, oldx, oldy);
			oldx = curx;
			oldy = cury;
			drawRect2(mx, my, oldx, oldy);
			GrafMouse(M_ON, NULL);
		}
	
	} while (bstate & 1);

	GrafMouse(M_OFF, NULL);
	drawRect2(mx, my, oldx, oldy);
	GrafMouse(num, &form);
	GrafMouse(M_ON, NULL);
	r[0] = min(mx, oldx);
	r[1] = min(my, oldy);
	r[2] = max(mx, oldx) - r[0] + 1;
	r[3] = max(my, oldy) - r[1] + 1;

#if MERGED
	if ((wp = getMupfWp()) != NULL)
		moveMWindow(wp);	/* built new rectangle list */
#endif
}

/*
 * static void dorubberbox(WindInfo *wp, word mx, word my)
 * let user mark icons with rubberbox
 */
static void dorubberbox(WindInfo *wp, word mx, word my)
{
	word r[4], r2[4], i;
	OBJECT *tree;
	
	rubberBox(wp, mx, my, r);
	tree = wp->tree;
	for(i=wp->tree[0].ob_head; i>0; i=wp->tree[i].ob_next)
	{
		if(tree[i].ob_type == G_ICON)
		{
			word ix, iy, text[4];
			ICONBLK *picon;
			
			picon = tree[i].ob_spec.iconblk;
			objc_offset(tree, i, &ix, &iy);
			r2[0] = ix + picon->ib_xicon;
			r2[1] = iy + picon->ib_yicon;
			r2[2] = picon->ib_wicon;
			r2[3] = picon->ib_hicon;
			text[0] = ix + picon->ib_xtext;
			text[1] = iy + picon->ib_ytext;
			text[2] = picon->ib_wtext;
			text[3] = picon->ib_htext;
			
			if(rc_intersect(r, r2) || rc_intersect(r, text))
			{
				if(tree[i].ob_state & SELECTED)
					objdeselect(wp, i);
				else
					objselect(wp, i);
			}
		}
		else if	(tree[i].ob_type == G_USERDEF)
		{
			objc_offset(tree, i, &r2[0], &r2[1]);
			r2[2] = tree[i].ob_width;
			r2[3] = tree[i].ob_height;
			if(rc_intersect(r, r2))
			{
				if(tree[i].ob_state & SELECTED)
					objdeselect(wp, i);
				else
					objselect(wp, i);
			}
		}
	}
	flushredraw();
}

/*
 * void doIcons(WindInfo *wp, word mx, word my, word kstate)
 * manage different types of action the user may want to 
 * do with icons (selecting,  dragging etc.)
 */
void doIcons(WindInfo *wp, word mx, word my, word kstate)
{
	WindInfo *towp;
	word bstate, foo, whandle;
	word fobj, tox, toy, towind, toobj;
	OBJECT *tree = wp->tree;

	switch (wp->kind)
	{
		case WK_DESK:
			fobj = FindObject(tree, mx, my);
			break;
		case WK_FILE:
			fobj = objc_find(tree, 0, MAX_DEPTH, mx, my);
			if (!isOnObject(mx, my, tree, fobj))
				fobj = -1;
			break;
		default:
			fobj = -1;
			break;
	}
	
	if (fobj > 0)
	{
		if(kstate & (K_LSHIFT | K_RSHIFT))  /* Shifttaste gedrckt */
		{
			desObjExceptWind(wp, 0);		/* alle in anderen Windows
										 * deselektieren
										 */
			if(tree[fobj].ob_state & SELECTED)
			{
				objdeselect(wp, fobj);	/* deselektieren */
				flushredraw();
				while (ButtonPressed(&foo, &foo, &bstate, &foo))
					;
			}
			else
				objselect(wp, fobj);		/* selektieren */

			flushredraw();
		}
		else if (!isSelected(tree, fobj))
		{
			deselectObjects(0);		/* alle Icons deselektieren */
			objselect(wp, fobj);	/* nur dies selektieren */
			flushredraw();
		}

		
		ButtonPressed(&foo, &foo, &bstate, &foo);

		if((tree[fobj].ob_state & SELECTED)  /* ICON war selektiert */	
			&& (bstate & 0x0001))		/* left button pressed */
		{
			word kstate, retcode;
			
			retcode = dragIcons(mx, my,	&tox, &toy, &towind,
								&toobj, &kstate);
			/* ziehe Icons bern Tisch */

			whandle = wp->handle;
			towp = getwp(towind);
			
			if ((towp == NULL) || (towp->kind == WK_ACC))
			{
				if (retcode)
					if (!PasteAccWindow(wp, towind, tox, toy))
						venusErr(NlsStr(T_MYWIND));
			}
			else
			{
				doDragLogic(wp, towp, toobj, kstate, mx, my, tox, toy);
			}
			/*
			 * programm kann gestartet worden
			 * sein, deshalb pointer neu holen
			 */
			if((wp = getwp(whandle)) != NULL)
				UnMarkDraggedObjects(wp);
		}
	}
	else
	{
		word rubber;
		
		ButtonPressed(&foo, &foo, &bstate, &kstate);
		rubber = (bstate & 0x0001);
		desObjExceptWind(wp, 0);
		if(!kstate & (K_LSHIFT|K_RSHIFT))	/* keine Shifttaste? */
			desWindObjects(wp, 0);
		if(rubber)
		{
			word r[4];
			
			r[0] = wp->workx;
			r[1] = wp->worky;
			r[2] = wp->workw;
			r[3] = wp->workh;
			if (pointInRect(mx, my, r))
				dorubberbox(wp, mx, my);
		}
	}
	
	UpdateWindowData();
}
