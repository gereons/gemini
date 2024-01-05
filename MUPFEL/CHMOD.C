/*
	@(#)Mupfel/chmod.c
	@(#)Gereon Steffens, Julian F. Reschke, 13. Februar 1991
*/
 
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>

#include "attrib.h"
#include "chario.h"
#include "chmod.h"
#include "comm.h"
#include "handle.h"
#include "mupfel.h"

#define RW	0
#define ARCH	1
#define SYS	2
#define HID	3

SCCS(chmod);

/* program/object header */ 
typedef struct
{
	int branch;
	long textlen;
	long datalen;
	long bsslen;
	long symlen;
	long reserved1;
	struct
	{
		unsigned tpasize: 4;
		unsigned res1: 12;
		unsigned res2: 13;
		unsigned malt: 1;
		unsigned lalt: 1;
		unsigned fload: 1;
	} loadflags;
	int flag;
} HEADER;

int set_run_flags (const char *file, int fl, int lf, int mf, int tpa_size,
	int verbose, int changes_only, int silent)
{
	int hnd, can_write = TRUE;
	HEADER header;
	int changed = FALSE;
	
	if (!access (file, A_EXEC))
	{
		if (silent) return FALSE;
	
		if (access (file, A_EXIST))
			eprintf ("runopts: " RO_NOTEXEC "\n", file);
		else
		{
			if (isdir(file))
				eprintf("runopts: " RO_ISDIR "\n", file);
			else
				eprintf("runopts: " RO_NOFILE "\n", file);
		}
		
		return FALSE;
	}

	hnd = Fopen (file, 2);
	
	if (hnd < MINHND)
	{
		/* Datei kann nicht beschrieben werden -- also nur zum
		Lesen ”ffnen */
		
		can_write = FALSE;
		hnd = Fopen (file, 0);
		can_write = FALSE;
	}
	
	if (hnd < MINHND)
	{
		eprintf ("runopts: " RO_CANTOPEN "\n", file);
		return FALSE;
	}
		
	if (Fread (hnd, sizeof(HEADER), &header) != sizeof(HEADER))
		eprintf ("runopts: " RO_READHDR "\n", file);
	else
	{
		if ((fl != -1) && can_write)	/* set fastload */
		{
			if (header.loadflags.fload != fl)
			{
				changed = TRUE;
				header.loadflags.fload = fl;
			}
		}

		if ((lf != -1) && can_write)	/* set load to fast ram */
		{
			if (header.loadflags.lalt != lf)
			{
				changed = TRUE;
				header.loadflags.lalt = lf;
			}
		}

		if ((mf != -1) && can_write)	/* set malloc from fast ram */
		{
			if (header.loadflags.malt != mf)
			{
				changed = TRUE;
				header.loadflags.malt = mf;
			}
		}

		if ((tpa_size != -1) && can_write)	/* set tpasize */
		{
			if (header.loadflags.tpasize != tpa_size)
			{
				changed = TRUE;
				header.loadflags.tpasize = tpa_size;
			}
		}
			
		if (changed)	/* rewrite the header? */
		{
			Fseek (0L, hnd, 0);		/* back to start */
			
			if (Fwrite (hnd, sizeof(HEADER), &header) != sizeof(HEADER))
			{
				eprintf ("runopts: " RO_WRITEHDR "\n", file);
				Fclose (hnd);
				return FALSE;
			}
		}

		if (verbose || (changes_only && changed))
			mprintf ("%c%c%c %4dK  %s\n",
				(header.loadflags.fload) ? 'f' : '-',
				(header.loadflags.lalt) ? 'l' : '-',
				(header.loadflags.malt) ? 'm' : '-',
				(header.loadflags.tpasize + 1) * 128,
				file);						
	}

	Fclose(hnd);
	return TRUE;
}

