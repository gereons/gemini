/*
 * misc.c  -  assorted junk
 * 17.11.90
 */
 
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <tos.h>

#include "alloc.h"
#include "attrib.h"
#include "commands.h"
#include "chario.h"
#include "curdir.h"
#include "getopt.h"
#include "handle.h"
#include "mupfel.h"
#include "parse.h"
#include "toserr.h"

#define X_MAGIC	((int)0x601A)
#define DEBUG 0

SCCS(misc);

static int (*prntf)(char *fmt,...) = eprintf;

/*
 * access(char *file, int mode)
 * determine accessibility of file according to mode
 * mode values:	check performed:
 *	A_EXITS		could be opened for reading
 *	A_READ		dto
 *	A_RDONLY		exists, is read-only
 *	A_WRITE		could be opened for writing
 *	A_RDWR		dto
 *	A_EXEC		exists, and is a TOS binary
 *				(magic number X_MAGIC in 1st 2 bytes == X_MAGIC)
 */
int access(const char *file, int mode)
{
	int hnd = ILLHND, retcode, magic, found, readonly;
	DTA dta, *olddta;
	
	if (!validfile(file))
		return FALSE;
		
	bioserr = 0;

	olddta = Fgetdta();
	Fsetdta(&dta);
	found = Fsfirst(file,FA_FILES);
	Fsetdta(olddta);

	if (bioserror(found))
		bioserr = found;
	if (found != 0 || dta.d_attrib & (FA_DIREC|FA_LABEL))
		return FALSE;

	readonly = dta.d_attrib & FA_RDONLY;
	
	switch (mode)
	{
		case A_EXIST:
		case A_READ:
			retcode = TRUE;
			break;
		case A_WRITE:
		case A_RDWR:
			retcode = !readonly;
			break;
		case A_RDONLY:
			retcode = readonly;
			break;
		case A_EXEC:
			if ((hnd=Fopen(file,0))>=MINHND)
			{
				if (Fread(hnd,sizeof(magic),&magic)!=sizeof(magic))
					magic = 0;
				retcode = (magic == X_MAGIC);
			}
			else
			{
				retcode = FALSE;
				if (bioserror(hnd))
					bioserr = hnd;
			}
			if (hnd >= MINHND)
				Fclose(hnd);
			break;
		default:
			retcode=FALSE;
			break;
	}
	return retcode;
}

/*
 * chrcat(char *str, char c)
 * append character c to str
 */
void chrcat(char *str,char c)
{
	int l=(int)strlen(str);
	
	str[l++]=c;
	str[l]='\0';
}

/*
 * strpos(const char *str, char ch)
 * return position from left of character ch in str, -1 if not found
 */
int strpos(const char *str,char ch)
{
	int i;

	for (i=0; i<(int)strlen(str); ++i)
		if (str[i]==ch)
			return i;
	return -1;
}

/*
 * strrpos(char *str, char ch)
 * return position from right of character ch in str, -1 if not found
 */
int strrpos(const char *str, char ch)
{
	int i;
	
	for (i=(int)strlen(str); i>=0; --i)
		if (str[i] == ch)
			return i;
	return -1;
}

/*
 * isdir(const char *dir)
 * check if dir is a directory.
 * save current dir, try to chdir to dir, chdir back
 */
int isdir(const char *directory)
{
	int oldbioserr;
	int dirok, dirlen, drvok, drvchange = FALSE;
	char drv, dir[200];

	strcpy(dir,directory);
	dirlen = (int)strlen(dir);
	if (!(dirlen==1 || (dirlen==3 && dir[1]==':')))
		if (lastchr(dir)=='\\')
			dir[dirlen-1] = '\0';
		
	if (!validfile(dir))
		return FALSE;
		
	bioserr = 0;	
	savedir();
	if (dir[1]==':' && (drv=toupper(*dir))!=getdrv())
	{
		drvok = chdrv(drv);
		if (bioserr)
		{
			oldbioserr = bioserr;
			restoredir();
			bioserr = oldbioserr;
			return FALSE;
		}
		else
		{
			if (drvok)
			{
				savedir();
				drvchange=TRUE;
			}
			else
			{
				restoredir();
				return FALSE;
			}
		}
	}
	dirok = chdir(dir);
	oldbioserr = bioserr;
	if (drvchange)
		restoredir();
	restoredir();
	bioserr = oldbioserr;
	return dirok;
}

