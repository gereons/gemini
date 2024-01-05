/*
	@(#)Mupfel/ls.c
	@(#)Julian F. Reschke, 10. Februar 1991

	06.01.91: `Angleichung' an gnu-ls (jr)
	Optionen umgeworfen, lu, lc, ll gekillt. dir und vdir eingebaut
	22.01.91: Bugfixes, zB sort-by-size
	25.01.91: use unsafe mallocs!
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <tos.h>

#include "alloc.h"
#include "attrib.h"
#include "chario.h"
#include "chmod.h"
#include "curdir.h"
#include "date.h"
#include "environ.h"
#include "getopt.h"
#include "ls.h"
#include "mupfel.h"
#include "parse.h"
#include "strsel.h"
#include "toserr.h"
#include "wildmat.h"

#define NAMESIZE 20		/* hand-crafted to make sizeof(finfo)==32 */

typedef struct finfo
{
	union
	{
		char *ptr;
		char str[NAMESIZE];
	} name;
	uint time;
	uint date;
	long size;
	struct {
		unsigned allocname : 1;
		unsigned expand : 1;
	} flags;
	char attrib;
} finfo;

#define fname(f)	((f)->flags.allocname ? (f)->name.ptr : (f)->name.str)

SCCS(ls);

static int maxlen;

/* Option flags */

/* long_format for lots of info, one per line.
   one_per_line for just names, one per line.
   many_per_line for just names, many per line, sorted vertically.
   horizontal for just names, many per line, sorted horizontally.
   with_commas for just names, many per line, separated by commas.

   -l, -1, -C, -x and -m control this parameter.  */

enum format
{
  long_format,			/* -l */
  one_per_line,			/* -1 */
  many_per_line,		/* -C */
  horizontal,			/* -x */
  with_commas			/* -m */	/* noch nicht implementiert */
};

enum format format;

/* The file characteristic to sort by.  Controlled by -t, -S, and -U. */

enum sort_type
{
  sort_none,			/* -U */
  sort_name,			/* default */
  sort_time,			/* -t */
  sort_size,			/* -S */
  sort_extension		/* -X */
};

enum sort_type sort_type;

/* Direction of sort.
   0 means highest first if numeric,
   lowest first if alphabetic;
   these are the defaults.
   1 means the opposite order in each case.  -r  */

int sort_reverse;

/* Nonzero means mention the size in 512 byte blocks of each file.  -s  */

int print_block_size;

/* Nonzero means show file sizes in kilobytes instead of blocks
   (the size of which is system-dependant).  -k */

int kilobyte_blocks;

/* none means don't mention the type of files.
   all means mention the types of all files.
   not_programs means do so except for executables.

   Controlled by -F and -p.  */

enum indicator_style
{
  none,				/* default */
  all,				/* -F */
  not_programs		/* -p */
};

enum indicator_style indicator_style;


/* Nonzero means when a directory is found, display info on its
   contents.  -R  */

int trace_dirs;

/* Nonzero means when an argument is a directory name, display info
   on it itself.  -d  */

int immediate_dirs;

/* Nonzero means output nongraphic chars in file names as `?'.  -q  */

int qmark_funny_chars;

/* Nonzero means: Merge all information in one long list
   -> Gereon's format */
   
int merged_list;

/* Print directory name above listing */

int print_dir_name;

typedef struct pendir
{
	char *name;
	struct pendir *next;
} PENDIR;

/* Pointer to list of pending dirs */

PENDIR *pending_dirs;


/* The number of digits to use for block sizes.
   4, or more if needed for bigger numbers.  */

int block_size_size;

/* Nonzero means don't omit files whose names start with `.'.  -A */

int all_files;

/* Nonzero means don't omit files `.' and `..'
   This flag implies `all_files'.  -a  */

int really_all_files;

/* A linked list of shell-style globbing patterns.  If a non-argument
   file name matches any of these patterns, it is omitted.
   Controlled by -I.  Multiple -I options accumulate.
   The -B option adds `*~' and `.*~' to this list.  */

struct ignore_pattern
{
  char *pattern;
  struct ignore_pattern *next;
};

struct ignore_pattern *ignore_patterns;

/* The line length to use for breaking lines in many-per-line format.
   Can be set with -w.  */

int line_length;

/* attribut byte for Fsfirst */

int fattrib;


static char *cmdname;	/* points to argv[0] */

