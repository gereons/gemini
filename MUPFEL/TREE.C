/*
 * tree.c  -  internal "tree", "du" and "find" commands,
 * support for "rm -r", "cp -r" and "hash -d"
 * 26.08.90
 *
 * 13.02.91 du neu (in du.c), find eliminiert
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "attrib.h"
#include "chario.h"
#include "cpmv.h"
#include "curdir.h"
#include "df.h"
#include "getopt.h"
#include "hash.h"
#include "mkdir.h"
#include "mupfel.h"
#include "parse.h"
#include "rm.h"
#include "tree.h"
#include "wildmat.h"

SCCS(tree);

static enum treetype {TREE, DU, RM, CP, FIND, HASH};
static int indent, dironly;

#ifdef gereonsdu
static int filecount, dircount;
static long totalsize;
#endif

static char *destdir, *srcdir, newdir[100];

static int dummy;

#define dtadummy(d)		dummy=d->d_reserved[0]

/*
 * Recursively walk the file hierarchy starting at path. Call
 * ffunc for each file matching pattern, dfunc for each subdirectory
 */
static int dotree(int type,char *path,char *pattern,int depth,
	int (*ffunc)(char *path,char *file,DTA *dta),
	int (*dfunc)(char *path,char *file,DTA *dta))
{
	DTA dta;
	char name[300];
	int retcode;
	
	Fsetdta(&dta);
	sprintf(name,"%s*.*",path);

	/* Look for files matching "pattern" */
	if (!dironly)
	{
		if (!Fsfirst(name,FA_FILES))
		{
			do
			{
				if (!wildmatch(dta.d_fname,pattern))
					continue;
				if (indent)
					rawoutn(' ',depth);
				if (intr() || !ffunc(path,dta.d_fname,&dta))
					return FALSE;
			} while (!Fsnext());
		}
	}
	
	/* Look for directories matching "pattern" */
	sprintf(name,"%s*.*",path);
	if (!Fsfirst(name,FA_DIREC))
	{
		do
		{
			if (!tstbit(dta.d_attrib,FA_DIREC))
				continue;
			/* Ignore "." and ".." */
			if (!strcmp(dta.d_fname,".") || !strcmp(dta.d_fname,".."))
				continue;
			if (type != RM && type != HASH)
			{
				if (indent)
					rawoutn(' ',depth);
				if (intr() || !dfunc(path,dta.d_fname,&dta))
					return FALSE;
			}
			sprintf(name,"%s%s\\",path,dta.d_fname);
			retcode = dotree(type,name,pattern,depth+4,ffunc,dfunc);
			Fsetdta(&dta);
			if (!retcode)
				return FALSE;
		} while (!Fsnext());
		if (type == RM)
			if (!dfunc(path,"",&dta))
				return FALSE;
		if (intr())
			return FALSE;
	}
	return TRUE;
}

/*
 * functions for "cp -r" 
 */

/*
 * static int cpfile(char *path, char *file)
 * ffunc for cptree ("cp -r" command)
 */
static int cpfile(char *path, char *file,DTA *dta)
{
	int retcode;
	char *fromfile = malloc(strlen(path)+strlen(file)+1);
	char *tofile = malloc(strlen(destdir)+strlen(newdir)+strlen(file)+2);

	setcpdta(dta);
	strcpy(fromfile,path);
	strcat(fromfile,file);
	
	strcpy(tofile,destdir);
	if (*newdir != '\0')
	{
		strcat(tofile,newdir);
		chrcat(tofile,'\\');
	}
	strcat(tofile,file);
	
	retcode = !cp(fromfile,tofile);
	
	free(tofile);
	free(fromfile);
	return retcode;
}

/*
 * static int cpdir(char *path, char *file)
 * dfunc for cptree ("cp -r" command)
 */
static int cpdir(char *path, char *file,DTA *dta)
{
	int retcode;
	char *dir;
	
	dtadummy(dta);
	dir = malloc(strlen(destdir)+strlen(path)+strlen(file)+3);

	strcpy(newdir,&path[strlen(srcdir)]);
	strcat(newdir,file);

	strcpy(dir,destdir);
	chrapp(dir,'\\');
	strcat(dir,&path[strlen(srcdir)]);
	chrapp(dir,'\\');
	strcat(dir,file);

	if (!isdir(dir))
	{
		if (cpverbose())
			mprintf("mkdir %s...",strlwr(dir));
	
		retcode = mkdir(dir);
		
		if (retcode && cpverbose())
			mprintf(TR_DONE "\n");
	}
	else
		retcode = TRUE;
	
	free(dir);
	return retcode;
}

/* front end for "cp -r" */
int cptree(char *fromdir, char *todir)
{
	int retcode;
	
	if (!isdir(todir))
	{
		if (cpverbose())
			mprintf("mkdir %s ...",strlwr(todir));
		retcode = mkdir(todir);
		if (retcode && cpverbose())
			mprintf(TR_DONE "\n");
		if (!retcode)
			return !retcode;
	}
		
	indent = dironly = FALSE;

	srcdir = malloc(strlen(fromdir)+2);
	strcpy(srcdir,fromdir);
	chrapp(srcdir,'\\');
	
	destdir = malloc(strlen(todir)+2);
	strcpy(destdir,todir);
	chrapp(destdir,'\\');
	
	*newdir = '\0';
	
	retcode = dotree(CP,srcdir,"*",0,cpfile,cpdir);
	
	free(destdir);
	free(srcdir);
	return !retcode;
}