/*
 * isdrive(const char *drv)
 * Determine if drv is a valid drive specifier (eg "A:")
 */
int isdrive(const char *drv)
{
	return strlen(drv)==2 && drv[1]==':' && legaldrive(*drv);
}

/*
 * validfile(const char *name)
 * check if name (filname or wildcard) contains illegal elements:
 *   - illegal drive specifiers
 *   - double backslashes
 */
int validfile(const char *name)
{
	if (name[1] == ':' && !legaldrive(*name))
		return FALSE;

	if (strstr(name,"\\\\"))
		return FALSE;
	
	return TRUE;
}

/*
 * lastchr(const char *str)
 * return last character in string
 */
char lastchr(const char *str)
{
	return str[strlen(str)-1];
}

/*
 * strfnm(char *path)
 * return pointer to filename part of path
 */
char *strfnm(char *path)
{
	char *s;
	
	if ((s=strrchr(path,'\\'))!=NULL)
		return s+1;
	else
		if (path[1]==':')
			return &path[2];
		else
			return path;
}

/*
 * strdnm(const char *path)
 * return pointer to allocated copy of directory part of path
 */
char *strdnm(const char *path)
{
	char *new = strdup(path);
	int backslash;

	backslash = strrpos(new,'\\');	/* find last \ */
	if (backslash > -1)
	{
		/* if 1st char or root dir on drive, increment */
		if (backslash==0 || (backslash==2 && new[1]==':'))
			++backslash;
		/* terminate string after dir name */
		new[backslash]='\0';
	}
	else
		*new='\0';
	return new;
}

/*
 * strdup(char *str)
 * return pointer to a newly allocated copy of str
 */
char *strdup(const char *str)
{
	char *dup = malloc(strlen(str)+1);
	
	if (dup!=NULL)
		strcpy(dup,str);
	return dup;
}

/*
 * newstr(char **dest, char *src)
 * free dest if allocated, strdup src to it
 */
char *newstr(char **dest, const char *src)
{
	if (*dest!=NULL)
		free(*dest);
	return *dest=strdup(src);
}

/*
 * isatty(int handle)
 * return status of the device/file associated with handle
 * TRUE	for devices (CON: PRN: AUX:)
 * FALSE	for file handles
 */
int isatty(int handle)
{
	long oldoffset, rc;
	
	oldoffset = Fseek(0L,handle,1);
	rc = Fseek(1L,handle,0);
	Fseek(oldoffset,handle,0);
	return !rc;
}

/*
 * chrapp(char *str, char ch)
 * append ch to str if it's not already the last character
 */
void chrapp(char *str,char ch)
{
	if (lastchr(str)!=ch)
		chrcat(str,ch);
}

char *itos(int i)
{
	static char str[17];
	
	return itoa(i,str,10);
}

char *mstrtok(char *src, const char *delim)
{
	static char *s;
	char *rtn;
	
	if (src!=NULL)
		s = src;
	rtn = s;
	while (*s != '\0')
	{
		if (strchr(delim,*s)!=NULL)
		{
			*s = '\0';
			while (*++s!='\0' && strchr(delim,*s)!=NULL)
				;
			return rtn;
		}
		++s;
	}
	return rtn==s ? NULL : rtn;
}

/*
 * int wildcard(const char *spec)
 * Purpose:
 * 	Determine if <spec> contains wildcard characters
 * Returns:
 * 	TRUE if <spec> contains '*', '?' or '[...]'
 *	FALSE otherwise
 */