static finfo *files;
static long files_index;		/* index into files */
static long nfiles;		/* size of files */
#define FSIZEVAL	100	/* start & increment value for nfiles */

/*
	Discards trailing \\
	cares for . and ..
*/

int
normalized_fsfirst (char *filename, int attr)
{
	int ret;
	char *normalized;

	normalized = normalize (filename);

	if (normalized)
	{	
		if (lastchr (normalized) == '\\')
			normalized[strlen(normalized)-1] = 0;
	}
	
	ret = Fsfirst (normalized ? normalized : filename, attr);
	
	if (normalized) free (normalized);
	return ret;
}

/*
	test for something like a:
*/

static int is_drivespec (char *name)
{
	if (strlen (name) != 2) return FALSE;
	return name[1] == ':';
}

/* care for things like f:\ */

int
might_be_dir (char *filename)
{
	DTA newDTA, *oldDTA = Fgetdta ();
	char *filespec;
	int ret;
	
	if (filename[1] == ':')
		if (!legaldrive (filename[0]))
			return FALSE;

	filespec = umalloc (6 + strlen (filename));
	if (!filespec) return FALSE;

	strcpy (filespec, filename);
	
	if (lastchr (filespec) != '\\') chrapp (filespec, '\\');
	strcat (filespec, "*.*");
	
	Fsetdta (&newDTA);
	ret = normalized_fsfirst (filespec, fattrib);
	Fsetdta (oldDTA);
	
	free (filespec);
	return (ret != -34);	/* -34: illegal dir */
}

/* Return non-zero if `name' doesn't end in `.' or `..'
   This is so we don't try to recurse on `././././. ...' */

int
is_not_dot_or_dotdot (char *name)
{
	char *t;

	if (!name)
	{
		eprintf ("murks in name\n");
		return FALSE;
	}

	t = strrchr (name, '\\');
	if (t) name = t + 1;

	if (name[0] == '.'
		&& (name[1] == '\0'
		|| (name[1] == '.' && name[2] == '\0')))
    return FALSE;

	return TRUE;
}

static int clear_files (void)
{
	files_index = 0;
	nfiles = FSIZEVAL;
	maxlen = 0;
	files = ucalloc (nfiles, sizeof(finfo));

	return (files != NULL);
}

void free_files (void)
{
	long i;

	if (!files) return;

	for (i=0; i<files_index; ++i)
		if (files[i].flags.allocname)
			free(files[i].name.ptr);
			
	free (files);
	files = NULL;	
}

/* Round file size to # of blocks */

static long block_count (long size)
{
	if (kilobyte_blocks)
		return (size + 1023L) / 1024L;
	else
		return (size + 511L) / 512L;
}



/* Add `directory' to the list of pending directories */

static int queue_directory (char *directory)
{
	PENDIR *new;

	new = umalloc (sizeof (PENDIR));

	if (!new)
	{
		eprintf ("%s: " LS_NOMEMD "\n", cmdname, directory);
		return FALSE;
	}
	
	new->name = strdup (directory);

	if (!new->name)
	{
		eprintf ("%s: " LS_NOMEMD "\n", cmdname, directory);
		free (new);
		return FALSE;
	}
	
	new->next = pending_dirs;
	pending_dirs = new;
	return TRUE;
}


/* remove directories from files and add them to the list of pending
   directories */

/* Remove any entries from `files' that are for directories,
   and queue them to be listed as directories instead.
   `dirname' is the prefix to prepend to each dirname
   to make it correct relative to ls's working dir.
   `recursive' is nonzero if we should not treat `.' and `..' as dirs.
   This is desirable when processing directories recursively.  */

void extract_dirs_from_files (char *dirname, int recursive)
{
	long i, j;
	char path[256];	/* enuff' for long paths! */

	if (!files) return;

  /* Queue the directories last one first, because queueing reverses the
     order.  */

	for (i = files_index - 1; i >= 0; i--)
	{
		char *filename = fname(&files[i]);

		if ((files[i].attrib & FA_DIREC) &&
			(!recursive || is_not_dot_or_dotdot (filename)) &&
			files[i].flags.expand)
		{
			if (filename[0] == '\\' || dirname[0] == 0 ||
				!strcmp (dirname, "."))
				queue_directory (filename);
			else
			{
				strcpy (path, dirname);
				if (lastchr (path) != '\\') chrapp (path, '\\');
				strcat (path, filename);
				queue_directory (path);
	  		}
	  	}
	}

	/* Now delete the directories from the table, compacting all the remaining
	entries.  */

	for (i = 0, j = 0; i < files_index; i++)
	{
		if ((files[i].attrib & FA_DIREC) && (files[i].flags.expand))
		{
			if (files[i].flags.allocname)
				free (files[i].name.ptr);		
		}
		else
			files[j++] = files[i];
	}
	
	files_index = j;
}

