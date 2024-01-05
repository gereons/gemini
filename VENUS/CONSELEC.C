/*
 * @(#) Gemini\conselec.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: selection in console window
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vdi.h>
#include <aes.h>
#include <tos.h>
#include <flydial\flydial.h>
#include <flydial\clip.h>
#include <nls\nls.h>

#include "vs.h"
#include "stand.h"
#include "conselec.h"
#include "termio.h"
#include "wintool.h"
#include "draglogi.h"
#include "conselec.rh"
#include "venuserr.h"
#include "util.h"
#include "fileutil.h"
#include "redraw.h"

#if STANDALONE
#error warum conselec compilieren in STANDALONE?
#endif

/* externals
 */
extern ShowInfo show;
extern word handle;
extern OBJECT *pconselec;
extern TermConf termconf;

/* internals
 */
static word work[4];
static word maxcol, maxrow, rancx, lancx, ancy;

/* internal texts
 */
#define NlsLocalSection "G.conselec.c"
enum NlsLocalText{
T_NOCLIP,	/*Kann das Verzichnis des Klemmbrettes nicht 
finden! Ein Einfgen ist daher nicht m”glich.*/
T_CREATE,	/*Kann die Datei %s zum Einfgen des Textes in 
das Klemmbrett nicht anlegen!*/
};


static void invertLine(word x1, word x2, word y)
{
	word pxy[4], rect[4];
	word count;
	
	pxy[0] = work[0] + cFontWidth * x1;
	pxy[1] = work[1] + cFontHeight * y;
	pxy[2] = work[0] + (cFontWidth * (x2+1)) - 1;
	pxy[3] = pxy[1] + cFontHeight - 1;
	
    count = 0;
    while (WT_GetRect(count++, rect))
	{
		if (VDIRectIntersect(pxy, rect))
			vr_recfl(handle, rect); 
	}
}

static int isHigher(word x1, word y1, word x2, word y2)
{
	if (y1 == y2)
		return x1 > x2;
	else
		return y1 > y2;
}

static int isLower(word x1, word y1, word x2, word y2)
{
	if (y1 == y2)
		return x1 < x2;
	else
		return y1 < y2;
}

static void dec(word x1, word y1, word *x, word *y)
{
	if (x1 || y1)
	{
		--x1;
		if (x1 < 0)
		{
			x1 = maxcol;
			--y1;
		}
		*x = x1;
		*y = y1;
	}
}

static void inc(word x1, word y1, word *x, word *y)
{
	if ((x1 == maxcol) && (y1 == maxrow))
		return;
	
	++x1;
	if (x1 > maxcol)
	{
		x1 = 0;
		++y1;
	}
	*x = x1;
	*y = y1;
}

static void mouse2Cells(word mx, word my, word *cx, word *cy)
{
	word checkright, checkleft;
	
	checkright = checkleft = FALSE;
	
	if (mx < work[0])
	{
		checkleft = TRUE;
		mx = 0;
	}
	else if (mx >= work[0] + work[2] - 1)
	{
		checkright = TRUE;
		mx = work[2] - 1;
	}
	else
		mx -= work[0];
	
	if (my < work[1])
		my = 0;
	else if (my >= work[1] + work[3] - 1)
		my = work[3] - 1;
	else
		my -= work[1];
	
	*cx = mx / cFontWidth;
	*cy = my / cFontHeight;
	
	if (checkleft)
	{
		if (isHigher(*cx, *cy, rancx, ancy))
			dec(*cx, *cy, cx, cy);
	}
	else if (checkright)
	{
		if (isLower(*cx, *cy, lancx, ancy))
			inc(*cx, *cy, cx, cy);
	}
}

static void invertBetween(word x1, word y1, word x2, word y2)
{
	GrafMouse(M_OFF, NULL);
	
	if (isHigher(x1, y1, x2, y2))
	{
		word tmp;		/* Punkte vertauschen */
		
		tmp = y1; y1 = y2; y2 = tmp;
		tmp = x1; x1 = x2; x2 = tmp;
	}
	
	if (y1 == y2)			/* selbe Zeile */
	{
		invertLine(x1, x2, y1);
	}
	else
	{
		word i;
		
		invertLine(x1, maxcol, y1);	/* erste Zeile */
		for (i = y1+1; i < y2; ++i)
		{
			invertLine(0, maxcol, i);
		}
		invertLine(0, x2, y2);
	}

	GrafMouse(M_ON, NULL);
}

#define GO_RIGHT	0
#define GO_LEFT		1

