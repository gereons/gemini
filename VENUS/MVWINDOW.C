/*
 * @(#) Gemini\mvwindow.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: top layer of functions for mupfel window
 */

#include <stdlib.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\fontsel.h>
#include <nls\nls.h>

#include "vs.h"
#include "fontbox.rh"

#include "window.h"
#include "venuserr.h"
#include "util.h"
#include "..\mupfel\environ.h"
#include "..\mupfel\gemsubs.h"

#include "mvwindow.h"
#include "terminal.h"
#include "termio.h"
#include "wintool.h"
#include "filedraw.h"
#include "redraw.h"

store_sccs_id(mvwindow);

/*
 * externals
 */
extern word deskx,desky,deskw,deskh,win_handle;
extern word handle;
extern ShowInfo show;
extern TermConf termconf;
extern GRECT work;
extern OBJECT *pfontbox;
extern word back_destructive;

/* internal texts
 */
#define NlsLocalSection "G.mvwindow"
enum NlsLocalText{
T_COLUMNS,		/*Die Anzahl der Spalten muž zwischen 10 und %d liegen!*/
T_ROWS,			/*Die Anzahl der Zeilen muž zwischen 3 und %d liegen!*/
T_TEST,			/*Mupfel forever!*/
T_WRONGFONT,	/*Der gewnschte Font fr das Consolefenster konnte 
nicht eingestellt werden! Ist vielleicht kein GDOS vorhanden?*/
};

/*
 * internals
 */

/* FONTWORK fr die beiden Workstation des Console-Fensters
 */
FONTWORK stdWork, textWork;

void GetConsoleFont(word *id, word *points)
{
	*id = cFontId;
	*points = cFontPoints;
}

void setInMWindow(ShowInfo *ps)
{
	termconf.columns = ps->m_cols;
	termconf.rows = ps->m_rows;
	termconf.inv_type = ps->m_inv;
	cFontId = ps->m_font;
	cFontPoints = ps->m_fsize;

	if (!SetFont(&stdWork, &cFontId,
		 &cFontPoints, &cFontWidth, &cFontHeight))
	{
		venusErr(NlsStr(T_WRONGFONT));
	}
	SetFont(&textWork, &cFontId,
		 &cFontPoints, &cFontWidth, &cFontHeight);

	work.g_x = deskx + (word)scale123(deskw,ps->m_wx,1000L);
	work.g_y = desky + (word)scale123(deskh,ps->m_wy,1000L);
	work.g_w = cFontWidth * termconf.columns;
	work.g_h = cFontHeight * termconf.rows;
}

void getInMWindow(ShowInfo *ps)
{
	ps->m_cols = termconf.columns;
	ps->m_rows = termconf.rows;
	ps->m_inv = termconf.inv_type;
	ps->m_font = cFontId;
	ps->m_fsize = cFontPoints;
	ps->m_wx = (word)scale123(1000L,work.g_x - deskx,deskw);
	ps->m_wy = (word)scale123(1000L,work.g_y - desky,deskh);
}

word MWindInit(void)
{
	if (init_terminal(&stdWork.sysfonts))
	{
		cFontId = 1;
		cFontPoints = 8;
		stdWork.handle = std_handle;
		textWork.handle = text_handle;
		stdWork.loaded = textWork.loaded = FALSE;
		textWork.sysfonts = stdWork.sysfonts;
		stdWork.addfonts = textWork.addfonts = 0;
		stdWork.list = textWork.list = NULL;

		FontLoad(&stdWork);
		FontLoad(&textWork);
		SetFont(&stdWork, &cFontId, &cFontPoints,
			&cFontWidth, &cFontHeight);
		SetFont(&textWork, &cFontId, &cFontPoints,
			&cFontWidth, &cFontHeight);

		win_handle = 0;
		work.g_x = work.g_y = work.g_w = work.g_h = 0;
		termconf.columns = 80;
		termconf.rows = 25;
		termconf.inv_type = 0;
		termconf.wrap = TRUE;
		termconf.tab_size = 8;
		termconf.tab_destructive = FALSE;
		termconf.back_destructive = FALSE;

		return TRUE;
	}
	return FALSE;
}

void MWindExit(void)
{
	FontUnLoad(&stdWork);
	FontUnLoad(&textWork);

	exit_terminal();
}

/*
 * Set Mupfel's idea of how large the window is
 */
static void SetMWSize(void)
{
	char tmp[50];
	void setLAxy(int x, int y);
	
	sprintf(tmp,"COLUMNS=%d",termconf.columns);
	putenv(tmp);
	sprintf(tmp,"ROWS=%d",termconf.rows);
	putenv(tmp);
	_maxcol = termconf.columns - 1;
	_maxrow = termconf.rows - 1;
	
	if(getMupfWp() != NULL)
	{
		setLAxy(_maxcol, _maxrow);
	}
}

/* 
 * Set parameters for the low level i/o routines
 * Build a rectangle list for the console window
 */
