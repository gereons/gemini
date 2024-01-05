/*
 * @(#) Gemini\Wildcard.c
 * @(#) Stefan Eissing, 17. April 1991
 *
 * description: Modul fÅr Wildcard-Extensions
 *
 */
 
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\listman.h>
#include <nls\nls.h>

#include "vs.h"
#include "wildbox.rh"
#include "wedit.rh"

#include "scancode.h"
#include "wildcard.h"
#include "util.h"
#include "redraw.h"
#include "venuserr.h"

store_sccs_id(wcard);


/* externs
 */
extern OBJECT *pwildbox, *pweditbox;
extern WindInfo wnull;
extern char wildpattern[5][WILDLEN];

/* internal texts
 */
#define NlsLocalSection		"Gmni.wildcard"
enum NlsLocalText{
T_EMPTY,		/*Ein leerer Wildcard ist nicht erlaubt!*/
};


static void changeWildWindow(const char *newpat, const char *oldpat)
{
	WindInfo *wp = wnull.nextwind;
	
	while (wp != NULL)
	{
		if (!strcmp(wp->wildcard,oldpat))
		{
			strcpy(wp->wildcard,newpat);
			pathchanged(wp);
		}
		wp = wp->nextwind;
	}
}

/*
 * editiere den Wildcard pattern, und gib zurÅck, ob er
 * verÑndert wurde.
 */