static word go2Word(word x, word y, word direction)
{
	word lastx;
	char c;

	c = screen[y][x];
	lastx = x;
	while ((x >= 0) && (x < termconf.columns) 
			&& (isalnum(c) || strchr("._-+„”Žš™ž", c)))
	{
		lastx = x;
		(direction == GO_LEFT)? --x : ++x;
		c = screen[y][x];
	}
	
	return lastx;
}

static void setAnchor(word cx, word cy, word doubleClick)
{
	ancy = cy;
	if (doubleClick)
	{
		rancx = go2Word(cx, cy, GO_RIGHT);
		lancx = go2Word(cx, cy, GO_LEFT);
	}
	else
		lancx = rancx = cx;
}

static void cell2Word(word cx, word cy, word *zx, word *zy)
{
	word direction;
	
	direction = isHigher(cx, cy, rancx, ancy)? GO_RIGHT : GO_LEFT;

	*zx = go2Word(cx, cy, direction);
	*zy = cy;
}

static char *getString(word x1, word y1, 
					word x2, word y2, word newline)
{
	size_t len = 1, max;
	char *string;
	
	if (isHigher(x1, y1, x2, y2))
	{
		word tmp;
		
		tmp = y1; y1 = y2; y2 = tmp;
		tmp = x1; x1 = x2; x2 = tmp;
	}
	
	/* get the length of selected string */
	if (y1 == y2)
	{
		max = strlen(screen[y1]);
		if (x1 >= max)
			return NULL;
		if (x2 >= max)
			x2 = (word)max - 1;
		len += x2 - x1 + 1;
		if (newline)
			++len;
	}
	else
	{
		word i;
		
		max = strlen(screen[y1]);
		if (x1 < max)
		{
			len += max - x1;
			if (newline)
				++len;
		}
		else
			x1 = (word)max;
		
		for (i = y1+1; i < y2; ++i)
		{
			len += strlen(screen[i]);
			if (newline)
				++len;
		}
		
		max = strlen(screen[y2]);
		if (x2 && (x2 >= max))
			x2 = (word)max - 1;
		len += x2;
		if (newline)
			++len;
	}
	
	if (len == 1)		/* keine Zeichen selektiert */
		return NULL;
		
	string = malloc(len+1);
	if (!string)
		return NULL;
	
	if (y1 == y2)
	{
		strncpy(string, &screen[y1][x1], x2 - x1 + 1);
		string[x2 - x1 + 1] = '\0';
		if (newline)
			strcat(string, "\n");
	}
	else
	{
		word i;
		
		strcpy(string, "");
		strcat(string, &screen[y1][x1]);
		if (newline)
			strcat(string, "\n");
		
		for (i = y1+1; i < y2; ++i)
		{
			strcat(string, screen[i]);
			if (newline)
				strcat(string, "\n");
		}
		
		strncat(string, screen[y2], x2 + 1);
		if (newline)
			strcat(string, "\n");
	}
	
	return string;
}

static void sendString(word x1, word y1, word x2, word y2)
{
	char *string;
	
	string = getString(x1, y1, x2, y2, FALSE);
	
	if (string)
	{
		PasteString(string);
		
		free(string);
	}
}

static void clipString(word x1, word y1, word x2, word y2)
{
	char *string;
	char clippath[MAXLEN];
	word fhandle;
	
	string = getString(x1, y1, x2, y2, TRUE);
	
	if (!string)
		return;

	if (ClipFindFile("TXT", clippath))
	{
		MFORM form;
		word index;
		
		GrafGetForm(&index, &form);
		GrafMouse(HOURGLASS, NULL);
		
		ClipClear(NULL);

		fhandle = Fcreate(clippath, 0);
		if (fhandle > 0)
		{
			char *line = string;
			char *cp;
			
			while((cp = strchr(line, '\n')) != NULL)
			{
				char *tmp = cp + 1;
				
				do
				{
					*cp-- = '\0';
				}
				while (strlen(line) && (*cp == ' '));
				
				Fputs(fhandle, line);
				line = tmp;
			}
			Fclose(fhandle);
		}
		else
			venusErr(NlsStr(T_CREATE), clippath);
			
		stripFileName(clippath);
		pathUpdate(clippath, "");
		
		GrafMouse(index, &form);
	}
	else
		venusErr(NlsStr(T_NOCLIP));
		
	free(string);
}

