/* 
 * echo.c - "echo" internal command
 * 25.09.90
 */

#include <string.h> 

#include "chario.h"
#include "keys.h"
#include "mupfel.h"

SCCS(echo);

static int newline;

/*
 * echoarg(char *str)
 * display str. may contain escape sequences:
 *	%t	TAB
 *   %b   Backspace
 *	%f   Formfeed
 *   %r   CR
 *	%n	CR/LF
 *   %%	%
 *	%c	(only at end of last arg) suppresses CR/LF
 *	%0	lead-in for up aribrary character, represented as max 3
 *		octal digits
 */
static void echoarg(char *str, int lastarg)
{
	char ch;
	
	while (*str != '\0')
	{
		if (*str=='%' && *(str+1)!='\0')
		{
			++str;
			switch (*str)
			{
				case 't':
					canout(TAB);
					break;
				case 'b':
					canout(BS);
					break;
				case 'f':
					canout(FF);
					break;
				case 'r':
					canout(CR);
					break;
				case 'c':
					if (lastarg && *(str+1)=='\0')
						newline = FALSE;
					else
						canout(*str);
					break;
				case 'n':
					crlf();
					break;
				case '0':
					++str;
					ch = '\0';
					while (*str>='0' && *str<='7')
					{
						ch = (ch*8) + (*str - '0');
						++str;
					}
					--str;
					canout(ch);
					break;
				default:
					canout(*str);
					break;
			}
		}
		else
			canout(*str);
		++str;
	}
}

/*
 * m_echo(ARGCV)
 * internal "echo" command
 * pass each argument to echoarg()
 */
int m_echo(ARGCV)
{
	int i;
	
	newline = TRUE;
	for (i=1; i<argc && !intr(); ++i)
	{
		echoarg(argv[i],(i==argc-1));
		if (i<argc-1)
			canout(' ');
	}
	if (newline)
		crlf();
	return 0;
}

/*
 * internal "pause" command
 * arguments are echoed without trailing cr/lf
 */
int m_pause(ARGCV)
{
	int i;
	
	if (argc==1)
		mprintf(EX_PRESSCR);
	else
	{
		for (i=1; i<argc; ++i)
		{
			echoarg(argv[i],(i==argc-1));
			canout(' ');
		}
	}
	inchar();
	crlf();
	return 0;
}
