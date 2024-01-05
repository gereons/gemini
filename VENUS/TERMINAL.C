/*
 * @(#) Gemini\Terminal.c
 *
 * project: mvmerge
 *
 * author: Arnd Beissner (changed by gs & se)
 *
 * description: functions for mupfel window
 *
 * last change: 07.02.1991
 */
 
#include <stddef.h>
#include <tos.h>
#include <string.h>

#include "vs.h"
#include "implimits.h"
#include "ascii.h"
#include "termio.h"
#include "terminal.h"

/* externals
 */
extern void storeLAcursor(int x,int y);		/* aus conio.s */


store_sccs_id(terminal);

/* Buffer fr 'save cursor' */
static word x_saved;
static word y_saved;

/* Buffer fr Escape-Modus: letztes Zeichen */
static int last_chr;
static int lastcols, lastrows;

static void _level0(char chr);
static void _level1(char chr);
static void _level2(char chr);
static void _level3(char chr);

/* vector for terminal output */
void (*term_out)(char) = _level0;

/* Terminal initialisieren */
char init_terminal(word *sysfonts)
{
	if (!TM_Init(sysfonts))
		return FALSE;
	
	x_saved = 0;
	y_saved = 0;
	cur_x = 0;
	cur_y = 0;
	rev_mode = FALSE;
	cur_on = TRUE;
	return TRUE;
}

/* Terminal deinitialisieren */
void exit_terminal(void)
{
	TM_Deinit();
}


/* Terminal updaten */
void reinit_terminal(void)
{
	if (cur_y >= termconf.rows)
	{
		cur_x = 0;
		cur_y = termconf.rows-1;
	}
	else if (cur_x >= termconf.columns)
		cur_x = termconf.columns-1;
}
		
void SizeScreenBuffer(void)
{
	word y;
	word diffcols = termconf.columns - lastcols;

	for (y = 0; y < lastrows; ++y)
	{
		if (diffcols > 0)
		{
			memset(&screen[y][lastcols], ' ', diffcols);
			memset(&attrib[y][lastcols], normal, diffcols);
		}
		screen[y][termconf.columns] = 0;	
	}

	for (y = lastrows; y < termconf.rows; ++y)
	{
		memset(screen[y], ' ', termconf.columns);
		memset(attrib[y], normal, termconf.columns);
		screen[y][termconf.columns] = 0;	
	}
	
	lastcols = termconf.columns;
	lastrows = termconf.rows;
}

/* Bildschirm-Buffer l”schen */	
void DelScreenBuffer(void)
{
	lastcols = lastrows = 0;
	SizeScreenBuffer();
}


/* Terminalfenster eine Zeile hinaufscrollen */
static void scroll_up(void)
{
	word y;

	/* eine Zeile im Buffer einfgen */
	for (y=1;y<termconf.rows;y++)
	{
		memcpy(screen[y-1],screen[y],termconf.columns);
		memcpy(attrib[y-1],attrib[y],termconf.columns);
	}

	/* letzte Bufferzeile l”schen */
	memset(screen[termconf.rows-1],' ',termconf.columns);
	memset(attrib[termconf.rows-1],normal,termconf.columns);
	
	/* Fensterinhalt hochschieben */
	TM_DelLine(0);
}

/* Terminalfenster um eine Zeilen abw„rtsscrollen */
static void scroll_down(void)
{
	word y;

	/* eine Zeile im Buffer einfgen */
	for (y=termconf.rows-1;y>=0;y--)
	{
			memcpy(screen[y],screen[y-1],termconf.columns);
			memcpy(attrib[y],attrib[y-1],termconf.columns);
	}
	 
	/* erste Bufferzeile l”schen */
	memset(screen[0],' ',termconf.columns);
	memset(attrib[0],normal,termconf.columns);
	
	/* Fensterinhalt runterschieben */
	TM_InsLine(0);
}


/* VT-52: Invers-Modus einschalten */
void rev_on(void)
{
	rev_mode = TRUE;
}
 