static void doPopResult(word index,
				word fx, word fy, word tx, word ty)
{
	switch(index)
	{
		case CONSCOPY:
			clipString(fx, fy, tx, ty);
			break;
		case CONSPAST:
			sendString(fx, fy, tx, ty);
			break;
		default:
			break;	
	}
}

void ConSelection(WindInfo *mwp, word mx, word my, 
					word kstate, word doubleClick)
{
	word bstate;
	word prevx, prevy;
	word curx, cury;
	word tmpx, tmpy;
	word anchorx;
	OBJECT *poptree;
	word popindex;
	
	if (!mwp)
		return;
	
	work[0] = mwp->workx;
	work[1] = mwp->worky;
	work[2] = mwp->workw;
	work[3] = mwp->workh;
	if (!pointInRect(mx, my, work))
		return;

	maxcol = termconf.columns - 1;
	maxrow = termconf.rows - 1;
	
	/* VDI-Attribte setzen */
	vswr_mode(handle, MD_XOR);
	vsf_color(handle, 1);
	vsf_interior(handle, FIS_SOLID);
	{
		word pxy[4];
		
		vs_clip(handle, 0, pxy);
	}
	
	mouse2Cells(mx, my, &prevx, &prevy);
	setAnchor(prevx, prevy, doubleClick);
	prevx = lancx;
	
	WT_BuildRectList(mwp->handle);
	GrafMouse(M_OFF, NULL);
	rem_cur();
	GrafMouse(M_ON, NULL);
	
	invertBetween(lancx, ancy, rancx, ancy);
	
	mouse2Cells(mx, my, &curx, &cury);
	
	while (ButtonPressed(&mx, &my, &bstate, &kstate))
	{
		if (doubleClick)
			cell2Word(curx, cury, &curx, &cury);
		 
		if ((curx != prevx) || (cury != prevy))
		{
			if (isHigher(prevx, prevy, rancx, ancy))
			{
				if (isHigher(curx, cury, rancx, ancy))
				{
					if (isHigher(curx, cury, prevx, prevy))
					{
						inc(prevx, prevy, &tmpx, &tmpy);
						invertBetween(tmpx, tmpy, curx, cury);
					}
					else
					{
						inc(curx, cury, &tmpx, &tmpy);
						invertBetween(tmpx, tmpy, prevx, prevy);
					}
				}
				else
				{
					inc(rancx, ancy, &tmpx, &tmpy);
					invertBetween(tmpx, tmpy, prevx, prevy);
					if (isLower(curx, cury, lancx, ancy))
					{
						dec(lancx, ancy, &tmpx, &tmpy);
						invertBetween(curx, cury, tmpx, tmpy);
					}
				}
			}
			else if (isLower(prevx, prevy, lancx, ancy))
			{
				if (isLower(curx, cury, lancx, ancy))
				{
					if (isLower(curx, cury, prevx, prevy))
					{
						dec(prevx, prevy, &tmpx, &tmpy);
						invertBetween(tmpx, tmpy, curx, cury);
					}
					else
					{
						dec(curx, cury, &tmpx, &tmpy);
						invertBetween(prevx, prevy, tmpx, tmpy);
					}
				}
				else
				{
					dec(lancx, ancy, &tmpx, &tmpy);
					invertBetween(tmpx, tmpy, prevx, prevy);
					if (isHigher(curx, cury, rancx, ancy))
					{
						inc(rancx, ancy, &tmpx, &tmpy);
						invertBetween(curx, cury, tmpx, tmpy);
					}
				}
			}
			else	/* prev und anc sind gleich */
			{
				if (isHigher(curx, cury, prevx, prevy))
				{
					inc(rancx, ancy, &tmpx, &tmpy);
					invertBetween(tmpx, tmpy, curx, cury);
				}
				else
				{
					dec(lancx, ancy, &tmpx, &tmpy);
					invertBetween(tmpx, tmpy, curx, cury);
				}
			}
			
			prevx = curx;
			prevy = cury;
		}
		mouse2Cells(mx, my, &curx, &cury);
	}
	
	JazzUp(pconselec, -1, -1, 1, CONSPAST, 0, &poptree, &popindex);

	if (isHigher(prevx, prevy, rancx, ancy))
		anchorx = lancx;
	else
		anchorx = rancx;
	invertBetween(anchorx, ancy, prevx, prevy);
	
	GrafMouse(M_OFF, NULL);
	disp_cur();
	GrafMouse(M_ON, NULL);
	if (poptree)
	{
		setSelected(poptree, popindex, FALSE);
		doPopResult(popindex, anchorx, ancy, prevx, prevy);
	}
}
