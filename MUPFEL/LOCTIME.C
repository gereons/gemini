/*
	@(#)Mupfel/loctime.c
	@(#)Julian F. Reschke, 4. Februar 1991
	
	local time functions

	keine relevanten Variablen, umalloc wird benutzt
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "alloc.h"
#include "chario.h"
#include "loctime.h"
#include "messages.h"

#define MAXLEN 1000
#define MAXTIMELEN	30		/* for calls of strftime */

/* Multilanguage support */

typedef struct
{
	char *daynames[7];
	char *fulldaynames[7];
	char *monthnames[12];
	char *fullmonthnames[12];
	char *std_x, *std_X, *std_full, *std_tz;
} TIMESTR;

TIMESTR EnglishTime =
{
	{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
	{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
	"Friday", "Saturday" },
	{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" },
	{ "January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December" },
	"%m/%d/%y", "%H:%M:%S",
	"%A %B, %e. %Y, %H:%M:%S  %Z",
	"GMT",
};

TIMESTR GermanTime =
{
	{"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"},
	{ "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag",
	"Freitag", "Samstag" },
	{ "Jan", "Feb", "M„r", "Apr", "Mai", "Jun",
	"Jul", "Aug", "Sep", "Okt", "Nov", "Dez" },
	{ "Januar", "Februar", "M„rz", "April", "Mai", "Juni",
	"Juli", "August", "September", "Oktober", "November", "Dezember" },
	"%d.%m.%y", "%H:%M:%S",
	"%A, %e. %B %Y, %H.%M Uhr %Z",
	"MEZ",
};

static TIMESTR *T;



/* Return formatted time string in allocated buffer, handle all
   cases unknown to TC's strftime, when called with format==NULL,
   the national standard full format, or $CFTIME is taken */

int conv_date (const char *format, char *to)
{
	time_t now = time (NULL);	/* get actual time */
	struct tm *t;
	char *merk = to;
	char *evar;
	
	t = localtime (&now);
	
	/* set up language tables */
	
	T = &EnglishTime;

	evar = getenv ("LANG");
	
	if (evar)
		if (!stricmp (evar, "german"))
			T = &GermanTime;
	
	if (!format)
	{
		evar = getenv ("CFTIME");
		
		if (evar)
			format = evar;
		else
			format = T->std_full;
	}
	
	
	while (*format)
	{
		if (*format == '%')
		{
			format += 1;
			
			switch (*format)
			{
				case 'a':
					to += sprintf (to, "%s", T->daynames[t->tm_wday]);
					format += 1;
					break;

				case 'b':
				case 'h':
					to += sprintf (to, "%s", T->monthnames[t->tm_mon]);
					format += 1;
					break;

				case 'e':
					to += sprintf (to, "%2d", t->tm_mday);
					format += 1;
					break;

				case 'n':
					*to++ = '\n';
					format += 1;
					break;

				case 'r':
					to += strftime (to, MAXTIMELEN, "%I:%M:%S %p", t);
					format += 1;
					break;

				case 't':
					*to++ = '\t';
					format += 1;
					break;

				case 'x':
					to += strftime (to, MAXTIMELEN, T->std_x, t);
					format += 1;
					break;

				case 'A':
					to += sprintf (to, "%s", T->fulldaynames[t->tm_wday]);
					format += 1;
					break;

				case 'B':
					to += sprintf (to, "%s", T->fullmonthnames[t->tm_mon]);
					format += 1;
					break;

				case 'D':
					to += sprintf (to, "%02d/%02d/%02d",
						t->tm_mon + 1, t->tm_mday, t->tm_year);
					format += 1;
					break;
						
				case 'R':
					to += strftime (to, MAXTIMELEN, "%I:%M", t);
					format += 1;
					break;

				case 'T':
					to += sprintf (to, "%02d:%02d:%02d",
						t->tm_hour, t->tm_min, t->tm_sec);
					format += 1;
					break;
						
				case 'X':
					to += strftime (to, MAXTIMELEN, T->std_X, t);
					format += 1;
					break;

				case 'Z':
					{
						evar = getenv ("TZNAME");
						
						if (!evar) evar = T->std_tz;
						
						to += sprintf (to, "%s", evar);
						format += 1;
					}
					break;

				default:
					{
						char fmtstr[3];
					
						sprintf (fmtstr, "%%%c", *format++);
						to += strftime (to, MAXTIMELEN, fmtstr, t);
					}
					break;
			}
		}
		else
			*to++ = *format++;
	}
	*to = 0;
	
	return (int) (to - merk);
}

void print_date (const char *format)
{
	char *tmpstr;
	
	tmpstr = umalloc (format ? strlen (format) * 10 : 1000);	/* enough? */
	if (!tmpstr)
	{
		mprintf (LO_NOMEM "\n");
		return;
	}
	
	conv_date (format, tmpstr);	
	mprintf ("%s", tmpstr);
	free (tmpstr);
}