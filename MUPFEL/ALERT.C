/*
 * alert.c  -  Komfortables Frontend fÅr form_alert()
 *
 * 23.06.90
 *
 * Definiert die Funktion
 * int alert(int icon,int defbutton,int numbuttons,char *fmt, ...)
 *
 * Aufruf:
 * alert(
 *	<icon_nummer>,				0 - 3                (Keins, !, ?, STOP)
 *	<default_button>,			0 - <anzahl_buttons> (0: kein Default)
 *	<anzahl_buttons>,			1 - 3
 *	<sprintf_format>,			siehe sprintf()
 *	<button_texte>,...,			soviele wie <anzahl_buttons>
 *	<argumente fÅr sprintf>,...);	je nach <sprintf_format>
 */

#include <aes.h>
#include <stdio.h>
#include <string.h>

#include "alert.h"
#include "mupfel.h"
#include "stand.h"

#ifdef RUNNER
#undef MERGED
#undef STANDALONE
#define MERGED 0
#define STANDALONE 1
#endif

#if MERGED
#include "flydial\flydial.h"
#define ILLCHARS "\n"
#else
#define ILLCHARS "[]|\n"
#endif

#define MAXLEN		30
#define MAXLINES	5

SCCS(alert);

static void changechars(char *str)
{
	char *p;
	
	while ((p=strpbrk(str,ILLCHARS))!=NULL)
	{
		switch (*p)
		{
			case '[':
				*p='(';
				break;
			case ']':
				*p=')';
				break;
			case '|':
				*p='!';
				break;
			case '\n':
				*p=' ';
				break;
		}
	}
}

#if STANDALONE
static int searchblank(char *str)
{
	char *blank, s[MAXLEN+1];
	int bpos = 0;
	
	strncpy(s,str,MAXLEN);
	s[MAXLEN]='\0';
	blank = strrchr(s,' ');
	if (blank!=NULL)
		bpos = (int)(blank - s);
	return bpos;
}

static void appendlines(char *alert, char line[MAXLINES][MAXLEN+1])
{
	int i;
	
	for (i=0; i<MAXLINES && line[i][0]!='\0'; ++i)
	{
		if (i>0)
			strcat(alert,"|");
		strcat(alert,line[i]);
	}
}
#endif

int alert(int icon,int defbutton,int numbuttons,char *fmt, ...)
{
	va_list argpoint;
	char line[MAXLINES][MAXLEN+1];	/* Textzeilen */
	char alert[200];				/* der fertige Alert */
	char buttons[100];
	char tmpstr[200];				/* fÅr vsprintf() */
	char *s;
	int i;

	/* Zeilen initialisieren */
	for (i=0; i<MAXLINES; *line[i++]='\0')
		;

	/* Button-Texte Åberspringen */
	va_start(argpoint,fmt);
	for (i=0; i<numbuttons; ++i)
		s = va_arg(argpoint,char *);
	
	/* Alert-Text vorbereiten */
	vsprintf(tmpstr,fmt,argpoint);
	va_end(argpoint);

	/* '[', ']' und '|' sind nicht erlaubt... */
	changechars(tmpstr);
#if STANDALONE
	
	s=tmpstr;
	for (i=0; i<MAXLINES; ++i)
	{
		if ((int)strlen(s)>MAXLEN)			/* noch zu lang */
		{
			char *blank = s + searchblank(s);	/* Blank suchen */
			if (blank==s)					/* keiner da    */
			{
				strncpy(line[i],s,MAXLEN);	/* MAXLEN Zeichen */
				line[i][MAXLEN]='\0';		/* kopieren       */
				s += MAXLEN;
			}
			else							/* Blank gefunden */
			{
				*blank = '\0';				/* String terminieren */
				strcpy(line[i],s);	
				s = blank+1;				/* Nach Blank weiter */
			}
		}
		else
		{
			strcpy(line[i],s);
			break;
		}
	}
#else
	strcpy(alert,tmpstr);
#endif

	/* Die Buttons */
	*buttons = '\0';
	va_start(argpoint,fmt);
	for (i=0; i<numbuttons; ++i)
	{
		s = va_arg(argpoint,char*);
		if (i>0)
			chrcat(buttons,'|');
#if MERGED
		chrcat(buttons,'[');		/* for FlyDial shortcut */
#endif
		strcat(buttons,s);
	}
	va_end(argpoint);
	
	if (defbutton<0 || defbutton>numbuttons)
		defbutton=1;

	if (icon<NO_ICON || icon>STOP_ICON)
		icon = NOTE_ICON;

#if STANDALONE
	graf_mouse(ARROW,NULL);
#else
	GrafMouse(ARROW,NULL);
#endif

#if STANDALONE
	/* Alert zusammenbasteln */
	sprintf(alert,"[%d][",icon);

	appendlines(alert,line);
	strcat(alert,"][");
	strcat(alert,buttons);
	strcat(alert,"]");
	
	return form_alert(defbutton,alert);
#else
	{
		BITBLK *pbit[4];

		pbit[NO_ICON] = NULL;
		pbit[NOTE_ICON] = ImSqExclamation();
		pbit[WAIT_ICON] = ImSqQuestionMark();
		pbit[STOP_ICON] = ImBomb();

		return DialAlert(pbit[icon],alert,defbutton-1,buttons)+1;
	}
#endif
}

void alertstr(char *str)
{
	alert(1,1,1,str,"OK");
}
