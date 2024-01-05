/*
 * lineedit.c  -  console line editor
 * changed by Julian F. Reschke for key events (22.02.90)
 * 07.02.91
 *
 * 19.01.91 - changes for fcomplete.c (jr)
 * 21.01.91 - changes for loctime (jr)
 */

#include <ctype.h> 
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "chario.h"
#include "curdir.h"
#include "environ.h"
#include "fkey.h"
#include "gemsubs.h"
#include "history.h"
#include "keys.h"
#include "lineedit.h"
#include "loctime.h"
#include "mupfel.h"
#include "scan.h"
#include "shellvar.h"
#include "stand.h"
#include "sysvec.h"
#include "vt52.h"

#include "fcomplet.h"

SCCS(lineedit);

char lbuf[LINSIZ];				/* Keeps current line */
static char lbuf2[LINSIZ];		/* Temp for scrolling */

static int firstscroll;
static int maxlp;				/* # of chars in lbuf */
static int lp;					/* Pointer into lbuf */
static int insflag;				/* insert mode on/off */
static char **hbuf;				/* History Buffer */
static int maxhist;				/* Max. History Entries */
static int maxhp;				/* Max. History Pointer */
static int hp;					/* Current History Entry */
static int fkeycr;
static int cmdlinemode;			/* TRUE for normal command entry,
                                      FALSE eg for "more" */
static int histchanged = FALSE;	/* to avoid writing unchanged history */
static void showprompt(void);

/* Input line edit functions */

static int outstr(char *s)
{
	int len = (int)strlen(s);
#if MERGED
	if (conwindow)
		disp_string(s);
	else
#endif
		while (*s)
			rawout(*s++);
	return len;
}

static void backspace(uint count, int destructive)
{
	int xcur, ycur;

	while (count--)
	{
		getcursor(&xcur,&ycur);
		if (xcur==0 && ycur>0)
		{
			vt52cm(_maxcol,ycur-1);
			if (destructive)
			{
				rawout(' ');
				vt52cm(_maxcol,ycur-1);
			}
		}	
		else
			canout(BS);
	}
}

static void eraseline(void)
{
#if MERGED
	if (conwindow)
	{
		if (lp<maxlp)
			outstr(&lbuf[lp]);
		setBackDestr(TRUE);
		backspace(maxlp,TRUE);
		setBackDestr(FALSE);
	}
	else
#endif
	{
		int xcur, ycur;
			
		cursoroff();
		backspace(lp,FALSE);
		getcursor(&xcur,&ycur);
		rawoutn(' ',maxlp);
		vt52cm(xcur,ycur);
		cursoron();
	}
}

static void histscroll(int up,int match)
{
	static char tmp[LINSIZ];
	
	if (maxhp==-1)				/* nothing in history */
	{
		beep();
		return;
	}
	if (firstscroll)			/* save current line */
	{
		strcpy(tmp,lbuf);
		firstscroll=FALSE;
	}
	if (up && hp<maxhp)			/* scroll up and room? */
		++hp;
	else
	{
		if (!up && hp>0)		/* scroll down and room? */
			--hp;
		else
		{
			if (!up && hp==0)	/* down but last? */
			{
				eraseline();
				strcpy(lbuf,tmp);
				hp = -1;
				if (!match)
					firstscroll=TRUE;
				lp = maxlp = outstr(lbuf);
				return;
			}
			else
			{
				beep();
				return;
			}
		}
	}
	strcpy(lbuf2,hbuf[hp]);
}

static void printscroll(void)
{
	if (hp>=0)
	{
		eraseline();
		strcpy(lbuf,lbuf2);
		lp = maxlp = outstr(lbuf);
	}
}

static void matchscroll(int direction)
{
	static int savelen;
	static int saved = FALSE;
	static int matchhp = -1;
	static char saveline[LINSIZ];
	int match;
	
	if (firstscroll)
		saved = FALSE;

	if (!saved && maxlp>0)
	{
		strcpy(saveline,lbuf);
		savelen = maxlp;
		matchhp = -1;
		saved = TRUE;
	}
	if (saved)
	{
		int savehp = hp;
		do
		{
			histscroll(direction,TRUE);
			match = !strncmp(lbuf2,saveline,savelen);
			if (match)
				matchhp = hp;
		} while (!match && hp<maxhp && hp>=0);

		if (!match)
		{
			if (matchhp<maxhp && matchhp>=0)
				hp = matchhp;
			else
				hp = savehp;
			if (hp>=0 && hp<maxhp)
				strcpy(lbuf2,hbuf[hp]);
			else
				*lbuf2 = '\0';
			beep();
		}
		else
			hp = matchhp;
	}
	else
		histscroll(direction,FALSE);
	printscroll();
}

