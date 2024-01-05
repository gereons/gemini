/*
	@(#)Mupfel/smallcmd.c
	@(#)Julian F. Reschke, 7. 2. 1991

	very small commands: true false dirname basename getopt

	keine globalen Variablen, alle mallocs werden gecheckt
*/

#include <stdio.h>
#include <string.h>

#include "alloc.h"
#include "chario.h"
#include "getopt.h"
#include "mupfel.h"

SCCS (smallcmd);

int
m_true (ARGCV)
{
	(void)argc;
	(void)argv;

	return 0;
}

int
m_false (ARGCV)
{
	(void)argc;
	(void)argv;

	return 1;
}

int
m_dirname (ARGCV)
{
	char *found;

	if (argc == 1)
	{
		mprintf (".\n");
		return 0;
	}
	
	if (argc != 2)
		return printusage (NULL);

	found = strrchr (argv[1], '\\');
	
	if (found) *found = 0;
		
	mprintf ("%s\n", found ? argv[1] : ".");
	return 0;
}

int
m_basename (ARGCV)
{
	char *found;

	if (argc < 2 || argc > 3)
		return printusage (NULL);

	found = strrchr (argv[1], '\\');
	
	if (found)				/* start of filename */
		found = &found[1];
	else
		found = argv[1];

	if (argc == 3)			/* suffix given? */
	{
		size_t lext = strlen (argv[2]);
		size_t lname = strlen (found);
		
		if (!stricmp (&found[lname-lext], argv[2]))
			found[lname-lext] = 0;
	}

	mprintf ("%s\n", found);
	return 0;
}

int
m_getopt (ARGCV)
{
	GETOPTINFO G;
	int	c;
	int	errflg = 0;
	char tmpstr[4];
	char *outstr;
	int local_argc;
	char **local_argv;
	char *option_string;

	if (argc < 3)
		return printusage (NULL);

	local_argc = argc - 1;
	local_argv = ucalloc (argc + 1, sizeof (char *));

	outstr = umalloc (5120);	/* SYSV command line limit */

	if (!outstr || !local_argv)
	{
		if (outstr) free (outstr);
		if (local_argv) free (local_argv);
		eprintf (GO_OOMEMORY "\n", "getopt");
		return 2;
	}

	*outstr = 0;

	/* Argumentliste kopieren */
	
	{
		int i;

		for (i = 1; i < argc; i++)
			local_argv[i] = argv [i+1];

		local_argv[++i] = NULL;
		local_argv[0] = argv[0];
	}
	
	option_string = argv[1];
	
	if (*option_string == '-') option_string += 1;
	
	optinit (&G);

	while ((c = getopt (&G, local_argc, local_argv, option_string))
		!= EOF)
	{
		if (c == '?')
		{
			errflg++;
			continue;
		}

		sprintf (tmpstr, "-%c ", c);
		strcat (outstr, tmpstr);

		if (*(strchr(argv[1], c)+1) == ':')
		{
			strcat (outstr, G.optarg);
			strcat (outstr, " ");
		}
	}

	if (errflg)
	{
		free (outstr);
		free (local_argv);
		return 2;
	}
	
	strcat (outstr, "-- ");

	while (G.optind < local_argc)
	{
		strcat (outstr, local_argv[G.optind++]);
		strcat (outstr, " ");
	}
	
	{
		int i = 0;

		while (outstr[i])
			mprintf ("%c", outstr[i++]);
	}
	crlf ();
	
	free (outstr);
	free (local_argv);
	return 0;
}
