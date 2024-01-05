/*
 * @(#) Gemini\scroll.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: Routinen zum Scrolling in Desktopwindows 
 */

#include <ctype.h> 
#include <vdi.h>
#include <flydial\flydial.h>

#include "vs.h"
#include "menu.rh"

#include "scroll.h"
#include "myalloc.h"
#include "redraw.h"
#include "window.h"
#include "select.h"

store_sccs_id(scroll);

/* externs
 */
extern DeskInfo NewDesk;
extern ShowInfo show;
extern word deskx, desky, deskw, deskh;
extern word wchar;
extern handle;

static void setScrollData(WindInfo *wp)
{
	wp->update = VESLIDER;		/* neues Update setzen */
	calcWindData(wp);
	wind_set(wp->handle, WF_VSLIDE, wp->vslpos);
}

void scrollit(WindInfo *wp, word div_skip, word dodraw)
{
	MFDB src_mfdb, dest_mfdb;
	word srcr[4], r[4], pxy[256], screen[4];
	word div_pix;
	register word i;
	
	if (!dodraw)
	{
		setScrollData(wp);
		return;
	}
	
	/* Clipping auf Bildschirmgrenze einschalten
	 */
	screen[0] = deskx;
	screen[1] = desky;
	screen[2] = deskx + deskw - 1;
	screen[3] = desky + deskh - 1;
	vs_clip(handle, TRUE, screen);
	
	/* Blitten auf dem Bildschirm */
	src_mfdb.fd_addr = dest_mfdb.fd_addr = NULL;
	
	
	r[0] = srcr[0] = pxy[4] = wp->workx;
	r[2] = wp->workw;
	srcr[2] = pxy[6] = wp->workx + wp->workw - 1;
	div_pix = div_skip * (wp->obh + 1);

	if(div_skip > 0)				/* scrolling up */
	{
		r[1] = srcr[1] = wp->worky;
		srcr[3] = wp->worky + wp->workh - div_pix - 1;
		r[3] = wp->workh - (srcr[3] - srcr[1]);
	}
	else							/* scrolling down */
	{
		srcr[1] = wp->worky - div_pix;
		srcr[3] = wp->worky + wp->workh - 1;
		r[1] = wp->worky + (srcr[3] - srcr[1]);
		if(desky + deskh < srcr[3])
			r[1] -= srcr[3] - desky - deskh;
		r[3] = wp->workh - (srcr[3] - srcr[1]);
	}
	
	if(srcr[1] < srcr[3])			/* ist noch was zu sehen */
	{
		for(i = 0; i < 4; i++)
			pxy[i] = srcr[i];
			
		pxy[5] = srcr[1] + div_pix;
		pxy[7] = srcr[3] + div_pix;
			
		setScrollData(wp);
		GrafMouse(M_OFF,NULL);
		
		if (VDIRectIntersect(screen, pxy)
			&& VDIRectIntersect(screen, &pxy[4]))
		{
			vro_cpyfm(handle, 3, pxy, &src_mfdb, &dest_mfdb);
		}

		redraw(wp, r);
		GrafMouse(M_ON,NULL);
	}
	else
		rewindow(wp,VESLIDER);
}

static void linedown(WindInfo *wp)
{
	if((wp->vslpos < 1000)&&(wp->vslsize < 1000))
	{
		wp->yskip++;
		scrollit(wp, -1, TRUE);
	}
}

static void lineup(WindInfo *wp)
{
	if(wp->yskip)
	{
		wp->yskip--;
		scrollit(wp, 1, TRUE);
	}
}

static void pageup(WindInfo *wp)
{
	word seen_lines,old_skip;
	
	if(wp->vslpos > 0 && wp->vslsize < 1000)
	{
		seen_lines = wp->workh / (wp->obh + 1);
		old_skip = wp->yskip;
	
		if(wp->yskip > seen_lines)
			wp->yskip -= seen_lines;
		else
			wp->yskip = 0;

		if(old_skip != wp->yskip)
			scrollit(wp, old_skip - wp->yskip, TRUE);
	}
}

static void pagedown(WindInfo *wp)
{
	word total_lines,seen_lines;
	word old_skip;
	
	if(wp->vslpos < 1000 && wp->vslsize < 1000)
	{
		total_lines = wp->fileanz / wp->xanz;
		if(wp->fileanz % wp->xanz)
			total_lines++;
		seen_lines = wp->workh / (wp->obh + 1);
		old_skip = wp->yskip;	

		if(wp->yskip + seen_lines <= total_lines - seen_lines)
			wp->yskip += seen_lines;
		else
			wp->yskip = total_lines - seen_lines;

		if(old_skip != wp->yskip)
			scrollit(wp, old_skip - wp->yskip, TRUE);
	}
}

