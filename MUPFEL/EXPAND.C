/*
 * expand.c  -  wildcard expansion
 * 27.05.90
 */
 
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "attrib.h"
#include "expand.h"
#include "mkargcv.h"
#include "mupfel.h"
#include "wildmat.h"

SCCS(expand);

static int endexpand;
static char **myargv;

int expandall(char *spec,int (*processfile)(char *dir,char *file));

/*
 * int splitname(char *name, char *drive, char **dir, char **file)
 * Purpose:
 *	Split a <name> into drive, directory and filename
 *	parts. If the directory part in name starts with \, <drive> will
 *	contain e.g. "x:\". Elements in the <dir> array are dynamically
 *	allocated.
 * Returns:
 *	Number of elements in dir array.
 */
static int splitname(char *name, char *drive, char **dir, char **file)
{
	int elements = 0, i;
	char *n, *np, *next, *nmdup = strdup(name);
	
	np = nmdup;
	*drive = '\0';
	
	if (np[1]==':')				/* drive spec? */
	{
		i = (np[2]=='\\') ? 3 : 2;	/* yes, root dir? */
		strncpy(drive,np,i);
		drive[i]='\0';
		np += i;					/* first dir spec */
	}
	else
		if (*np=='\\')				/* no drive, root? */
		{
			strcpy(drive,"\\");
			++np;
		}
		
	n = strtok(np,"\\");			/* first dir spec */
	while (n != NULL)				/* more to do ? */
	{
		next = strtok(NULL,"\\");	/* is this the last? */
		if (next != NULL)
			dir[elements++] = strdup(n);	/* no, copy to dir */
		else
			*file=strdup(n);		/* yes, copy to file */
		n = next;
	}
	free(nmdup);

	return elements;
}

/*
 * int pathlength(int elements,char *drive,char **dir,char *file)
 * Purpose:
 *	Calculate length of pathname that is built from <drive>,
 *	<elements> members of <dir> and <file>. Neccessary Backslashes
 *	and the trailing \0 are accounted for.
 * Returns:
 *	Length of complete pathname.
 */
static int pathlength(int elements,char *drive,char **dir,char *file)
{
	int i,len;
	
	len = elements + 1 +(int)strlen(drive) + (int)strlen(file);
	for (i=0; i<elements; ++i)
		len += (int)strlen(dir[i]);
	return len;
}

/*
 * void cleanup(char *str)
 * Purpose:
 *	Remove repeated Backslashes from <str>.
 * Returns:
 *	Nothing.
 */
static void cleanup(char *str)
{
	char *s; 

	s=str;
	while (*s)
	{
		if (*s=='\\' && *(s+1)=='\\')
			memmove(s,s+1,strlen(s));
		++s;
	}
}

/*
 * int expandone(char *spec, char *dirprefix,
 *			  int (*processfile)(char *dir,char *file))
 * Purpose:
 *	Expand the single wildcard specifier in <dirprefix>\<spec>.
 *	Call <processfile> for each match.
 * Returns:
 *	TRUE for at least one match
 *	FALSE for no matches
 */
static int expandone(char *spec, char *dirprefix,
				 int (*processfile)(char *dir,char *file))
{
	DTA dta, *olddta = Fgetdta();
	char *newspec = malloc(strlen(dirprefix)+5);
	int retcode = FALSE;
	int matches = 0;

	Fsetdta(&dta);

	strcpy(newspec,dirprefix);
	if (*newspec!='\0')
		chrapp(newspec,'\\');
	strcat(newspec,"*.*");

	cleanup(newspec);

	if (!Fsfirst(newspec,FA_ATTRIB))
	{
		do
		{
			if (!strcmp(dta.d_fname,".") || !strcmp(dta.d_fname,".."))
				continue;
			if (tstbit(dta.d_attrib,FA_HIDDEN))
				continue;
				
			if (wildmatch(dta.d_fname,spec))
				if (!processfile(dirprefix,dta.d_fname))
				{
					retcode = FALSE;
					break;
				}
				else
					++matches;
		} while (!Fsnext() && !endexpand);
		if (!endexpand && matches>0)
			retcode = TRUE;
	}
	else
		retcode = FALSE;

	Fsetdta(olddta);
	free(newspec);
	return retcode;
}