/* Add `pattern' to the list of patterns for which files that match are
   not listed.  */

static int add_ignore_pattern (char *pattern)
{
	struct ignore_pattern *ignore;
	
	ignore = (struct ignore_pattern *) umalloc (sizeof (struct ignore_pattern));
	if (!ignore) return FALSE;
	
	ignore->pattern = pattern;
	/* Add it to the head of the linked list. */
	ignore->next = ignore_patterns;
	ignore_patterns = ignore;
	return TRUE;
}

void free_ignore_patterns (void)
{
	struct ignore_pattern *i = ignore_patterns;
	
	while (i)
	{
		struct ignore_pattern *nxt = i->next;
		free (i);
		i = nxt;
	}
}

int file_interesting (DTA *dta)
{
	struct ignore_pattern *ignore;

	for (ignore = ignore_patterns; ignore; ignore = ignore->next)
		if (wildmatch (dta->d_fname, ignore->pattern))
			return FALSE;

	if (really_all_files
		|| dta->d_fname[0] != '.'
		|| (all_files
		&& dta->d_fname[1] != '\0'
		&& (dta->d_fname[1] != '.' || dta->d_fname[2] != '\0')))
    return TRUE;

	return FALSE;
}

static int fappend (DTA *d, char *name, int explicit_arg)
{
	finfo *f = &files[files_index];
	int len;

	if (tstbit(d->d_attrib,FA_HIDDEN) && !all_files)
		return TRUE;
		
	f->time = d->d_time;
	f->date = d->d_date;
	f->size = d->d_length;
	f->attrib = d->d_attrib;
	f->flags.expand = (explicit_arg && !immediate_dirs) ? TRUE : FALSE;

	len = (int)strlen(name);
	if (len < NAMESIZE)
	{
		strcpy (f->name.str, name);
		f->flags.allocname = FALSE;
	}
	else
	{
		f->name.ptr = malloc (len+1);
		strcpy (f->name.ptr, name);
		f->flags.allocname = TRUE;
	}

	strlwr (fname (f));

	if (len > maxlen)
		maxlen = len;

	++files_index;
	if (files_index >= nfiles)
	{
		finfo *newfiles;

		nfiles += FSIZEVAL;
		newfiles = ucalloc (nfiles, sizeof (finfo));
		
		if (!newfiles)
		{
			free_files ();
		}
		else	
		{
			memcpy (newfiles, files, (nfiles - FSIZEVAL) * sizeof(finfo));
			free (files);
			files = newfiles;
		} 
	}
	return (files != NULL);
}

/* called if qmark_funny_chars == TRUE */

static void conv_filename (char *name)
{
	char *c = name;
	
	while (*c)
		if (*c & 0xe0)
			c++;
		else
			*c++ = '?';
}


static int showfile (finfo *f, int trailing)
{
	int l = 0;
	dosdate fd;
	dostime ft;

	if (print_block_size)
	{
		if (trailing)
			l += mprintf ("%*ld ", block_size_size, block_count (f->size));
		else
			l += mprintf ("%ld ", block_count (f->size));
	}
		
	if (format == long_format)
	{
		int year;
		
		fd.d = f->date;
		ft.t = f->time;
		year = fd.s.year + 80;

		mprintf("%c%c%c%c%c%9ld %02d.%02d.%02d %02d:%02d:%02d ",
			tstbit(f->attrib,FA_DIREC)  ? 'd' :
			(tstbit(f->attrib,FA_VOLUME) ? 'v' : '-'),
			tstbit(f->attrib,FA_RDONLY) ? 'r' : '-',
			tstbit(f->attrib,FA_HIDDEN) ? 'h' : '-',
			tstbit(f->attrib,FA_SYSTEM) ? 's' : '-',
			tstbit(f->attrib,FA_ARCH)   ? 'a' : '-',
			f->size,
			fd.s.day, fd.s.month, year % 100,
			ft.s.hour, ft.s.min, ft.s.sec*2);
	}

	if (qmark_funny_chars)
		conv_filename (strlwr (fname(f)));

	l += mprintf ("%s", strlwr (fname(f)));
		
	switch (indicator_style)
	{
		case all:
			if (!tstbit (f->attrib, FA_DIREC) &&
				!tstbit (f->attrib, FA_VOLUME) &&
				extinsuffix (fname(f)))
				l += mprintf("*");
			else
				if (tstbit (f->attrib, FA_VOLUME))
					l+= mprintf ("%%");				

			/* fall thru!!! */

		case not_programs:
			if (tstbit (f->attrib, FA_DIREC))
				l += mprintf("\\");
	}

	/* Argh! Filenames with leading blanks have no length!!! */
	return intr() ? 0 : l+1;
}

