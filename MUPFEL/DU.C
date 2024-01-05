/*
	@(#)Mupfel/du.c
	@(#)Julian F. Reschke, 10. Februar 1991
	
	disk usage 
*/

#include <stddef.h>
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "attrib.h"
#include "chario.h"
#include "curdir.h"
#include "getopt.h"
#include "ls.h"
#include "messages.h"
#include "mupfel.h"
#include "toserr.h"

typedef struct
{
	int summarize, total, all;
} DUINFO;

static void
show_info (long blocks, char *file)
{
	mprintf ("%6ld %s\n", blocks, file);
}

static long
round (long blocks)
{
	return (blocks + 1023L) / 1024L;
}

/* du about the contents of a folder */

static int
dudir (DUINFO *D, char *file, long *sum)
{
	DTA dta, *old_dta = Fgetdta ();
	char *new_name;
	int stat;

	if (intr()) return 2;
	
	Fsetdta (&dta);
	new_name = umalloc (strlen (file) + strlen ("\\*.*") + 2);
	
	if (!new_name)
	{
		eprintf ("out of mem\n");
		Fsetdta (old_dta);
		return 1;
	}
	
	strcpy (new_name, file);
	
	if (lastchr (new_name) != '\\') strcat (new_name, "\\");
	strcat (new_name, "*.*");
	
	stat = normalized_fsfirst (new_name, FA_ATTRIB);
	
	while (!stat)
	{
		if (dta.d_attrib & FA_DIREC)
		{
			char *str = umalloc (strlen (file) + 20);
			
			if (!str)
			{
				eprintf ("du: " DU_NOMEM "\n");
				Fsetdta (old_dta);
				return 1;
			}
			
			strcpy (str, file);
			if (lastchr (str) != '\\') strcat (str, "\\");
			strcat (str, dta.d_fname);
			
			if (is_not_dot_or_dotdot (dta.d_fname))
			{
				long count = 0L;
				
				strlwr (str);
				dudir (D, str, &count);
				
				if (!D->summarize)
					show_info (count, str);
					
				*sum += count;				
			}
			free (str);			
		}
		else
		{
			if (D->all && !D->summarize)
			{
				strlwr (dta.d_fname);
				mprintf ("%6ld %s\\%s\n", round (dta.d_length), file, dta.d_fname);
			}
			
			*sum += round (dta.d_length);
		}	
		
		stat = Fsnext ();

		if (intr()) stat = 2;
	}
	
	if (stat == ENMFIL) stat = 0;
	
	Fsetdta (old_dta);
	free (new_name);
	return stat;
}



/* this functions does most of the job */

static int
du (DUINFO *D, char *file, int immediate_arg, long *sum)
{
	int retcode;
	long blocks = 0L;
	DTA	my_dta, *old_dta = Fgetdta ();
	
	Fsetdta (&my_dta);
	
	if (normalized_fsfirst (file, FA_ATTRIB))
	{
		/* Hack fÅr drv:\, da Fsfirst das nicht kann */
	
		if (might_be_dir (file))
			my_dta.d_attrib |= FA_DIREC;
		else
		{
			eprintf ("du: " DU_ILLFILE "\n", file);
			Fsetdta (old_dta);
			return 1;
		}
	}

	if (my_dta.d_attrib & FA_DIREC)
	{
		int ret;
	
		ret = dudir (D, file, &blocks);
		if (!ret)
			show_info (blocks, file);
		else
			retcode = ret;	
	}
	else
	{
		blocks = round (my_dta.d_length);
		if (immediate_arg)
			show_info (blocks, file);
	}
	
	*sum += blocks;
	Fsetdta (old_dta);
	return retcode;
}



/* main function for `du' */

int
m_du (int argc, char **argv)
{
	GETOPTINFO G;
	DUINFO D;
	long sum = 0L;
	int opt_index = 0, i, ret = 0;
	char c;
	
	struct option long_options[] =
	{
		{"all",			0, NULL, 'a'},
		{"total",		0, NULL, 'c'},
		{"summarize",	0, NULL, 's'},
		{NULL, 0, 0, 0},
	};

	D.all = D.summarize = D.total = 0;

	optinit (&G);

	while ((c = getopt_long (&G, argc, argv, "acs", long_options,
		&opt_index)) != EOF)
	{
		if (!c)			c = long_options[G.option_index].val;
		switch (c)
		{
			case 0:
				break;
			case 'a':
				D.all = 1;
				break;
			case 'c':
				D.total = 1;
				break;
			case 's':
				D.summarize = 1;
				break;
			default:
				return printusage(long_options);
		}
	}

	if (argc == G.optind)
		ret = du (&D, ".", TRUE, &sum);
	else
	{
		for (i = G.optind; (i < argc) && !intr(); i++)
		{
			ret = du (&D, argv[i], TRUE, &sum);
			
			if (ret == 2)	/* fatal error? */
				break;
		}

		if (D.total && !intr())
			mprintf ("total %ld\n", sum);
	}

	return ret;
}