word openMWindow(WindInfo *wp)
{
	win_handle = wp->handle;
	WT_BuildRectList(win_handle);
	work.g_x = wp->workx;
	work.g_y = wp->worky;
	work.g_w = wp->workw;
	work.g_h = wp->workh;
	wp->oldw = wp->oldh = 0;
	SetMWSize();
	return TRUE;
}

/* 
 * Change the console window's size
 */
word resizeMWindow(WindInfo *wp)
{
	SetMWSize();
	work.g_x = wp->workx;
	work.g_y = wp->worky;
	work.g_w = wp->workw;
	work.g_h = wp->workh;
	WT_BuildRectList(win_handle);
	return TRUE;
}

word moveMWindow(WindInfo *wp)
{
	work.g_x = wp->workx;
	work.g_y = wp->worky;
	work.g_w = wp->workw;
	work.g_h = wp->workh;
	WT_BuildRectList(win_handle);
	
	return TRUE;
}

word closeMWindow(WindInfo *wp)
{
	(void)wp;
	return TRUE;
}

void drawMWindow(WindInfo *wp, word r1[4])
{
	redrawMWindow(wp, r1);
	wp->update |= KILLREDRAW;
}

word redrawMWindow(WindInfo *wp,word r1[4])
{
	word req[4],draw[4],count;
	
	req[0] = r1[0];
	req[1] = r1[1];
	req[2] = r1[0] + r1[2] - 1;
	req[3] = r1[1] + r1[3] - 1;

	GrafMouse(M_OFF,NULL);	
	WT_BuildRectList(wp->handle);
	count = 0;
	while (WT_GetRect(count++,draw))
	{
		if (VDIRectIntersect(req,draw))
			TM_RedrawTerminal(draw);
	}
	GrafMouse(M_ON,NULL);
	return TRUE;
}

void SizeOfMWindow(word *wx,word *wy,word *ww,word *wh)
{
	word workx,worky,workw,workh;
	
	if (work.g_w && work.g_h)
	{
		workx = work.g_x;
		worky = work.g_y;
	}
	else
	{
		workx = show.m_wx;
		worky = show.m_wy;
	}
	work.g_w = workw = cFontWidth * termconf.columns;
	work.g_h = workh = cFontHeight * termconf.rows;
	wind_calc(WC_BORDER,MUPFWIND,workx,worky,workw,workh,wx,wy,ww,wh);
}

static void fontRedrawWindow(word redraw)
{
	WindInfo *wp;
	word draw[4];
	
	if((wp = getMupfWp()) != NULL)
	{
		wp->workw = cFontWidth * termconf.columns;
		wp->workh = cFontHeight * termconf.rows;
		wind_calc(WC_BORDER,MUPFWIND,
			wp->workx,wp->worky,wp->workw,wp->workh,
			&wp->windx,&wp->windy,&wp->windw,&wp->windh);
		work.g_x = draw[0] = wp->workx;
		work.g_y = draw[1] = wp->worky;
		work.g_w = draw[2] = wp->workw;
		work.g_h = draw[3] = wp->workh;
		if (redraw)
			redrawMWindow(wp, draw);
	}
}

void FullMWindow(WindInfo *wp)
{
	word redraw;
	
	if (wp->oldw)
	{
		redraw = (wp->workx == wp->oldx);
		termconf.columns = wp->oldw;
		termconf.rows = wp->oldh;
		wp->workx = wp->oldx;
		wp->worky = wp->oldy;
		wp->oldx = wp->oldy = wp->oldw = 0;
	}
	else
	{
		wp->oldx = wp->workx;
		wp->oldy = wp->worky;
		wp->oldw = termconf.columns;
		wp->oldh = termconf.rows;
		wp->windx = deskx;
		if (show.aligned)
			wp->windx = charAlign(wp->windx);
		wp->windy = desky;
		wp->windw = deskw - wp->windx;
		wp->windh = deskh;
			
		wind_calc(WC_WORK, MUPFWIND,
			wp->windx, wp->windy, wp->windw, wp->windh,
			&wp->workx, &wp->worky, &wp->workw, &wp->workh);
		termconf.columns = wp->workw / cFontWidth;
		if (termconf.columns > max_columns)
			termconf.columns = max_columns;
		termconf.rows = wp->workh / cFontHeight;
		if (termconf.rows > max_lines)
			termconf.rows = max_lines;
		
		wp->workw = termconf.columns * cFontWidth;
		wp->workh = termconf.rows * cFontHeight;
		redraw = FALSE;
		wind_calc(WC_BORDER, MUPFWIND,
			wp->workx, wp->worky, wp->workw, wp->workh,
			&wp->windx, &wp->windy, &wp->windw, &wp->windh);
		wp->windx = deskx + ((deskw - wp->windw) / 2);
		wp->windy = desky + ((deskh - wp->windh) / 2);
		if (show.aligned)
			wp->windx = charAlign(wp->windx);
		wind_calc(WC_WORK, MUPFWIND,
			wp->windx, wp->windy, wp->windw, wp->windh,
			&wp->workx, &wp->worky, &wp->workw, &wp->workh);
	}
	
	reinit_terminal();
	SizeScreenBuffer();
	SetMWSize();
	fontRedrawWindow(redraw);
	wind_set(wp->handle, WF_CURRXYWH, wp->windx, wp->windy,
			wp->windw, wp->windh);
	wind_get(wp->handle, WF_WORKXYWH, &wp->workx, &wp->worky,
			&wp->workw, &wp->workh);
	moveMWindow(wp);
}