static void
change_mode (char *file, int attr[], int verbose, int changes_only)
{
	int attrib, i, set, reset;
	int saveattrib;
	
	if ((attrib=Fattrib(file,GETATTRIB,0))<0)
	{
		eprintf("chmod: " CH_GETATTR "\n",file);
		return;
	}
	saveattrib = attrib;
	
	for (i=0; i<4; ++i)
		if (attr[i]!=-1)
		{
			switch (i)
			{
				case RW:
					set=FA_RDONLY;
					break;
				case ARCH:
					set=FA_ARCH;
					break;
				case SYS:
					set=FA_SYSTEM;
					break;
				case HID:
					set=FA_HIDDEN;
					break;
			}
			reset = ~set;
			if (attr[i])
				attrib |= set;
			else
				attrib &= reset;
		}
	
	if (saveattrib != attrib)
	{
		if (Fattrib(file,SETATTRIB,attrib)<0)
		{
			eprintf("chmod: " CH_SETATTR "\n",file);
			return;
		}
		else
			CommInfo.dirty |= drvbit(file);
	}

	if (verbose || (changes_only && (saveattrib != attrib)))
		mprintf ("%c%c%c%c (%03o) %s\n",
			attrib & FA_RDONLY ? 'r' : '-',
			attrib & FA_HIDDEN ? 'h' : '-',
			attrib & FA_SYSTEM ? 's' : '-',
			attrib & FA_ARCH   ? 'a' : '-',
			attrib, file);
}

int m_chmod (ARGCV)
{
	int i, attr[4] = {-1,-1,-1,-1};
	int something_done = FALSE;
	int changes_only = FALSE;
	int verbose = FALSE;
		
	for (i = 1; i < argc; i++)
	{
		char opt_char = argv[i][0];

		if (!strcmp (argv[i], "--"))
		{
			argv[i][0] = 0;
			break;
		}

		if ((opt_char == '+') || (opt_char == '-'))
		{
			size_t slen = strlen (argv[i]);
			int j , break_it = FALSE;
			
			for (j = 1; (j < slen) && (!break_it); j++)
			{
				switch (argv[i][j])
				{
					case 'w':
						attr[RW] = (opt_char == '+') ? 0 : 1;
						break;

					case 'r':
						attr[RW] = (opt_char == '+') ? 1 : 0;
						break;

					case 'a':
						attr[ARCH] = (opt_char == '+') ? 1 : 0;
						break;

					case 's':
						attr[SYS] = (opt_char == '+') ? 1 : 0;
						break;

					case 'h':
						attr[HID] = (opt_char == '+') ? 1 : 0;
						break;

					case 'c':
						if ((opt_char == '-') || (!strncmp ("+changes-only",
							argv[i], strlen (argv[i]))))
						{
							changes_only = TRUE;
							if (opt_char == '+') break_it = TRUE;
						}
						else	/* found beginning with '+' */
						{
							eprintf ("chmod: " CH_ILLOPTST "\n", argv[i]);
							return 2;
						}
						break;

					case 'v':
						if ((opt_char == '-') || (!strncmp ("+verbose", argv[i],
							strlen (argv[i]))))
						{
							verbose = TRUE;
							if (opt_char == '+') break_it = TRUE;
						}
						else	/* found beginning with '+' */
						{
							eprintf ("runopts: " CH_ILLOPTST "\n", argv[i]);
							return 2;
						}
						break;

					default:
						eprintf ("chmod: " CH_ILLOPT "\n", argv[i][j]);
						return 2;
				}
			}
			
			argv[i][0] = 0;
		}
	} 
	
	for (i = 1; (i < argc) && (!intr()); ++i)
	{
		if (argv[i][0])
		{
			something_done = TRUE;
			change_mode (argv[i], attr, verbose, changes_only);
		}
	}
	
	if (!something_done)
		return printusage (NULL);
	
	return 0;
}


/* calculate tpa size from ascii string, return -1 for error */

int calc_tpa (char *str)
{
	int mega_bytes = FALSE;
	int size;
	
	if (toupper (lastchr (str)) == 'M')
	{
		mega_bytes = TRUE;
		str[strlen(str)-1] = 0;
	}
	
	if (toupper (lastchr (str)) == 'K')	/* ignore k's */
		str[strlen(str)-1] = 0;

	if (!isdigit (lastchr (str)))
	{
		eprintf ("runopts: " RO_ILLSIZE "\n", str);
		return -1;
	}
		
	size = atoi (str);
	
	if (mega_bytes) size *= 1024;
	size = (size + 63) & ~127;	/* round to multiples of 128 */

	if ((size < 128) || (size > 2048))
	{
		if (!size)
			eprintf ("runopts: " RO_ILLSIZE "\n", str);
		else
			eprintf ("runopts: " RO_OUTRANGE "\n", str);
		return -1;
	}

	return size;	
}