/*
 * int alldirs(int index,int elements,char *drive,char **dir,char *file,
 *			int (*processfile)(char *dir,char *file))
 * Purpose:
 *	Expand the <index>th element in <dir>, call expand for each match
 * Returns:
 *	TRUE for at least one match
 *	FALSE for no matches
 */		 
static int alldirs(int index,int elements,char *drive,char **dir,
			char *file, int (*processfile)(char *dir,char *file))
{
	DTA dta, *olddta = Fgetdta();
	char *basedir;					/* base part */
	char *wilddir;					/* base + wildcard */
	char *newdir;					/* expanded dir */
	int i, plen, retcode = FALSE;
	
	Fsetdta(&dta);
	
	plen=pathlength(elements,drive,dir,file)+5;
	basedir=malloc(plen);
	wilddir=malloc(plen);
	
	/* build base part */
	strcpy(basedir,drive);
	for (i=0; i<index; ++i)
	{
		strcat(basedir,dir[i]);
		strcat(basedir,"\\");
	}
	
	/* build wildcard dir */
	strcpy(wilddir,basedir);
	strcat(wilddir,"*.*");

	cleanup(wilddir);

	if (!Fsfirst(wilddir,FA_DIREC))
	{
		do
		{
			/*
			 * found one. replace dir[index] with matching name,
			 * call expandall for next recursion level
			 */
			
			if (*dta.d_fname=='.' || !tstbit(dta.d_attrib,FA_DIREC))
				continue;
			
			if (!wildmatch(dta.d_fname,dir[index]))
				continue;

			newdir = malloc(plen+strlen(dta.d_fname));
			strcpy(newdir,basedir);
			strcat(newdir,dta.d_fname);
			for (i=index+1; i<elements; ++i)
			{
				chrcat(newdir,'\\');
				strcat(newdir,dir[i]);
			}
			chrapp(newdir,'\\');
			strcat(newdir,file);
			if (expandall(newdir,processfile))
				retcode = TRUE;
			free(newdir);
		} while (!Fsnext() && !endexpand);
	}
	
	free(basedir);
	free(wilddir);
	Fsetdta(olddta);
	return retcode;
}

/*
 * int expandall(char *spec, int (*processfile)(char *dir,char *file))
 * Purpose:
 *	Expand the wildcard specifier <spec>, call <processfile> for
 *	each match.
 * Returns:
 *	TRUE for at least one match
 *	FALSE for no matches
 */
static int expandall(char *spec, int (*processfile)(char *dir,char *file))
{
	char drive[4], *file, *dir[20];
	int i, elements, wilddir, retcode = FALSE;
	
	elements = splitname(spec,drive,dir,&file);

	if (elements==0)
	{
		retcode = expandone(file,drive,processfile);
		free(file);
		return retcode;
	}

	i = anywild(elements,dir);
	if (i!=-1)
	{
		wilddir = TRUE;
		if (alldirs(i,elements,drive,dir,file,processfile))
			retcode = TRUE;
	}
	else
		wilddir = FALSE;

	if (!wilddir)
	{
		char *dir2;
		
		dir2 = malloc(pathlength(elements,drive,dir,file));
		strcpy(dir2,drive);
		for (i=0; i<elements; ++i)
		{
			strcat(dir2,dir[i]);
			strcat(dir2,"\\");
		}
		if (expandone(file,dir2,processfile))
			retcode = TRUE;
		free(dir2);
	}

	for (i=0; i<elements; ++i)
		free(dir[i]);
	free(file);
	if (endexpand)
		retcode = FALSE;
	return retcode;
}

static int nextfile(char *dir, char *file)
{
	char *mypath = malloc(strlen(dir)+strlen(file)+2);
	int retcode;
	
	strcpy(mypath,dir);
	if (*mypath!='\0')
		chrapp(mypath,'\\');
	strcat(mypath,file);
	strlwr(mypath);
	retcode = argcpy(myargv,mypath);
	if (!retcode)
		endexpand = TRUE;
	free(mypath);
	return retcode;
}

int expand(char **argv, char *filespec)
{
	int retcode;
	
	myargv = argv;
	endexpand = FALSE;
	if (validfile(filespec))
		retcode = expandall(filespec,nextfile);
	else
		retcode = FALSE;
		
	if (!retcode)
		return argcpy(argv,filespec);
	else
		return retcode;
}
