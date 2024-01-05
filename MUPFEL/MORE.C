/*
 * more.c  -  internal "more" command
 * 10.06.91
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>

#include "alert.h"
#include "chario.h"
#include "environ.h"
#include "filebuf.h"
#include "handle.h"
#include "keys.h"
#include "lineedit.h"
#include "mupfel.h"
#include "redir.h"
#include "scan.h"
#include "stand.h"
#include "sysvec.h"
#include "vt52.h"

SCCS(more);

enum
{
	END_MORE, SCREENFWD, LINEFWD , SCREENBWD, LINEBWD,
	NEXTFILE, PREVFILE, MOREINFO, ENDFILE, BEGINFILE,
	SRCHFWD, SRCHBWD, SRCHAGAIN
};

typedef struct
{
	char *buffer;
	char **linestart;
	size_t linecount;
	size_t filesize;
} moreinfo;

static int screenlines, screencols;
static int endmore;
static int currfile;
static int manyfiles;
static long lastsearch;
static moreinfo m;
static char *actfile;

static size_t countlines(void)
{
	size_t lines = 0;
	char *buf = m.buffer;

	while (*buf)
	{
		if (*buf == '\n')
			++lines;
		++buf;
	}
	return lines + 1L;
}

static int setuplines(void)
{
	size_t i = 0L;
	char *buf, *line;
	int add;

	m.linecount = countlines();
	m.linestart = Malloc(m.linecount * sizeof(char *));
	if (m.linestart == NULL)
		return FALSE;
		
	memset(m.linestart, 0, m.linecount * sizeof(char *));
	
	buf = m.buffer;
	m.linestart[i++] = buf;
	
	while ((line=strchr(buf,'\n'))!=NULL)
	{
		if (*(line-1)=='\r')
		{
			--line;
			add = 2;
		}
		else
			add = 1;
		*line = '\0';
		buf = m.linestart[i++] = line+add;
	}
	/* trailing cr/lf ? */
	if (m.linestart[i-1][0] == '\0')
		--m.linecount;

	if (i > m.linecount+1)
		alert(1,1,1,"more: too many lines! (%ld, %ld)","OK",
			i,m.linecount);
	return TRUE;
}

static size_t showline(size_t index)
{
	char *line;
	long len = 0;
	size_t usedlines;

	if (index >= m.linecount)
		return 0;

	if ((line = m.linestart[index])==NULL)
		return 0;

	cleareol();
#if MERGED
	if (conwindow)
	{
		char *tab;
		
		while ((tab=strchr(line,'\t'))!=NULL)
		{
			*tab = '\0';
			disp_string(line);
			len += strlen(line);
			*tab = '\t';
			line = tab+1;
			canout('\t');
			do
				++len;
			while (len % 8);
		}
		if (*line)
		{
			disp_string(line);
			len += strlen(line);
		}
		if (len==0 || (len % screencols))
			crlf();
	}
	else
#endif
	{
		while (*line)
		{
			if (*line == TAB)
			{
				canout(*line);
				do
					++len;
				while (len % 8);
			}
			else
			{
				rawout(*line);
				++len;
			}
			++line;
		}
		if (len==0 || (len % screencols))
			crlf();
	}

	usedlines = 0L;
	while (len > screencols)
	{
		len -= screencols;
		++usedlines;
	}	

	return usedlines;
}

static void showscreen(long *start)
{
	size_t i;
	size_t line;
	
	cursorhome();
	
	if (*start + screenlines > m.linecount)
		*start = m.linecount - screenlines;
	if (*start < 0)
		*start = 0;
	
	cursoroff();
	clearscreen();
	i = *start;
	line = 0L;
	while (line < screenlines)
	{
		line += showline(i)+1;
		++i;
	}
	cursoron();
}

static void morehelp(void)
{
	vt52cm(0,screenlines);
	reverseon();
	mprintf(MO_HELP);
	reverseoff();
	inchar();
}