word fontDialog(void)
{
	DIALINFO d;
	FONTSELINFO F;
	char *pftrows,*pftcols;
	word retcode,selinv;
	word id, x, y, size, clicks;
	word draw, item;

	pftrows = pfontbox[FTROWS].ob_spec.tedinfo->te_ptext;
	pftcols = pfontbox[FTCOLS].ob_spec.tedinfo->te_ptext;
	sprintf(pftrows,"%2d",termconf.rows);
	sprintf(pftcols,"%3d",termconf.columns);

	setSelected(pfontbox,FTINV,termconf.inv_type == INV_INV);
	setSelected(pfontbox,FTUNDER,termconf.inv_type == INV_UDL);
	setSelected(pfontbox,FTBOLD,termconf.inv_type == INV_BLD);

	DialCenter(pfontbox);

	id = cFontId;
	size = cFontPoints;
	
	GrafMouse(HOURGLASS, NULL);
	FontGetList(&stdWork, TRUE, FALSE);
	FontSelInit(&F, &stdWork, pfontbox, FTBOX, FTBGBOX, FTPBOX,
		FTPBGBOX, FTFONT, (char *)NlsStr(T_TEST), 0, &id, &size);
	GrafMouse(ARROW, NULL);
	DialStart(pfontbox,&d);
	DialDraw(&d);
	FontSelDraw(&F, id, size);
	
	do
	{
		draw = TRUE;
		item = -1;
		retcode = DialDo(&d, 0);
		
		clicks = (retcode&0x8000)? 2 : 1;
		retcode &= 0x7FFF;
		
		if (retcode == FTBOX || retcode == FTBGBOX)
			item = FontClFont (&F, clicks, &id, &size);
		else if (retcode == FTPBOX || retcode == FTPBGBOX)
			item = FontClSize (&F, clicks, &id, &size);

		if ((item >= 0) && (clicks == 2))
		{
			retcode = FTOK;
			draw = FALSE;
		}
		
		if (retcode == FTOK)
		{
			y = atoi(pftrows);
			x = atoi(pftcols);
			if (y < 3 || y > max_lines)
			{
				venusErr(NlsStr(T_ROWS), max_lines);
				setSelected(pfontbox, retcode, FALSE);
				fulldraw(pfontbox, retcode);
				retcode = 0;
			}
			else if (x < 10 || x > max_columns)
			{
				venusErr(NlsStr(T_COLUMNS), max_columns);
				setSelected(pfontbox, retcode, FALSE);
				fulldraw(pfontbox, retcode);
				retcode = 0;
			}
		}
	} while ((retcode != FTCANCEL) && (retcode != FTOK));

	if (draw)
	{
		setSelected(pfontbox, retcode, FALSE);
		fulldraw(pfontbox, retcode);
	}
	
	if (retcode == FTOK)
	{
		if (isSelected(pfontbox, FTINV))
			selinv = INV_INV;
		else if (isSelected(pfontbox, FTUNDER))
			selinv = INV_UDL;
		else
			selinv = INV_BLD;
		
		if ((id != cFontId) || (size != cFontPoints)
			|| (termconf.rows != y) 
			|| (termconf.columns != x)
			|| (selinv != termconf.inv_type))
		{
			termconf.inv_type = selinv;
			cFontId = id;
			cFontPoints = size;
			
			SetFont(&stdWork, &cFontId, &cFontPoints,
				&cFontWidth, &cFontHeight);
			SetFont(&textWork, &cFontId, &cFontPoints,
				&cFontWidth, &cFontHeight);

			if (x!=termconf.columns || y!=termconf.rows)
			{
				termconf.columns = x;
				termconf.rows = y;
				reinit_terminal();
				SizeScreenBuffer();
			}

			SetMWSize();

			FontSelExit(&F);
			DialEnd(&d);
			fontRedrawWindow(TRUE);
			return TRUE;
		}
	}
	
	SetFont(&stdWork, &cFontId, &cFontPoints,
		&cFontWidth, &cFontHeight);
	SetFont(&textWork, &cFontId, &cFontPoints,
		&cFontWidth, &cFontHeight);

	FontSelExit(&F);
	DialEnd(&d);
	return FALSE;
}

void setBackDestr(int flag)
{
	termconf.back_destructive = flag;	
}