static word editWildcard(char *pattern)
{
	DIALINFO d;
	word retcode, ok;
	char mypat[WILDLEN+1];
	int edit_object = WEDITTXT;
	
	strcpy(mypat,pattern);
	pweditbox[WEDITTXT].ob_spec.tedinfo->te_ptext = mypat;
	
	DialCenter(pweditbox);
	DialStart(pweditbox,&d);
	DialDraw(&d);

	do
	{
		retcode = DialDo(&d, &edit_object) & 0x7fff;
		setSelected(pweditbox, retcode, FALSE);
		fulldraw(pweditbox, retcode);
		
		if (retcode == WEDITOK)
		{
			ok = (strlen(mypat)>0);
			if (!ok)
				venusErr(NlsStr (T_EMPTY));
		}
		
	} while(!ok);
	
	DialEnd(&d);
	
	if (retcode == WEDITOK)
	{
		if (strcmp(pattern, mypat))
		{
			strncpy(pattern, mypat, WILDLEN-1);
			pattern[WILDLEN-1] = '\0';
			strupr(pattern);
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * Zeichne einen Wildcard fÅr Listman
 */
static void drawWildcard(LISTSPEC *l, word x, word y, word offset,
				GRECT *clip, word how)
{
	char *str;
	word dummy, xy[8];
	
	if (!l)
		return;
	
	str = l->entry;
	
	RectGRECT2VDI(clip, xy);
	vs_clip(DialWk, 1, xy);

	if (how & LISTDRAWREDRAW)
	{
		vswr_mode(DialWk, MD_REPLACE);
		vst_alignment(DialWk, 0, 5, &dummy, &dummy);
		vst_effects(DialWk, 0);
		v_gtext(DialWk, x-offset+HandXSize, y, str);
	}

	if ((l->flags.selected) || !(how & LISTDRAWREDRAW))	
	{
		MFDB screen;
	
		RectGRECT2VDI(clip, xy);
		vs_clip(DialWk, 1, xy);
	
		screen.fd_addr = 0L;
		memcpy (&xy[4], xy, 4*sizeof(int));	
		vro_cpyfm (DialWk, D_INVERT, xy, &screen, &screen); 
	}
	
	vs_clip (DialWk, 0, xy);
}

static LISTINFO *pL;
static LISTSPEC list[6];
static word wnr;

static int _UpDown (OBJECT *tree, int object, int obnext, int ochar, 
					int kstate, int *nxtobject, int *nxtchar)
{
	if (ochar == CUR_UP)
	{
		/* einen nach oben */
		
		if (wnr > 0) /* war nicht schon am Anfang */
		{
			list[wnr].flags.selected = 0;
			ListInvertEntry(pL, wnr);
			--wnr;
			list[wnr].flags.selected = 1;
			ListInvertEntry(pL, wnr);

			if (wnr == 4)
			{
				setDisabled(pwildbox, WILDEDIT, FALSE);
				fulldraw(pwildbox, WILDEDIT);
			}
			*nxtchar = 0;
			return 1;
		}
	}

	if (ochar == CUR_DOWN)
	{
		/* schon am Ende? */

		if (wnr < 5)
		{
			list[wnr].flags.selected = 0;
			ListInvertEntry(pL, wnr);
			++wnr;
			list[wnr].flags.selected = 1;
			ListInvertEntry(pL, wnr);

			if (wnr == 5)
			{
				setDisabled(pwildbox, WILDEDIT, TRUE);
				fulldraw(pwildbox, WILDEDIT);
			}
			*nxtchar = 0;
			return 1;
		}
	}
	
	return FormKeybd (tree, object, obnext, 
						ochar, kstate, nxtobject, nxtchar);
}

/* neuen Wildcard fÅr Window eingeben
 */
void doWildcard(WindInfo *wp)
{
	DIALINFO d;
	LISTINFO L;
	word retcode, i, exit, draw, clicks;
	long listresult;
	char mypattern[6][WILDLEN];
	
	memcpy(mypattern,wildpattern,sizeof(wildpattern));
	strcpy(mypattern[5], "*");

	pL = &L;
	wnr = -1;
	for (i = 0; i <= 5; ++i)
	{
		list[i].entry = mypattern[i];
		if (i)
			list[i-1].next = &list[i];
		
		if (strcmp(mypattern[i], wp->wildcard))
			list[i].flags.selected = 0;
		else
		{
			wnr = i;
			list[i].flags.selected = 1;
		}
	}
	
	list[5].next = NULL;

	if (wnr < 0)
	{
		strcpy(wp->wildcard, "*");
		wnr = 5;
		list[5].flags.selected = 1;
	}

	setDisabled(pwildbox, WILDEDIT, wnr >= 5);
	
	DialCenter(pwildbox);
	
	ListStdInit(&L, pwildbox, WILDB, WILDBG, drawWildcard, 
				list, 0, wnr, 1);
	ListInit(&L);

	DialStart(pwildbox,&d);
	DialDraw(&d);
	ListDraw(&L);

	do
	{
		draw = exit = FALSE;
		
		FormSetFormKeybd (_UpDown);
		retcode = DialDo(&d,0);
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;
		
		switch(retcode)
		{
			case WILDB:
			case WILDBG:
				listresult = ListClick(&L, clicks);
				if ((listresult >= 0) && (wnr != listresult))
				{
					if ((listresult >=  5)||(wnr >= 5))
					{
						setDisabled(pwildbox, WILDEDIT,
									listresult >= 5);
						fulldraw(pwildbox, WILDEDIT);
					}
					wnr = (word)listresult;
				}
				if (clicks == 2)
				{
					retcode = WILDEDIT;
				}
				break;
			case WILDEDIT:
				draw = TRUE;
				break;
			default:
				draw = exit = TRUE;
				break;
		}
		
		if (retcode == WILDEDIT)
			if ((wnr < 5) && (editWildcard(mypattern[wnr])))
			{
				ListExit(&L);
				ListInit(&L);
				ListScroll2Selection(&L);
				ListDraw(&L);
			}

		if (draw)
		{
			setSelected(pwildbox,retcode,FALSE);
			fulldraw(pwildbox,retcode);
		}
	} while(!exit);

	ListExit(&L);
	DialEnd(&d);
	
	
	if((retcode == WILDQUIT))
	{
		if (strcmp(mypattern[wnr], wp->wildcard))
		{
			char *ct;
			
			strncpy(wp->wildcard, mypattern[wnr], WILDLEN-1);
			wp->wildcard[WILDLEN-1] = '\0';

			ct = strchr(wp->wildcard, '\\');
			if(ct)
				*ct = '\0';			/* kein Backslash im Wildcard */
				
			pathchanged(wp);
		}
		
		for (i = 0; i < 5; i++)
		{
			if (strcmp(mypattern[i],wildpattern[i]))
			{
				changeWildWindow(mypattern[i],wildpattern[i]);
				strcpy(wildpattern[i],mypattern[i]);
			}
		}
	}
}


int matchpattern(char *text,char *pattern);

static int star(char *s, char *p)
{
	while (!matchpattern(s,p))
		if (*++s == '\0')
			return FALSE;
	return TRUE;
}

static int matchpattern(char *s, char *p)
{
	int 	 last;
	int 	 matched;
	int 	 reverse;

	for ( ; *p; s++, p++)
	switch (*p)
	{
		case '@':
			/* Literal match following character; fall through. */
			p++;
		default:
			if (*s != *p)
		    		return FALSE;
			continue;
		case '?':
			/* Match anything. */
			if (*s == '\0')
				return FALSE;
			continue;
		case '*':
			/* Trailing star matches everything. */
			if (*s == '\0')
			{
				if (*(p+1) == '\0')
					return TRUE;
				else
					return FALSE;
			}
			else
				return(*++p ? star(s, p) : TRUE);
		case '[':
			/* [^....] means inverse character class. */
			if ((reverse = p[1] == '^') == TRUE)
				p++;
			for (last = 0400, matched = FALSE; *++p && *p != ']'; last = *p)
				/* This next line requires a good C compiler. */
				if (*p == '-' ? *s <= *++p && *s >= last : *s == *p)
					matched = TRUE;
			if (matched == reverse)
				return FALSE;
			continue;
	}
	return *s == '\0';
}

/*
 * int wildmatch(char *text, char *pattern)
 */
word filterFile(const char *wcard,const char *fname)
{
	char text[20],pattern[WILDLEN];
	char *cp;
	
	strcpy(text,fname);
	strcpy(pattern,wcard);
	strupr(text);
	strupr(pattern);
	cp = strtok(pattern, ",;");
	while (cp)
	{
		if (matchpattern(text,cp))
			return TRUE;
		cp = strtok(NULL, ",;");
	}
	return FALSE;
}