static void showhist(void)
{
	int i;
	
	mprintf("\n" LE_CMDHIST "\n");
	for (i=maxhp; i>=0; --i)
	{
		mprintf("%2d: ",i);
		outstr(hbuf[i]);
		crlf();
	}
	showprompt();
	outstr(lbuf);
	lp=maxlp;
}

static int wordchr(char ch)
{
	return isalnum(ch) || strchr("$,=.:\\\"\'<>!",ch);
}

static void wordright(void)
{
	if (lp==maxlp)
	{
		beep();
		return;
	}
	
	if (wordchr(lbuf[lp]))
	{
		while (lp<maxlp && wordchr(lbuf[lp+1]))
			rawout(lbuf[lp++]);
		while (lp<maxlp && !wordchr(lbuf[lp+1]))
			rawout(lbuf[lp++]);
	}
	else
	{
		while (lp<maxlp && !wordchr(lbuf[lp+1]))
			rawout(lbuf[lp++]);
	}
	if (lp<maxlp)
		rawout(lbuf[lp++]);
}

static void wordleft(void)
{
	if (lp==0)
	{
		beep();
		return;
	}
	
	if (wordchr(lbuf[lp]))
	{
		while (lp>0 && !wordchr(lbuf[lp-1]))
		{
			backspace(1,FALSE);
			--lp;
		}
		while (lp>0 && wordchr(lbuf[lp-1]))
		{
			backspace(1,FALSE);
			--lp;
		}
	}
	else
	{
		while (lp>0 && !wordchr(lbuf[lp-1]))
		{
			backspace(1,FALSE);
			--lp;
		}
		while (lp>0 && wordchr(lbuf[lp-1]))
		{
			backspace(1,FALSE);
			--lp;
		}
	}
}

static void parsescan(int scn)
{
	int keyno;
	char *fk;
	void insertchar(char chr,char scn);

	if (cmdlinemode)
	{
		if ((scn>=F1 && scn<=F10) || (scn>=SF1 && scn<=SF10))
		{
			keyno=convkey(scn);
			fk=getfkey(keyno);
			while (*fk)
			{
				if (*fk=='|' && *(fk+1)=='\0')
				{
					++fk;
					fkeycr = TRUE;
				}
				else
					insertchar(*fk++,0);
			}
			return;
		}
	}
	
	switch (scn)
	{
		case INSERT:
			insflag = !insflag;
#if STANDALONE
			Cursconf(insflag+2,0);
#endif
			break;
		case LT_ARROW:
			if (lp>0)
			{
				backspace(1,FALSE);
				--lp;
			}
			else
				beep();
			break;
		case RT_ARROW:
			if (lp<maxlp)
				rawout(lbuf[lp++]);
			else
			{
				if (cmdlinemode && maxhp>0 && strlen(hbuf[0])>lp)
					insertchar(hbuf[0][lp],0);
				else
					beep();
			}
			break;
		case UP_ARROW:
		case DN_ARROW:
			if (cmdlinemode)
			{
				histscroll(scn==UP_ARROW,FALSE);
				printscroll();
			}
			else
				beep();
			break;
		case SUP_ARROW:
		case SDN_ARROW:
			if (cmdlinemode)
				matchscroll(scn==SUP_ARROW);
			else
				beep();
			break;
		case HELP:
			if (cmdlinemode)
				showhist();
			else
				beep();
			break;
		case SRT_ARROW:
			if (lp<maxlp)
			{
				cursoroff();
				while (lp<maxlp)
					rawout(lbuf[lp++]);
				cursoron();
			}
			break;
		case SLT_ARROW:
			if (lp>0)
			{
				cursoroff();
				backspace(lp,FALSE);
				lp=0;
				cursoron();
			}
			break;
		case CLT_ARROW:
			wordleft();
			break;
		case CRT_ARROW:
			wordright();
			break;
		default:
			beep();
			break;
	}
}

static void deltoeol(void)
{
	int xcur, ycur;
	
	lbuf[lp]='\0';
	getcursor(&xcur,&ycur);
	cursoroff();
	rawoutn(' ',maxlp-lp);
	vt52cm(xcur,ycur);
	cursoron();
	maxlp=lp;
}