static int outcols(void)
{
	finfo *h = &files[0];
	finfo *h1;
	finfo *last = &files[files_index];
	int numcols, i, cols, nelem, height, leftcols, cnt;
	int l;

	cnt = 0;
	nelem = (int)files_index;
	maxlen += 2;	/* wg. Spaltenabstand */

	if (maxlen > line_length)
		line_length = maxlen+1;
		
	numcols = line_length / maxlen;
	
	height = nelem / numcols;
	leftcols = nelem % numcols;
	
	h1 = h;
	do
	{
		if (cnt >= (height*numcols))
			break;
			
		for (cols=0; cols<numcols && !intr(); ++cols)
		{
			l = showfile (h1, TRUE)-1;

			if (cols != numcols-1) rawoutn(' ', maxlen-l);
			++cnt;

			if (format == many_per_line)
				++h1;
			else
			{
				for (i=0; i<height && h1<last; ++i)
					++h1;
				if (cols<leftcols && h1<last)
					++h1;
			}
		}
		crlf();
		
		if (format == horizontal)
		{
			++h;
			h1 = h;
		}
	} while (cnt < (height*numcols) && !intr());

	while (cnt < nelem && !intr())
	{
		l=showfile (h1, TRUE)-1;
		rawoutn(' ',maxlen-l);
		
		if (format == many_per_line)
			++h1;
		else
		{
			for (i=0; i<height+1 && h1<last; ++i)
				++h1;
		}
		++cnt;
	}
	if (cnt % numcols != 0)
		crlf();

	return !intr();
}

static int outcommas (void)
{
	int i;
	int line_count;
	
	maxlen += 2;
	line_count = 0;
	
	for (i=0; i<files_index && !intr(); ++i)
	{
		int lc = showfile (&files[i], FALSE);
		
		if (!lc) return FALSE;

		line_count += lc;
		
		if (i != (files_index - 1))
			line_count += mprintf (", ");
		else
			crlf ();
		
		if (line_count + maxlen > line_length)
		{
			crlf ();
			line_count = 0;
		}		
	}
	return !intr();
}

static int sortreturn(int cmp)
{
	return sort_reverse ? -cmp : cmp;
}

static int namecmp(const finfo *f1, const finfo *f2)
{
	return sortreturn(strcmp(fname(f1),fname(f2)));
}

static int extcmp (const finfo *f1, const finfo *f2)
{
	char *ext1, *ext2;
	
	ext1 = strrchr (fname (f1), '.');
	if (!ext1)
		ext1 = (f1->attrib & (FA_DIREC|FA_VOLUME)) ? "\001" : "\002";
	
	ext2 = strrchr (fname (f2), '.');
	if (!ext2)
		ext2 = (f2->attrib & (FA_DIREC|FA_VOLUME)) ? "\001" : "\002";

	if (!strcmp (ext1, ext2)) return namecmp (f1, f2);

	return sortreturn (strcmp (ext1, ext2));
}

static int sign(long x)
{
	if (x<0)
		return -1;
	if (x>0)
		return 1;
	return 0;
}

static int sizecmp(const finfo *f1, const finfo *f2)
{
	if (f1->size == f2->size)
		return namecmp(f1,f2);
	else
		return sortreturn(sign(f2->size - f1->size));
}

static int timecmp(const finfo *f1, const finfo *f2)
{
	if (f1->date == f2->date)
	{
		if (f1->time == f2->time)
			return namecmp(f1,f2);
		else
			return sortreturn(f2->time - f1->time);
	}
	else
		return sortreturn(f2->date - f1->date);
}

