/*
	@(#)Mupfel/fcomplet.c
	@(#)Julian F. Reschke, 9. Februar 1991
	
	Filename completion

	keine globalen Variablen, allocs werden gecheckt

	21.01.91:	Completion mit leeren Namen tut's jetzt auch
*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "attrib.h"
#include "chario.h"
#include "curdir.h"
#include "mupfel.h"
#include "shellvar.h"

/* returns pointer to allocated string containing characters that
   might be inserted. Beeping on error is done locally, may return
   NULL on error */

static char *fcomplete (char *what)
{
	DTA *oldDTA, newDTA;
	char *name, *newname, *tmp, *t;
	char merkname[14], cmp_name[14];
	int stat;
	int something_found = FALSE;
	int full_name = FALSE;
	int was_dir = FALSE;
	int slash_found = FALSE;

	tmp = umalloc (strlen (what) + 4);
	
	if (!tmp)
	{
		beep ();
		return NULL;
	}

	strcpy (tmp, what);
	if (!strlen (tmp)) strcpy (tmp, ".\\");

	if (getvar("slashconv"))
	{
		char *slash;
		
		while ((slash=strpbrk(tmp,"/"))!=NULL)
		{
			*slash = '\\';
			slash_found = TRUE;
		}
	}
	
	name = normalize (tmp);
	
	if (!name)
	{
		free (tmp);
		beep ();
		return NULL;
	}
	
	newname = umalloc (strlen (name) + 14);

	if (!newname)
	{
		free (tmp);
		free (name);		
		beep ();
		return NULL;
	}
	
	strcpy (newname, name);
	free (name);	

	*merkname = 0;	/* record file name to search for */
	t = strrchr (newname, '\\');
	
	if (!t)
	{
		free (tmp);
		free (newname);		
		beep ();
		return NULL;
	}	
	
	strcpy (merkname, &t[1]);
	strcpy (&t[1], "*.*");
	*cmp_name = 0;
	
	oldDTA = Fgetdta ();
	Fsetdta (&newDTA);
	
	stat = Fsfirst (newname, FA_ATTRIB);
	
	while (!stat)
	{
		/* match? */
		
		if (!strnicmp (merkname, newDTA.d_fname, strlen (merkname)) &&
			strcmp (newDTA.d_fname, ".") && strcmp (newDTA.d_fname, ".."))
		{
			if (!something_found)			/* first match? */
			{
				full_name = TRUE;
				something_found = TRUE;
				strcpy (cmp_name, newDTA.d_fname);
				if (newDTA.d_attrib & FA_DIREC) was_dir = TRUE;
			}
			else
			{
				int i;
				
				full_name = FALSE;
				for (i=0; i<strlen (cmp_name); i++)
					if (toupper(cmp_name[i]) != toupper(newDTA.d_fname[i]))
					{
						cmp_name[i] = 0;
						break;
					}
			}
		}
	
		stat = Fsnext ();
	}	

	Fsetdta (oldDTA);
	free (tmp);

	if (!strlen (cmp_name))
	{
		if (!something_found) beep ();
		free (newname);
		return NULL;
	}

	strlwr (cmp_name);
	strcpy (newname, &cmp_name[strlen(merkname)]);
	if (full_name)
		chrapp (newname, was_dir ? (slash_found ? '/' : '\\') : ' ');

	return newname;
}

/* returns pointer to allocated string containing characters that
   might be inserted. Beeping on error is done locally, may return
   NULL on error */

char *lcomplete (char *line, int linepos)
{
	/* search last occurence of blanks */
	
	char *last_blank;
	char next_c;
	
	next_c = line[linepos+1];

	if ((next_c != 0) && (next_c != ' '))
		return NULL;
	
	last_blank = strrchr (line, ' ');
	if (!last_blank)
		last_blank = line;
	else
		last_blank = &last_blank[1];

	if (last_blank[0] == '~')	/* ignore ~ */
		last_blank = &last_blank[1];
		
	return (fcomplete (last_blank));
}