int wildcard(const char *spec)
{
	char *closebrack, *openbrack;

	/* contains "*" or "?" ? */	
	if (strpbrk(spec,"*?"))
		return TRUE;

	/* open bracket? */
	openbrack = strchr(spec,'[');
	if (openbrack == NULL)
		return FALSE;
	
	/* closing bracket also? */
	closebrack = strchr(spec,']');
	if (closebrack == NULL)
		return FALSE;

	/* open bracket left of closing bracket? */
	if (openbrack >= closebrack-1)
		return FALSE;
	else
		return TRUE;
}

/*
 * int anywilddir(int elements,char **dir)
 * Purpose:
 *	Determine if any element in <dir> contains wildcard specifiers
 * Returns:
 *	Index of first wildcard element in <dir>
 *	-1 if no wildcards in <dir>
 */
int anywild(int elements,char **dir)
{
	int i, first = -1;
	
	for (i=elements-1; i>=0; --i)
		if (wildcard(dir[i]))
			first = i;
	return first;
}

/*
 * char charselect(const char *allowed)
 * Wait until one of the characters in allowed is typed in.
 */
char charselect(const char *allowed)
{
	char ch, uch;
	int legal;
	
	do
	{
		ch = (char)inchar();
		uch = toupper(ch);
		legal = strchr(allowed,uch)!=NULL;
		if (legal)
			rawout(ch);
		else
			beep();
	} while (!legal);
	crlf();
	return uch;
}

char *strins(char *dest, const char *ins, size_t where)
{
	size_t inslen = strlen(ins);
	char *pos = &dest[where];

	memmove(pos+inslen,pos,strlen(pos)+1);
	memcpy(pos,ins,inslen);
	return dest;
}

int ioerror(const char *cmd,const char *str,void *dta,int errcode)
{
	bioserr = oserr = errcode;

	/* Sorry, but this _has_ to be one single mprint call... */
	eprintf(MI_IOFMT,
		cmd==NULL ? "" : cmd,
		cmd==NULL ? "" : ": ",
		errcode,
		str==NULL ? "" : MI_IOACC,
		str==NULL ? "" : str);

	if (dta!=NULL)
		Fsetdta((DTA *)dta);
	return errcode;
}

int isdevice(const char *file)
{
	return !stricmp(file,"CON:") || !stricmp(file,"AUX:") || !stricmp(file,"PRN:");
}

/*
 * drvbit(const char *file)
 * return bit value (a la Drvmap()) for the drive "file" is on
 */
size_t drvbit(const char *file)
{
	int drv;

	/* check for devices first */
	if (isdevice(file))
		return 0;
			
	if (file[1] == ':' && legaldrive(*file))
		drv = *file;
	else
		drv = getdrv();
		
	return 1 << (toupper(drv) - 'A');
}

void print_longoption(char *options, struct option *long_option)
{
	char *a;
	char args[100], shortargs[20];

	*shortargs = '\0';
	strcpy(args,options);

	if (*args != '[')
		return;
		
	a = strtok(args,"[");
	while (a)
	{
		if (*a == '-' || *a == '+')
		{
			while (*a=='-' || *a=='+')
				++a;
			
			while (*a!=' ' && *a!=']')
				if (*a == '|')
					break;
				else
					chrcat(shortargs,*a++);
		}	
		a = strtok(NULL,"[");
	}
	a = shortargs;
	prntf(MI_OPTIONS "\n");
	while (long_option->name && *a)
		prntf("-%c = +%s\n",*a++,(long_option++)->name);
#if DEBUG
	if (long_option->name)
		dprint("oops, more longs than shorts\n");
	if (*a)
		dprint("oops, more shorts than longs\n");
#endif
}

int printusage(struct option *long_option)
{
	oserr = ERROR;
	if (iserrfile)
		prntf = mprintf;
	else
		prntf = eprintf;
		
	prntf(MI_USAGE ": %s %s\n",
		interncmd[icmd].name,interncmd[icmd].usage);

	if (long_option)
		print_longoption(interncmd[icmd].usage,long_option);
	prntf = eprintf;
	return 1;
}