void doArrowed(word whandle,word desire)
{
	WindInfo *wp;
	
	if((wp = getwp(whandle)) != NULL)
	{
		switch(desire)
		{
			case WA_UPPAGE:		/* Seite aufw„rts */
				pageup(wp);
				break;
			case WA_DNPAGE:		/* Seite abw„rts */
				pagedown(wp);
				break;
			case WA_UPLINE:		/* Zeile aufw„rts */
				lineup(wp);
				break;
			case WA_DNLINE:		/* Zeile abw„rts */
				linedown(wp);
				break;
		}
	}
}

void doVslid(word whandle,word position)
{
	WindInfo *wp;
	word lost_lines,total_lines,seen_lines;
	word old_skip,new_yskip;

	if(((wp = getwp(whandle)) != NULL)
		&&(wp->fileanz)
		&&(position != wp->vslpos))
	{
		seen_lines = wp->workh / (wp->obh + 1);
		total_lines = wp->fileanz / wp->xanz;
		if(wp->fileanz % wp->xanz)
			total_lines++;
		lost_lines = total_lines - seen_lines;
		if(lost_lines > 0)
		{
			new_yskip = (word)((long)position * lost_lines / 1000);
			if((position * lost_lines) % 1000 > 499)
				new_yskip++;
				
			if(new_yskip != wp->yskip)
			{
				old_skip = wp->yskip;
				wp->yskip = new_yskip;
				scrollit(wp, old_skip - new_yskip, TRUE);
			}
		}
	}
}

word calcYSkip(WindInfo *wp,word toskip)
{
	word xanz,yanz,fullines;
	word skip,leftfiles,gesamtanz;

	xanz = wp->workw / (wp->obw + wp->xdist);
	if(((wp->workw % (wp->obw + wp->xdist)) > (wp->obw * 2/3))
		||((xanz == 0)&&(wp->fileanz != 0)))
		xanz++;
			
	fullines = yanz = wp->workh / (wp->obh+1);
	if(wp->workh % (wp->obh+1))
		yanz++;

	skip = xanz * toskip;

	gesamtanz = xanz * yanz;
	leftfiles = wp->fileanz - skip;

	if(gesamtanz > leftfiles)
	{
		if(toskip)
		{
			word usedlines,freelines;
		
			usedlines = leftfiles / xanz;
			if((leftfiles > 0) && (leftfiles % xanz))
				usedlines++;
					
			freelines = fullines - usedlines;
				
			if(freelines > 0)/* lines in window left blank */
			{
				toskip -= freelines;
				if(toskip < 0)
					toskip = 0;
			}
		}
	}
	return toskip;
}

void charScroll(char c,word kstate)
{
	WindInfo *wp;
	FileBucket *bucket;
	word topwind, currline;
	word forfolder, nr, i, found;
	char *cp;
	
	if(wind_get(0, WF_TOP, &topwind)		/* is it our window? */
		&& (topwind)					/* not window 0 */
		&& ((wp = getwp(topwind)) != NULL)
		&& (show.sortentry == SORTNAME	/* can we do it? */
			|| show.sortentry == SORTTYPE))
	{
		if(isalpha(c))
			c = toupper(c);
		forfolder = ((kstate & (K_LSHIFT|K_RSHIFT)) != 0);
		
		bucket = wp->files;
		currline = nr = 0;
		found = FALSE;
		while ((bucket != NULL) && !found)	
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				if (((bucket->finfo[i].attrib & FA_FOLDER) 
						&& forfolder)
					|| (!(bucket->finfo[i].attrib & FA_FOLDER) 
						&& !forfolder))
				{
					if (show.sortentry == SORTNAME)
						cp = bucket->finfo[i].name;
					else
						cp = bucket->finfo[i].ext;
	
					if ((*cp != '.') && (c <= *cp))
					{
						found = TRUE;
						break;
					}
				}
				else if (forfolder)
				{
					found = TRUE;
					break;		/* no more folders to come */
				}
	
				nr++;
				if (nr % wp->xanz == 0)
					currline++;
			}
				
			bucket = bucket->nextbucket;
		}

		currline = calcYSkip(wp,currline);
	
		if(currline != wp->yskip)
		{
			nr = wp->yskip - currline;
			wp->yskip = currline;
			scrollit(wp, nr, TRUE);
		}
		charSelect(wp,c,forfolder);
	}
}