/* VT-52: Invers-Modus ausschalten */
void rev_off(void)
{
	rev_mode = FALSE;
}


/* VT-52: termconf.wrap-Around einschalten */
void wrap_on(void)
{
	termconf.wrap = TRUE;
}


/* VT-52: termconf.wrap-Around ausschalten */
void wrap_off(void)
{
	termconf.wrap = FALSE;
}

/* VT-52: Textcursor logisch einschalten */
void show_cursor(void)
{
	cur_on = TRUE;
}
	

/* VT-52: Textcursor logisch ausschalten */
void hide_cursor(void)
{
	cur_on = FALSE;
}

	
void clear_screen(void)
{
	DelScreenBuffer();
	TM_YErase(0,termconf.rows-1);
}



/* VT-52 Funktion: Carriage Return */
void vt_cr(void)
{
	cur_x = 0;
}

	
/* VT-52 Funktion: Line Feed */
void vt_lf(void)
{
	if (cur_y<termconf.rows-1)
		cur_y++;
	else
		scroll_up();
}


void vt_c_right(void)
{
	if (cur_x<termconf.columns-1)
		cur_x++;
} 


void vt_c_left(void)
{
	if (cur_x>0)
		cur_x--;
} 


void vt_c_up(void)
{
	if (cur_y>0)
		cur_y--;
}
 
	
void vt_c_down(void)
{
	if (cur_y<termconf.rows-1)
		cur_y++;
}


void vt_c_home(void)
{
	cur_x = 0;
	cur_y = 0;
}


void vt_cl_home(void)
{
	vt_c_home();
	clear_screen();
}
	
	
void vt_c_set(int y, int x)
{
	cur_x = x - ' ';
	cur_y = y - ' ';

	/* prfen, ob Koordinaten gltig, andernfalls korrigieren */
	if (cur_x < 0)
		cur_x=0;
	else if (cur_x > termconf.columns-1)
		cur_x=termconf.columns-1;

	if (cur_y < 0)
		cur_y=0;
	else if (cur_y > termconf.rows-1)
		cur_y=termconf.rows-1;
}


void vt_cui(void)
{
	if (cur_y>0)
		cur_y--;
	else
		scroll_down();
}


void vt_c_save(void)
{
	x_saved = cur_x;
	y_saved = cur_y;
}
	
	
void vt_c_restore(void)
{
	cur_x = x_saved;
	cur_y = y_saved;
}


/* VT-52 Funktion: vor der Cursorzeile eine Leerzeile einfgen */
void vt_ins_line(void)
{
	word y;
	
	
	/* Bufferausschnitt verschieben */
	for (y=termconf.rows-1;y>cur_y;y--)
	{
		memcpy(screen[y],screen[y-1],termconf.columns);
		memcpy(attrib[y],attrib[y-1],termconf.columns);
	}
	
	/* Bufferzeile l”schen */
	memset(screen[cur_y],' ',termconf.columns);
	memset(attrib[cur_y],normal,termconf.columns);
	
	TM_InsLine(cur_y);
	 	
	cur_x = 0;
}
	
	
/* VT-52 Funktion: die aktuelle Zeile entfernen */
void vt_del_line(void)
{
	word y;
	
	/* Bufferausschnitt verschieben */
	for (y=cur_y;y<termconf.rows-1;y++)
	{
		memcpy(screen[y],screen[y+1],termconf.columns);
		memcpy(attrib[y],attrib[y+1],termconf.columns);
	}

	/* Bufferzeile l”schen */	
	memset(screen[termconf.rows-1],' ',termconf.columns);
	memset(attrib[termconf.rows-1],normal,termconf.columns);
	
	TM_DelLine(cur_y);
	
	cur_x = 0;
}
	


/* VT-52-Funktion: aktuelle Zeile l”schen */
void vt_erline(void)
{
	memset(screen[cur_y],' ',termconf.columns);
	memset(attrib[cur_y],normal,termconf.columns);
	TM_XErase(cur_y,0,termconf.columns);
	
	cur_x = 0;
}

	