static void exchange(void)
{
	char tmp;
	
	if (lp<maxlp-1)
	{
		tmp = lbuf[lp];
		lbuf[lp] = lbuf[lp+1];
		lbuf[lp+1] = tmp;
		rawout(lbuf[lp]);
		rawout(lbuf[lp+1]);
		backspace(2,FALSE);
	}
	else
		beep();
}

static void changecase(void)
{
	if (lp<maxlp)
	{
		if (isupper(lbuf[lp]))
			lbuf[lp] = tolower(lbuf[lp]);
		else
			if (islower(lbuf[lp]))
				lbuf[lp] = toupper(lbuf[lp]);
		rawout(lbuf[lp]);
		++lp;
	}
	else
		beep();
}

static void deletechar(int chr)
{
	int sl;
	int error = FALSE;
	
	if (lp>0 || (chr==DEL && lp>=0))
	{
		if (lp < maxlp)	/* middle of line ? */
		{
			cursoroff();
			if (chr == BS)
			{
				memmove(&lbuf[lp-1],&lbuf[lp],maxlp-lp+1);
				backspace(1,FALSE);
				sl=outstr(&lbuf[lp-1])+1;
				rawout(' ');
			}
			else
		 	{
				memmove(&lbuf[lp],&lbuf[lp+1],maxlp-lp);
				sl=outstr(&lbuf[lp])+1;
				rawout(' ');
			}
			backspace(sl,FALSE);
			cursoron();
		 	lbuf[maxlp-1]=NUL;
		}
		else /* at end of line */
		{
			if (lp > 0)
			{
				backspace(1,FALSE);
				rawout(' ');
				backspace(1,FALSE);
				lbuf[lp-1]=NUL;
				if (chr == DEL)
					--lp;
			}
			else
			{
				++error;
				beep();
			}
		}
		if (chr == BS)
			--lp;
		if (!error)
			--maxlp;
	}
	else
		beep();

	if (lp == 0)
		firstscroll=TRUE;
}

static void insertchar(char chr,char scn)
{
	int xcur, ycur;
	
	/* is it a shifted arrow key? */
	if ((chr=='6' && scn==RT_ARROW) || (chr=='4' && scn==LT_ARROW) ||
	    (chr=='8' && scn==UP_ARROW) || (chr=='2' && scn==DN_ARROW))
	{
		chr = NUL;
		scn = -scn;	/* value for S??_ARROW */
	}
	if (chr==NUL)
	{
		/* Process scan codes */
		parsescan(scn);
		return;
	}
	if (lp==maxlp)
	{
		/* try to add char at end of line */
		if (maxlp<LINESIZE)
		{
			rawout(chr);
			lbuf[lp]=chr;
			++lp;
			++maxlp;
			lbuf[maxlp]=NUL;
		}
		else
			beep();
		return;
	}
	/* insert char in the middle */
	if (insflag)
	{
		if (maxlp<LINESIZE)
		{
			int l;
			
			memmove(&lbuf[lp+1],&lbuf[lp],maxlp-lp+1);
			lbuf[lp]=chr;
			++maxlp;
			lbuf[maxlp]=NUL;
			getcursor(&xcur,&ycur);
			cursoroff();
			l=outstr(&lbuf[lp]);
			if (ycur==_maxrow && xcur+l>_maxcol)
				--ycur;
			vt52cm(xcur,ycur);
			rawout(lbuf[lp]);
			++lp;
			cursoron();
		}
		else
			beep();
		return;
	}
	else
	{
		if (lp<maxlp)
		{
			lbuf[lp]=chr;
			++lp;
			rawout(chr);
		}
		else
			beep();
	}
}

/* Initialize history buffer, set Screen to wraparound */
/* Must be called once during init sequence */
void lineedinit(void)
{
#if STANDALONE
	wrapon();
	cursoron();
	clearscreen();
#endif
}

void histinit(void)
{
	int i;
	
	maxhp= -1;

	maxhist=getvarint("history");
	if (maxhist == 0)
	{
		setvar("history","20");
		maxhist = 20;
	}
	hbuf = malloc(maxhist * sizeof(char *));
	for (i=0; i<maxhist; ++i)
		hbuf[i]=NULL;
	readhistory(maxhist,hbuf,&maxhp);
}

void lineedexit(void)
{
	int i;

	if (histchanged)	
		writehistory(maxhp,hbuf);	
	for (i=0; i<maxhist; ++i)
		if (hbuf[i]!=NULL)
			free(hbuf[i]);
	free(hbuf);
}