static int percentage(long actline)
{
	if (actline > m.linecount)
		actline = m.linecount;
	if (m.linecount == 0)
		return 100;
	return (int)(actline * 100 / m.linecount);
}

static void showmoreinfo(long actline)
{
	vt52cm(0,screenlines);
	reverseon();
	if (actline > m.linecount)
		actline = m.linecount;
	mprintf(MO_INFO,actfile,actline,m.linecount,percentage(actline),
		m.filesize);
	reverseoff();
	inchar();
}

static void moreprompt(long actline)
{
	vt52cm(0,screenlines);
	reverseon();
	if (manyfiles)
		mprintf("(%s) ",actfile);
	mprintf("-more- (%d%%)",percentage(actline));
	reverseoff();
	cleareol();
}

static int morecommand(long actline)
{
	long c;
	int chr, scn;

	while (TRUE)
	{
		moreprompt(actline);
		c = inchar();
		/*
		 * if (_buttonpressed)
		 *	return END_MORE;
		 */
		chr = (int)(c & 0xff);
		scn = (int)((c>>16) & 0xff);
		/* is it a shifted arrow key? */
		if ((chr=='6' && scn==RT_ARROW) || (chr=='4' && scn==LT_ARROW) ||
		    (chr=='8' && scn==UP_ARROW) || (chr=='2' && scn==DN_ARROW) ||
		    (chr=='7' && scn==CLR))
		{
			chr = '\0';
			scn = -scn;
		}
		switch (toupper(chr))
		{
			case ' ':
			case CTRLF:
			case 'F':
				return SCREENFWD;
			case CR:
			case 'J':
			case '+':
				return LINEFWD;
			case BS:
			case CTRLB:
			case 'B':
				return SCREENBWD;
			case 'K':
			case '-':
				return LINEBWD;
			case 'Q':
			case ESC:
			case CTRLC:
				return END_MORE;
			case 'N':
				return NEXTFILE;
			case 'P':
				return PREVFILE;
			case CTRLG:
				return MOREINFO;
			case '0':
				return BEGINFILE;
			case '$':
			case 'G':
				return ENDFILE;
			case '/':
				return SRCHFWD;
			case '?':
				return SRCHBWD;
			case 'H':
				morehelp();
				break;
			case 'A':
				return SRCHAGAIN;
		}
		switch(scn)
		{
			case DN_ARROW:
				return LINEFWD;
			case UP_ARROW:
				return LINEBWD;
			case SDN_ARROW:
				return SCREENFWD;
			case SUP_ARROW:
				return SCREENBWD;
			case HELP:
				morehelp();
				break;
			case UNDO:
				return END_MORE;
			case CLR:
				return BEGINFILE;
			case SCLR:
				return ENDFILE;
			default:
				beep();
				break;
		}
	}
}

static void dosearch(int direction, long *start)
{
	long i;
	static int firsttime = TRUE;
	static int lastdir = -1;
	char *s;
	int found, done = FALSE;
	static char srch[LINSIZ];
	
	if (firsttime)
	{
		*srch = '\0';
		lastdir = direction;
		firsttime = FALSE;
	}
	vt52cm(0,screenlines);
	cleareol();

	if (direction != SRCHAGAIN)
	{
		rawout((direction == SRCHFWD) ? '/' : '?');
	
		s = readline(FALSE);
		if (*s=='\0' && *srch=='\0')
			return;
		if (*s)
			strcpy(srch,s);
	}
	else
	{
		if (*srch=='\0')
			return;
		direction = lastdir;
	}
				
	i = *start;
	
	if (i+screenlines >= m.linecount)
	{
		found = FALSE;
		done = TRUE;
	}
	else
		if (i==lastsearch)
		{
			if (direction == SRCHFWD)
			{
				if (i<m.linecount)
					++i;
			}
			else
			{
				if (i>0)
					--i;
			}
		}

	while (!done)
	{
		if (strstr(m.linestart[i],srch))
		{
			*start = i;
			lastsearch = i;
			showscreen(start);
			found = done = TRUE;
		}
		else
		{
			if (direction==SRCHFWD)
				done = ++i >= m.linecount;
			else
				done = --i < 0;
			found = FALSE;
		}
	}
	if (!found)
	{
		beep();
		lastdir = (direction == SRCHFWD) ? SRCHBWD : SRCHFWD;
	}
	else
		lastdir = direction;
}