/* VT-52-Funktion: erase to line start */
void vt_erlst(void)
{
	if (cur_x==0) return;
	
	memset(screen[cur_y],' ',cur_x);
	memset(attrib[cur_y],normal,cur_x);
	TM_XErase(cur_y,0,cur_x);
}


/* VT-52 Funktion: bis zum Ende der Zeile l”schen */
void vt_cleol(void)
{
	memset(&(screen[cur_y][cur_x]),' ',termconf.columns-cur_x);
	memset(&(attrib[cur_y][cur_x]),normal,termconf.columns-cur_x);
	TM_XErase(cur_y,cur_x,termconf.columns);
}


/* VT-52 Funktion: bis zum Ende der Seite l”schen */
void vt_ereop(void)
{
	word y;

	vt_cleol();
	if (cur_y==termconf.rows-1) return;

	for (y=cur_y+1;y<termconf.rows;y++)
	{
		memset(screen[y],' ',termconf.columns);
		memset(attrib[y],normal,termconf.columns);
	}
	TM_YErase(cur_y+1,termconf.rows-1);
}


/* VT-52 Funktion: bis zum Anfang der Seite l”schen */
void vt_ersop(void)
{
	word y;
	
	vt_erlst();				/* bis zum Anfang der Zeile l”schen */
	if (cur_y==0) return;
	
	for (y=0;y<cur_y;y++)
	{
		memset(screen[y],' ',termconf.columns);
		memset(attrib[y],normal,termconf.columns);
	}
	TM_YErase(0,cur_y-1);
}


/* forward declaration */
void disp_char(char ch);

/* Tabulator ausgeben */
void tabulate(void)
{
	word new_x,i;
	
	if (termconf.tab_destructive)
	{
		new_x = (cur_x / termconf.tab_size) * termconf.tab_size 
				+ termconf.tab_size;
		if (new_x>=termconf.columns)
			new_x = termconf.columns-1;

		for (i=0;i<(new_x-cur_x);i++)
		{
			disp_char(' ');
		}
	}
	else
	{
		new_x = (cur_x / termconf.tab_size) * termconf.tab_size
				+ termconf.tab_size;

		if (new_x>=termconf.columns)
			new_x = termconf.columns-1;
	}
		
	cur_x = new_x;
}

	
/* Backspace ausgeben */
void t_backspace(void)
{
	if (termconf.back_destructive)
	{
		vt_c_left();
		disp_char(' ');
		vt_c_left();
	}
	else
		vt_c_left();
}


/* eine Zeile ausgeben */
static void _disp_str(char *str)
{
	word len;

	len = (word)strlen(str);
	
	/* Zeichen mit Attribut speichern */
	memcpy(&(screen[cur_y][cur_x]),str,len);
	memset(&(attrib[cur_y][cur_x]),rev_mode,len);

	/* Zeichen ausgeben */
	TM_DispString(str);

	/* Cursor neu positionieren */
	if (cur_x + len<termconf.columns)
		cur_x += len;
	else
	{
		if (termconf.wrap==1)
		{
			cur_x = termconf.columns - cur_x - len;
			if (cur_y<termconf.rows-1)
				cur_y++;
			else
				scroll_up();
		}
		else
			cur_x = termconf.columns - 1;
	}
}

void our_disp_str(char *str)
{
	size_t this_line;
	word done = FALSE;
	char save_ch;

	do
	{
		this_line  = strlen(str);
		if (this_line+cur_x <= termconf.columns)
		{
			_disp_str(str);
			done = TRUE;
		}
		else
		{
			this_line = termconf.columns - cur_x;
			save_ch = str[this_line];
			str[this_line] = '\0';
			_disp_str(str);
			str[this_line] = save_ch;
			str += this_line;
		}
	} while (!done);
}

void disp_string(char *str)
{
	rem_cur();
	our_disp_str(str);
	disp_cur();	
}