/* Add all files in directory dirname to files, with full name
   returns 0 for success or GEMDOS error code */

static int gobble_dir (char *dirname)
{
	DTA dta, *olddta = Fgetdta();
	int stat, only_point = 0;
	char *filespec;

	filespec = umalloc (strlen(dirname) + 5);
	
	if (!filespec)
	{
		eprintf ("%s: " LS_NOMEMD "\n", cmdname, dirname);
		return ENSMEM;
	}
	
	Fsetdta (&dta);

	if (!strcmp (dirname, "."))	/* nur der Punkt? */
	{
		*filespec = 0;
		only_point = TRUE;
	}
	else
	{
		strcpy (filespec, dirname);
		if (!is_drivespec (filespec))
			chrapp (filespec, '\\');
	}
	strcat (filespec, "*.*");
	
  /* Read the directory entries, and insert the subfiles into the `files'
     table.  */

	stat = Fsfirst (filespec, fattrib);
	free (filespec);

	if (stat) return stat;
	
	while (!stat)
	{
    	char *newname;
    	
    	newname = umalloc (15 + strlen (dirname));

    	if (newname)
    	{
    		if (only_point)
    			*newname = 0;
    		else
    		{
	    		strcpy (newname, dirname);
    			if (lastchr (newname) != '\\') chrapp (newname, '\\');
	   		}
    		strcat (newname, dta.d_fname);
    		
	    	if (file_interesting (&dta))
	    		if (!fappend (&dta, newname, trace_dirs))
	    		{
	    			eprintf ("%s: " LS_NOMEMD "\n", cmdname, dirname);
					free (newname);
	    			return ENSMEM;
	    		}

			free (newname);
		}
		else
		{
			eprintf ("%s: " LS_NOMEMD "\n", cmdname, dirname);
   			return ENSMEM;
		}

		stat = Fsnext ();
	}

	Fsetdta (olddta);

	return 0;	/* ok */
}


/* Add a file to the current table of files.
   Verify that the file exists, and print an error message if it does not.
   Return the number of blocks that the file occupies.  */

static int gobble_file (char *filename, int explicit_arg)
{
	DTA dta, *olddta = Fgetdta();
	int fstat;
	int ret = 0;
		
	Fsetdta (&dta);

	if (validfile(filename))
	{
		fstat = normalized_fsfirst (filename, fattrib);
		if (bioserror(fstat))
			return ioerror (cmdname, filename, olddta, fstat);
	}
	else
		fstat = 1;
		
	if (!fstat)	/* Fsfirst ok? */
	{
		if ((dta.d_attrib & FA_DIREC) && merged_list)
			gobble_dir (filename);	
		else
		{
			if (!fappend (&dta, filename, explicit_arg))
			{
				eprintf ("%s: " LS_NOMEM "\n", cmdname);
				return ENSMEM;
			}
		}
	}
	else
	{
		if (might_be_dir (filename) && !immediate_dirs)
		{
			if (merged_list)
				gobble_dir (filename);
			else
				queue_directory (filename);
		}
		else
		{
			eprintf("%s: " LS_NOFILE "\n", cmdname, filename);
			ret = -1;
		}
	}
	
	Fsetdta (olddta);
	return ret;
}

static int print_current_files (void)
{
	int ret = TRUE;

	if (!files) return FALSE;

	if (files_index != 0)
	{
		long maxsize = 0L;
		long i;

		for (i = 0; i < files_index; i++)
		{
			long merk;
			
			merk = block_count (files[i].size);
			if (merk > maxsize) maxsize = merk;
		}
		
		{
			char tmp[10];
			
			block_size_size = sprintf (tmp, "%ld", maxsize);
		}
			
		if (print_block_size)
			maxlen += block_size_size + 2;

		switch (format)
		{
			case many_per_line:
			case horizontal:
				ret = outcols();
				break;
				
			case with_commas:
				ret = outcommas();
				break;

			default:
				for (i=0; i<files_index && !intr(); ++i)
				{
					showfile (&files[i], TRUE);
					crlf ();
				}
				ret = !intr();
				break;
		}
	}
	return ret;
}

/* Sort the files now in the table.  */