static void domore(int argc)
{
	long start;
	int done = FALSE;
	int cmd;
	
	start = 0;
	
	clearscreen();
/*	wrapoff(); */
	showscreen(&start);
	do
	{
		switch (endmore = cmd = morecommand(start+screenlines))
		{
			case END_MORE:
				done = TRUE;
				break;
			case NEXTFILE:
				if (currfile == argc-1)
					beep();
				else
					done = TRUE;
				break;
			case PREVFILE:
				if (currfile == 1)
					beep();
				else
					done = TRUE;
				break;
			case SCREENFWD:
				if (start+screenlines >= m.linecount)
				{
					beep();
					break;
				}
				start += screenlines;
				showscreen(&start);
				break;
			case SCREENBWD:
				if (start==0)
				{
					beep();
					break;
				}
				start -= screenlines;
				showscreen(&start);
				break;
			case LINEFWD:
				if (start+screenlines >= m.linecount)
				{
					beep();
					break;
				}
				++start;
				cursorhome();
				deleteline();
				vt52cm(0,screenlines-1);
				showline(start+screenlines-1);
				break;
			case LINEBWD:
				if (start == 0)
				{
					beep();
					break;
				}
				--start;
				cursorhome();
				insertline();
				showline(start);
				break;
			case BEGINFILE:
				if (start != 0)
				{
					start = 0;
					showscreen(&start);
				}
				break;
			case ENDFILE:
				if (start < m.linecount-screenlines)
				{
					start = m.linecount;
					showscreen(&start);
				}
				break;
			case MOREINFO:
				showmoreinfo(start+screenlines);
				break;
			case SRCHFWD:
			case SRCHBWD:
			case SRCHAGAIN:
				dosearch(cmd,&start);
				break;
		}
	} while (!done);
/*	wrapon(); */
	crlf();
}

static int more(int argc, char *file)
{
	int hnd;
	
	if ((hnd=Fopen(file,O_RDONLY))<=MINHND)
	{
		eprintf("more: " MO_CANTOPEN "\n",file);
		return FALSE;
	}
	if ((m.filesize=filebuf(hnd,file,&m.buffer,"more"))==BUF_ERR)
	{
		Fclose(hnd);
		return FALSE;
	}

	if (!setuplines())
	{
		eprintf("more: " MO_NOMEM "\n");
		return FALSE;
	}
	actfile = file;
	lastsearch = -1;
				
	domore(argc);
		
	Mfree(m.buffer);
	Mfree(m.linestart);
	return TRUE;
}

int m_more(ARGCV)
{
	int retcode = 0;
	char *rows, *cols;
	
	if ((rows=getenv("ROWS"))!=NULL)
		screenlines = atoi(rows)-1;
	else
		screenlines = 24;
	if (screenlines <= 0)
		screenlines = 24;

	if ((cols=getenv("COLUMNS")) != NULL)
		screencols = atoi(cols);
	else
		screencols = 80;
	if (screencols <= 0)
		screencols = 80;

	endmore = -1;
				
	if (argc==1)
		if (isatty(0))
		{
			eprintf("more: " MO_ISTTY "\n");
			return 1;
		}
		else
			if (redirect.in.file)
			{
				argv[1] = redirect.in.file;
				argc = 2;
			}
			else
				return printusage(NULL);

	currfile = 1;
	manyfiles = argc > 2;
	while (endmore != END_MORE)
	{
		if (!more(argc,argv[currfile]))
			retcode = 1;
		(endmore == NEXTFILE) ? ++currfile : --currfile;
		if (currfile==0 || currfile==argc)
			break;
	}
	return retcode;
}
