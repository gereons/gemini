/*
 * wildmat.c  -  wildcard matching
 * 23.01.1991
 */
 
/*
**  Do shell-style pattern matching for ?, \, [], and * characters.
**  Might not be robust in face of malformed patterns; e.g., "foo[a-"
**  could cause a segmentation violation.
**
**  Written by Rich $alz, mirror!rs, Wed Nov 26 19:03:17 EST 1986.
*/

#include <string.h>

#include "mupfel.h"
#include "wildmat.h"

SCCS(wildmat);

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
	int last, matched, reverse;

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

int wildmatch(char *text, char *pattern)
{
	strupr(text);
	strupr(pattern);
	return matchpattern(text,pattern);
}