/* place a new line in first history buffer entry */
static void histcopy(void)
{
	int i, max;

	/* Reset History scroll pointer */
	hp=-1;
	/* Don't store empty lines */
	if (maxlp==0)
		return;
	/* Don't store same line again */
	if (maxhp!=-1 && !strcmp(hbuf[0],lbuf))
		return;
	if (getvar("nohistdouble") != NULL)
	{
		for (i=1; i<=maxhp; ++i)
			if (!strcmp(hbuf[i],lbuf))
			{
				int j;
				
				for (j=i-1; j>=0; --j)
					newstr(&hbuf[j+1],hbuf[j]);
				newstr(&hbuf[0],lbuf);
				return;
			}
	}
	histchanged = TRUE;
	/* History buffer still empty ? */
	if (maxhp!=-1)
	{
		/* If not first history entry, move previous ones up */
		if (maxhp<maxhist-1)
		{
			max=maxhp;
			++maxhp;
		}
		else
			max=maxhp-1;
		for (i=max; i>=0; --i)
			newstr(&hbuf[i+1],hbuf[i]);
	}
	else
		++maxhp;
	newstr(&hbuf[0],lbuf);
}
		
char *feedchar (long l)
{
	int chr,scn;
	int done = FALSE;

	fkeycr = FALSE;

	chr=(int)(l & 0xFF);
	scn=(int)((l>>16) & 0xFF);

	if (scn==0)
	{
		insertchar(chr,scn);
		chr = NUL;
		return NULL;
	}

	switch (chr)
	{
		default:
			insertchar(chr,scn);
			if (!fkeycr)
				break;
			chr=CR;
		case CR:
			lbuf[maxlp] = NUL;
			if (cmdlinemode)
				histcopy();
			done = firstscroll = TRUE;
			break;
		case ESC:
			eraseline();
			lp = maxlp = 0;
			hp = -1;
			firstscroll = TRUE;
			memset(lbuf,NUL,LINSIZ);
			break;
		case BS:
		case DEL:
			deletechar(chr);
			break;
		case TAB:
			{
				char *found;
				
				while (lbuf[lp] && wordchr (lbuf[lp]))
					insertchar (0, RT_ARROW);
				
				found = lcomplete (lbuf, lp);

				if (found)
				{
					char *c = found;
					
					while (*c) insertchar (*c++, 0);
					free (found);
				}
			}
			break;
		case CTRLDEL:
			deltoeol();
			break;
		case CTRLX:
			exchange();
			break;
		case CTRLC:
			changecase();
			break;
	}
	if (done)
	{
		if (cmdlinemode)
		{
			crlf();
			if (!lbuf[0])
			{
				initline(TRUE);
				done = FALSE;
			}
		}
	}
	
	return done ? lbuf : NULL;
}

void initline (int flag)
{
	lp = maxlp = 0;
	firstscroll = TRUE;
	hp = -1;
	insflag = 1;
	cmdlinemode = flag;
	
#if STANDALONE
	Cursconf(insflag+2,0);
#endif

	if (cmdlinemode)
		showprompt();
	memset(lbuf,NUL,LINSIZ);
}


char *readline (int flag)
{
	char *complete;

	initline (flag);

	do
	{
		complete = feedchar(inchar());
	} while (!complete);

	return complete;
}


/* 
 * void showprompt(void)
 * Display prompt. Normally from $PS1, if not set use "$ " 
 */
static void showprompt(void)
{
	char *p;
	int i=0;
	int reverse=0;
	
	if ((p=getenv("PS1"))==NULL)
		p="$ ";
	while (p[i]!='\0')
	{
		if (p[i]=='%')
		{
			if (p[i+1]!='\0')
			{
				++i;
				switch (p[i])
				{
					case '%':
						rawout('%');
						break;
					case 'P':
					case 'p':
						if (p[i] == 'p')
							mprintf("%c:%s",tolower(getdrv()),
								getdir());
						else
						{
							char *p = strdup(getdir());

							mprintf("%c:%s",getdrv(),strupr(p));
							free(p);
						}
						break;
					case 'N':
					case 'n':
						crlf();
						break;
					case 'D':
					case 'd':
						print_date ("%x");
						break;
					case 'T':
					case 't':
						print_date ("%X");
						break;
					case 'i':
						reverseon();
						++reverse;
						break;
					case 'I':
						reverseoff();
						--reverse;
						break;
				}
			}
			else
			{
				mprintf("$ ");
				break;
			}
		}
		else
			rawout(p[i]);
		++i;
	}
	if (reverse)
		reverseoff();
}