/*
 * functions for "rm -r"
 */
 
/*
 * static int rmfile(char *path, char *file)
 * ffunc for rmtree ("rm -r" command)
 */
static int rmfile(char *path, char *file,DTA *dta)
{
	int retcode;
	char *f = malloc(strlen(path)+strlen(file)+1);

	dtadummy(dta);
	strcpy(f,path);
	strcat(f,file);
	retcode = !remove(f);
	free(f);
	return retcode;
}

/*
 * static int rmdirec(char *path, char *file)
 * dfunc for rmtree ("rm -r" command)
 */
static int rmdirec(char *path, char *file,DTA *dta)
{
	int lastp = (int)strlen(path)-1;
	
	dtadummy(dta);
	if (path[lastp] == '\\')
		path[lastp] = '\0';
	lastp = *file;
	return !rmdir(path);
}

/* front end for "rm -r" */
int rmtree(const char *path)
{
	char *mypath = malloc(strlen(path)+2);
	int retcode;
	
	strcpy(mypath,path);
	chrapp(mypath,'\\');
	indent = dironly = FALSE;
	retcode = dotree(RM,mypath,"*",0,rmfile,rmdirec);
	free(mypath);
	return !retcode;
}

#ifdef gereonsdu
/*
 * functions for "du" command
 */
 
/*
 * static int infofile(char *path, char *file)
 * ffunc and dfunc for infotree ("du" command)
 */
static int infofile(char *path, char *file,DTA *dta)
{
	char *f = malloc(strlen(path)+strlen(file)+1);
	
	strcpy(f,path);
	strcat(f,file);
	totalsize += dta->d_length;
	if (tstbit(dta->d_attrib,FA_DIREC))
		++dircount;
	else
		++filecount;
	free(f);
	return TRUE;
}

static void showinfo(char *dir)
{
	mprintf(TR_INFO "\n",
		strlwr(dir),totalsize,filecount,dircount);
}

/* front end for "du" */
static int infotree(char *path, char *pattern)
{
	int retcode;
	char *mypath = malloc(strlen(path)+2);
	
	indent = dironly = FALSE;
	totalsize = dircount = 0;
	filecount = -1;
	strcpy(mypath,path);
	if (isdir(mypath))
	{
		filecount = 0;
		chrapp(mypath,'\\');
		retcode = dotree(DU,mypath,pattern,0,infofile,infofile);
	}
	else
		retcode = FALSE;
	free(mypath);
	return retcode;
}

#endif

/*
 * functions for "tree" command
 */

/*
 * static int showfile(char *path, char *file)
 * ffunc and dfunc for showtree ("tree" command)
 */
static int showfile(char *path, char *file, DTA *dta)
{
	char *p = strdup(path);
	char *f = strdup(file);

	dtadummy(dta);	
	mprintf("%s%s\n",strlwr(p),strlwr(f));
	free(f);
	free(p);
	return TRUE;
}

int m_tree(ARGCV)
{
	GETOPTINFO G;
	DTA *olddta = Fgetdta();
	char *pattern, path[100];
	int c, retcode, pflag;
	struct option long_option[] =
	{
		{ "dironly", FALSE, &dironly, TRUE },
		{ "indent", FALSE, &indent, TRUE },
		{ "path", TRUE, NULL, 0 },
		{ "filespec", TRUE, NULL, 0 },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;
	
	indent = dironly = pflag = FALSE;
	pattern = NULL;
	strcpy(path,".");
	
	optinit (&G);

	while ((c = getopt_long (&G, argc, argv, "dip:f:", long_option,
		&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				if (G.optarg)
					switch(opt_index)
					{
						case 2: goto path;
						case 3: goto filespec;
					}
				break;
			case 'i':
				indent = TRUE;
				break;
			case 'd':
				dironly = TRUE;
				break;
			case 'p':
			path:
				strcpy (path, G.optarg);
				pflag = TRUE;
				break;
			case 'f':
			filespec:
				pattern = strdup (G.optarg);
				break;
			default:
				return printusage(long_option);
		}
	if (G.optind < argc && !pflag)
		strcpy (path, argv[G.optind]);
	chrapp(path,'\\');
	if (pattern == NULL)
		pattern = strdup("*");
	retcode = dotree(TREE,path,pattern,0,showfile,showfile);
	free(pattern);
	Fsetdta(olddta);
	return !retcode;
}

/*
 * functions for "hash -d"
 */

static int hashfile(char *path, char *file, DTA *dta)
{
	char *mypath = malloc(strlen(path)+strlen(file)+1);
	
	dtadummy(dta);
	strcpy(mypath,path);
	strcat(mypath,file);
	strlwr(mypath);
	
	if (extinsuffix(mypath))
	{
		char *dot, *cmd = strdup(file);

		strlwr(cmd);
		dot = strchr(cmd,'.');
		if (dot)
			*dot = '\0';
		enterhash(cmd,mypath,0);
		free(cmd);
	}
	free(mypath);
	return TRUE;
}

int hashtree(const char *path)
{
	int retcode;
	char *mypath;

	mypath = normalize(path);
	if (mypath == NULL)
	{
		eprintf("hash: " TR_HASH "\n",path);
		return 1;
	}
	chrapp(mypath,'\\');
	indent = dironly = FALSE;
	retcode = dotree(HASH,mypath,"*",0,hashfile,hashfile);
	free(mypath);
	return retcode;
}