int m_runopts (ARGCV)
{
	int i;
	int verbose, changes_only, silent;
	int fastload, loadtofast, mallocfast;
	int tpa_size = -1;
	int something_done;
		
	silent = verbose = changes_only = FALSE;
	fastload = loadtofast = mallocfast = -1;

	for (i = 1; i < argc; i++)
	{
		char opt_char = argv[i][0];

		if (!strcmp (argv[i], "--"))
		{
			argv[i][0] = 0;
			break;
		}

		if ((opt_char == '+') || (opt_char == '-'))
		{
			size_t slen = strlen (argv[i]);
			int j , break_it = FALSE;
			
			for (j = 1; (j < slen) && (!break_it); j++)
			{
				switch (argv[i][j])
				{
					case 'f':
						fastload = (opt_char == '+') ? 1 : 0;
						break;

					case 'l':
						loadtofast = (opt_char == '+') ? 1 : 0;
						break;

					case 'm':
						mallocfast = (opt_char == '+') ? 1 : 0;
						break;

					case 'c':
						if ((opt_char == '-') || (!strncmp ("+changes-only",
							argv[i], strlen (argv[i]))))
						{
							changes_only = TRUE;
							if (opt_char == '+') break_it = TRUE;
						}
						else	/* found beginning with '+' */
						{
							eprintf ("runopts: " RO_ILLOPTST "\n", argv[i]);
							return 2;
						}
						break;

					case 's':
						if ((opt_char == '-') || (!strncmp ("+silent",
							argv[i], strlen (argv[i]))))
						{
							silent = TRUE;
							if (opt_char == '+') break_it = TRUE;
						}
						else	/* found beginning with '+' */
						{
							eprintf ("runopts: " RO_ILLOPTST "\n", argv[i]);
							return 2;
						}
						break;

					case 't':
						if ((opt_char == '-') || (!strncmp ("+tpa-size",
							argv[i], strlen (argv[i]))))
						{
							/* parameter in same string? */
							
							if ((opt_char == '-') && (j < (slen-1)))
							{
								tpa_size = calc_tpa (&argv[i][j+1]);
								break_it = TRUE;
							}
							else
							{
								if (i < (argc - 1))
								{
									if (opt_char == '+') break_it = TRUE;
									tpa_size = calc_tpa (argv[i+1]);
									argv[i+1][0] = 0;
								}
								else
								{
									eprintf ("runopts: " RO_NOTPA "\n");
									return 2;
								}	
																	
							}
						}
						else
						{
							eprintf ("runopts: " RO_ILLOPT "\n");
							return 2;
						}
						break;

					case 'v':
						if ((opt_char == '-') || (!strncmp ("+verbose", argv[i],
							strlen (argv[i]))))
						{
							verbose = TRUE;
							if (opt_char == '+') break_it = TRUE;
						}
						else	/* found beginning with '+' */
						{
							eprintf ("runopts: " RO_ILLOPTST "\n", argv[i]);
							return 2;
						}
						break;

					default:
						eprintf ("runopts: " RO_ILLOPT "\n", argv[i][j]);
						return 2;
				}
			}
			
			argv[i][0] = 0;
		}
	} 

	if ((fastload == -1) && (loadtofast == -1) &&
		(mallocfast == -1) && (tpa_size == -1)) verbose = TRUE;

	if (tpa_size != -1)
	{
		tpa_size -= 128;
		tpa_size /= 128;
	}

	something_done = FALSE;

	for (i = 1; (i < argc) && (!intr()); i++)
	{
		if (argv[i][0])
		{
			something_done = TRUE;
			set_run_flags (argv[i], fastload, loadtofast, mallocfast,
				tpa_size, verbose, changes_only, silent);
		}
	}
	
	if (!something_done)
		return printusage(NULL);
	
	return 0;
}

void setwritemode(const char *filename,int mode)
{
	int attrib;
	
	attrib = Fattrib(filename,GETATTRIB,0);
	if (mode)
		attrib &= ~FA_RDONLY;
	else
		attrib |= FA_RDONLY;
	Fattrib(filename,SETATTRIB,attrib);
}