void sort_files (void)
{
	int (*sortfunc)(const finfo *f1, const finfo *f2);

	if (!files) return;

	switch (sort_type)
    {
    	case sort_none:
      		return;
		case sort_time:
			sortfunc = timecmp;
			break;			
		case sort_name:
			sortfunc = namecmp;
			break;
		case sort_size:
			sortfunc = sizecmp;
			break;
		case sort_extension:
			sortfunc = extcmp;
			break;
    }
	qsort (files, files_index, sizeof (finfo), sortfunc);
}

/* Read directory `name', and list the files in it. */

static int print_dir (char *name)
{
	long total_blocks = 0L;
	DTA dta, *olddta = Fgetdta();
	int stat, retcode = 1;
	char *filespec;

	filespec = umalloc (strlen(name) + 5);

	if (!filespec)
	{
		eprintf ("%s: " LS_NOMEMD "\n", cmdname, name);
		return FALSE;
	}
	
	Fsetdta (&dta);

	if (!strcmp (name, "."))	/* nur der Punkt? */
		*filespec = 0;
	else
	{
		strcpy (filespec, name);
		if (!is_drivespec (filespec))
			chrapp (filespec, '\\');
	}
	strcat (filespec, "*.*");
	
  /* Read the directory entries, and insert the subfiles into the `files'
     table.  */

	if (!clear_files ())
	{
		eprintf ("%s: " LS_NOMEMD "\n", cmdname, name);
		return FALSE;
	}

	stat = Fsfirst (filespec, fattrib);
	free (filespec);

	if ((stat != -33) && stat)	/* Not File not found or ok */
	{
		free_files ();
		return FALSE;
	}
	
	while (!stat)
	{
	    if (file_interesting (&dta))
	    {
	    	if (!fappend (&dta, dta.d_fname, trace_dirs))
	    	{
	    		eprintf ("%s: " LS_NOMEMD "\n", cmdname, dta.d_fname);
	    		return FALSE;
	    	}
	    	
	    	total_blocks += block_count (dta.d_length);
		}
		stat = Fsnext ();
	}

	Fsetdta (olddta);

	/* Sort the directory contents.  */
	sort_files ();

	/* If any member files are subdirectories, perhaps they should have their
	contents listed rather than being mentioned here as files.  */

	if (trace_dirs)
		extract_dirs_from_files (name, TRUE);

	if (print_dir_name)
		mprintf ("%s:\n", name);

	if (format == long_format || print_block_size)
		printf ("total %ld\n", total_blocks);

	if (files_index)
		retcode = print_current_files ();

	if (pending_dirs)
		crlf ();
	
	free_files ();

	return retcode;
}