/* ein Zeichen ausgeben */
void disp_char(char ch)
{
	/* Zeichen mit Attribut speichern */
	screen[cur_y][cur_x] = ch;
	attrib[cur_y][cur_x] = rev_mode;

	/* Zeichen ausgeben */
	TM_DispChar(ch);

	/* Cursor neu positionieren */
	if (cur_x<termconf.columns)
		cur_x++;
	if (cur_x == termconf.columns)
	{
		if (termconf.wrap)
		{
			cur_x = 0;
			if (cur_y<termconf.rows-1)
				cur_y++;
			else
				scroll_up();
		}
		else
			cur_x = termconf.columns-1;
	}
}

/*
 * Front-ends for canonical and raw console output
 * (_o_con and _o_rawcon point to these routines)
 */
void disp_canchar(char ch)
{
	/* Bei normaler Ausgabe lassen wir den Cursor
	 * erstmal unberhrt.
	 */
	if (term_out == _level0)
		term_out(ch);
	else
	{
		rem_cur();
		term_out(ch);
		disp_cur();
		storeLAcursor(cur_x,cur_y);
	}
}

void disp_rawchar(char ch)
{
	rem_cur();
	disp_char(ch);
	disp_cur();
	storeLAcursor(cur_x,cur_y);
}

static void _level3(char chr)
{
	vt_c_set(last_chr, (unsigned char)chr);
	term_out = _level0;
}

static void _level2(char chr)
{
	last_chr = (unsigned char)chr;
	term_out = _level3;
}

static void setForeColor(char chr)
{
	TM_ForeColor(chr);
	term_out = _level0;
}

static void setBackColor(char chr)
{
	TM_BackColor(chr);
	term_out = _level0;
}

static void _level1(char chr)
{
	switch(chr)
	{
		case 'b':	term_out = setForeColor;
					return;
		case 'c':	term_out = setBackColor;
					return;
		case 'd':	vt_ersop(); 		break;
		case 'e':   show_cursor(); 		break;
		case 'f':   hide_cursor(); 		break;
		case 'j':   vt_c_save(); 		break;
		case 'k':   vt_c_restore(); 	break;
		case 'l':   vt_erline(); 		break; 
		case 'o':   vt_erlst(); 		break;
		case 'p':   rev_on(); 			break;
		case 'q':   rev_off(); 			break;
		case 'v':   wrap_on(); 			break;
		case 'w':   wrap_off(); 		break; 
		case 'A':   vt_c_up(); 			break;
		case 'B':   vt_c_down(); 		break;
		case 'C':   vt_c_right();		break;
		case 'D':   vt_c_left();		break;
		case 'E':   vt_cl_home();		break;
		case 'H':   vt_c_home();		break;
		case 'I':   vt_cui();	 		break;
		case 'J':	vt_ereop();			break;
		case 'K':	vt_cleol();			break;
		case 'L':	vt_ins_line();		break;
		case 'M':	vt_del_line();		break;
		case 'Y':   term_out = _level2;
					return;
	}
	term_out = _level0;
}

static unsigned char belldata[] = {0,0x34,1,0,2,0,3,0,4,0,5,0,6,0,
						7,0xfe,8,16,9,0,10,0,11,0,12,16,13,9,0xff,0};

static void JingleBells(void)
{
	Dosound(belldata);
}

static void _level0(char chr)
{
	switch(chr)
	{
		case ESC: term_out = _level1;
			break;
	   	case TAB: 
	   		rem_cur();
	   		tabulate();		
			disp_cur();
			storeLAcursor(cur_x,cur_y);
	   		break;
		case CR: 
			rem_cur();
			vt_cr();		
			disp_cur();
			storeLAcursor(cur_x,cur_y);
			break;
		case LF: 
			rem_cur();
			vt_lf();		
			disp_cur();
			storeLAcursor(cur_x,cur_y);
			break;
		case BEL:	JingleBells();	
			break;
		case BS:
			rem_cur();
			t_backspace();	
			disp_cur();
			storeLAcursor(cur_x,cur_y);
			break;
		default:
			disp_char(chr); 
			disp_cur();
			storeLAcursor(cur_x,cur_y);
			break;
	}
}

char escon(void)
{
	return (term_out != _level0);
}