int
m_ls (int argc, char **argv)
{
	GETOPTINFO G;
	int c, retcode = 0;
	int dir_defaulted = TRUE;
	
	struct option long_options[] =
	{
		{"all",				0, NULL, 'a'},
		{"directory",		0, NULL, 'd'},
		{"kilobytes",		0, NULL, 'k'},
		{"long-format",		0, NULL, 'l'},
		{"comma-format",	0, NULL, 'm'},
		{"append-backslash",0, NULL, 'p'},
		{"hide-control-chars",	0, NULL, 'q'},
		{"reverse",			0, NULL, 'r'},
		{"size",			0, NULL, 's'},
		{"time-sort",		0, NULL, 't'},
		{"horizontal-format",	0, NULL, 'x'},
		{"almost-all",		0, NULL, 'A'},
		{"ignore-backups",	0, NULL, 'B'},
		{"vertical-format",	0, NULL, 'C'},
		{"classify",		0, NULL, 'F'},
		{"merged-list",		0, NULL, 'M'},
		{"recursive",		0, NULL, 'R'},
		{"size-sort",		0, NULL, 'S'},
		{"no-sort",			0, NULL, 'U'},
		{"extension-sort",	0, NULL, 'X'},
		{"single-column",	0, NULL, '1'},
		{"width",			1, NULL, 'w'},
		{"ignore",			1, NULL, 'I'},
		{NULL, 0,0,0},
	};
	int opt_index = 0;

	cmdname = argv[0];
	
	print_dir_name = 1;
	trace_dirs = FALSE;
	immediate_dirs = FALSE;
	all_files = really_all_files = FALSE;
	print_block_size = kilobyte_blocks = FALSE;
	indicator_style = none;
	ignore_patterns = NULL;

	if (getenv("COLUMNS"))
		line_length = atoi(getenv("COLUMNS"));
	else
		line_length = 80;

	format = isatty (1) ? horizontal : one_per_line;
	
	sort_type = sort_name;
	sort_reverse = FALSE;
	fattrib = FA_ATTRIB;
	merged_list = FALSE;
	pending_dirs = NULL;
	qmark_funny_chars = FALSE;
	
	strlwr(argv[0]);
	
	STRSELECT(argv[0])
	WHEN("dir")
		format = horizontal;
	WHEN("ls")
		qmark_funny_chars = isatty (1);
	WHEN("vdir")
		format = long_format;
	ENDSEL

	optinit (&G);

	while ((c = getopt_long (&G, argc, argv, "adklmpqrstxABCFMRSUX1w:I:",
		long_options, &opt_index)) != EOF)
	{
		if (!c)			c = long_options[G.option_index].val;
		switch (c)
		{
			case 0:
				break;
			case 'a':
				all_files = really_all_files = TRUE;
				fattrib |= FA_VOLUME;	/* include volume labels */
				break;
			case 'd':
				immediate_dirs = TRUE;
				break;
			case 'k':
				kilobyte_blocks = TRUE;
				break;
			case 'l':
				format = long_format;
				break;
			case 'm':
				format = with_commas;
				break;
			case 'p':
				indicator_style = not_programs;
				break;
			case 'q':
				qmark_funny_chars = TRUE;
				break;
			case 'r':
				sort_reverse = TRUE;
				break;
			case 's':
				print_block_size = TRUE;
				break;
			case 't':
				sort_type = sort_time;
				break;
			case 'w':
				line_length = atoi (G.optarg);
				if (line_length < 1)
				{
					eprintf ("%s: "LS_WRONGW"\n", cmdname);
					return 1;
				}
				break;
			case 'x':
				format = many_per_line;
				break;
			case 'A':
				all_files = TRUE;
				break;
			case 'B':
				add_ignore_pattern ("*.DUP");
				add_ignore_pattern ("*.BAK");
				add_ignore_pattern ("*~");
				break;
			case 'C':
				format = horizontal;
				break;
			case 'F':
				indicator_style = all;
				break;
			case 'I':
				add_ignore_pattern (G.optarg);
				break;
			case 'M':
				merged_list = TRUE;
				break;
			case 'R':
				trace_dirs = TRUE;
				break;
			case 'S':
				sort_type = sort_size;
				break;
			case 'U':
				sort_type = sort_none;
				break;
			case 'X':
				sort_type = sort_extension;
				break;
			case '1':
				format = one_per_line;
				break;
			default:
				return printusage(long_options);
		}
	}
	
	if (!clear_files ())
	{
		eprintf ("%s: " LS_NOMEM "\n", cmdname);
		return 2;
	}

	if (G.optind != argc)
	{
		dir_defaulted = FALSE;

		for (; G.optind < argc; ++G.optind)
			gobble_file (argv[G.optind], TRUE);
	}
	
	if (dir_defaulted)
	{
		if (immediate_dirs)
			gobble_file (".", TRUE);
		else
			queue_directory (".");
    }

	if (merged_list)	/* loop until no more dirs */
	{
		extract_dirs_from_files ("", TRUE);
		
		while (pending_dirs)
		{
			PENDIR *thispend;		

			thispend = pending_dirs;
			pending_dirs = pending_dirs->next;
		
			if (!retcode)
				if (gobble_dir (thispend->name))
					retcode = 2;

			free (thispend->name);
			free (thispend);	
			extract_dirs_from_files ("", TRUE);
		}
	
		sort_files ();
	}


	if (files_index)
	{
		sort_files ();
		if (!immediate_dirs)
			extract_dirs_from_files ("", FALSE);
      /* `files_index' might be zero now.  */
    }

	if (files_index)
	{
		print_current_files ();
		if (pending_dirs)
			crlf ();
    }
	else
		if (pending_dirs && !pending_dirs->next)
			print_dir_name = FALSE;

	free_files ();

	while (pending_dirs)
	{
		PENDIR *thispend;		

		thispend = pending_dirs;
		pending_dirs = pending_dirs->next;
		
		if (!retcode)
			if (!print_dir (thispend->name))
				retcode = 2;

		free (thispend->name);
		free (thispend);	
		print_dir_name = TRUE;
	}
	
	free_ignore_patterns ();
	return retcode;
}
