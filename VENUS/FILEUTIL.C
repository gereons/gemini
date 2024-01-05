/*
 * @(#) Gemini\fileutil.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: utility functions for files
 */

#include <tos.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <nls\nls.h>

#include "vs.h"
#include "fileutil.h"
#include "util.h"
#include "venuserr.h"
#include "toserror.h"
#include "myalloc.h"

store_sccs_id(fileutil);

/* externals
 */
extern WindInfo wnull;
extern char bootpath[];
extern long drivemap;

/*
 * internal texts
 */
#define NlsLocalSection "G.fileutil"
enum NlsLocalText{
T_NESTED,	/*Operation wird abgebrochen, da die Schachtelung
 der Ordner zu tief ist.*/
T_CLIPDIR,	/*Konnte den Ordner fr das Klemmbrett nicht anlegen.*/
T_TRASHDIR,	/*Konnte den Ordner fr den Papierkorb nicht anlegen.*/
};

word legalDrive(word drive)
{
	if ( drive>=0)
		return (word)((Drvmap() >> drive) & 0x0001);
	else
		return FALSE;
}

/*
 * word isDirectory(const char *path)
 * stellt fest, ob path ein Directory ist.
 * path darf nur ein absoluter Pfadname sein.
 */
word isDirectory (const char *path)
{
	DTA mydta, *olddta;
	char p[MAXLEN];
	size_t last;
	
	strcpy (p, path);
	
	last = strlen (p) - 1;
	
	if( p[last] == '\\')
		p[last] = '\0';
		
	if (!legalDrive (toupper (*p) - 'A'))
		return FALSE;
	
	if (last == 2)
		return TRUE;		

	olddta = Fgetdta ();
	Fsetdta (&mydta);

	if (!Fsfirst (p, FA_ALL))
	{
		do
		{
			if (mydta.d_attrib & FA_FOLDER)
			{
				Fsetdta (olddta);
				return TRUE;
			}
		}
		while (!Fsnext ());
	}
	
	Fsetdta (olddta);
	return FALSE;
}

word addFileAnz(char *path, uword *foldnr, uword *filenr, long *size)
{
	DTA myDta, *olddta;
	char tmp[MAXLEN];
	char filename[MAX_FILENAME_LEN];
	word retcode = 0, ok = TRUE;
	
	olddta = Fgetdta ();
	Fsetdta (&myDta);

	if (strlen (path) > MAXLEN-MAX_FILENAME_LEN)
	{
		venusErr (NlsStr (T_NESTED));
		ok = FALSE;
	}
	else
	{
		strcpy (tmp, path);
		addFileName (tmp, "*.*");
	}
	
	if(ok && ((retcode = Fsfirst (tmp, FA_NOLABEL)) == 0))
	{
		do
		{
			strncpy (filename, myDta.d_fname, MAX_FILENAME_LEN - 1);
			filename[MAX_FILENAME_LEN - 1] = '\0';
		
			if(myDta.d_attrib & FA_FOLDER)	/* is folder */
			{
				if(strcmp (filename, ".")
					&& strcmp (filename, ".."))
				{
					(*foldnr)++;
					addFolderName (path, filename);
					ok = addFileAnz (path, foldnr, filenr, size);
					stripFolderName (path);
				}
			}
			else if (!(myDta.d_attrib & FA_LABEL))
			{
				*size += myDta.d_length;
				(*filenr)++;
			}
			
		} 
		while(!Fsnext () && ok);
	}
	else if (retcode != -33 && retcode != -34 && retcode != -49)
	{
		sysError (retcode);
		ok = FALSE;
	}
	
	Fsetdta (olddta);
	return ok;
}

word countMarkedFiles(WindInfo *wp, char *path, uword *foldnr, 
					uword *filenr,long *size)
{
	word i, ok = TRUE;
	char oldpath[MAXLEN];
	char tmpstr[MAXLEN];
	
	*size = *foldnr = *filenr = 0;
	getFullPath(oldpath);
	if (strlen(path) > MAXLEN-MAX_FILENAME_LEN)
	{
		venusErr(NlsStr(T_NESTED));
		return FALSE;
	}
	strcpy(tmpstr,path);	
	
	if (wp->kind == WK_FILE)
	{
		FileBucket *bucket = wp->files;
		
		while (bucket)
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				if (bucket->finfo[i].flags.dragged
					&& bucket->finfo[i].flags.selected)
				{
					if(bucket->finfo[i].attrib & FA_FOLDER)
					{
						(*foldnr)++;
						addFolderName(tmpstr,
								bucket->finfo[i].fullname);
						ok = addFileAnz(tmpstr, foldnr, filenr, size);
						stripFolderName(tmpstr);
					}
					else
					{
						*size += bucket->finfo[i].size;
						(*filenr)++;
					}
				}
			}
			bucket = bucket->nextbucket;
		}
	}
	else if (wp->kind == WK_DESK)
	{
		IconInfo *pii;

		for(i=wp->tree[0].ob_head;
			ok && (i > 0); i = wp->tree[i].ob_next)
		{
			if (wp->tree[i].ob_type == (G_ICON|DRAG))
			{
				pii = getIconInfo(&wp->tree[i]);
				switch(pii->type)
				{
					case DI_FOLDER:
						++(*foldnr);
					case DI_FILESYSTEM:
					case DI_SCRAPDIR:
					case DI_TRASHCAN:
						strcpy(tmpstr,pii->path);
						setFullPath(tmpstr);
						ok = addFileAnz(tmpstr,foldnr,
										filenr,size);
						break;
					case DI_PROGRAM:
						++(*filenr);
						*size += getFileSize(pii->path);
						break;
				}
			}
		}
	}
	setFullPath(oldpath);
	return ok;
}

word fileExists (const char *name)
{
	DTA mydta, *olddta;
	word gotit;
	
	olddta = Fgetdta ();
	Fsetdta (&mydta);
	
	gotit = (!Fsfirst (name, FA_NOLABEL));

	if (gotit)
		gotit = !(mydta.d_attrib & FA_LABEL);
		
	Fsetdta (olddta);
	return gotit;
}

long getFileSize(const char *name)
{
	DTA mydta,*olddta;
	long size;
	char drive;
	
	olddta = Fgetdta();
	Fsetdta(&mydta);
	
	drive = getDrive();
	setDrive(name[0]);
	
	if (!Fsfirst(name,FA_NOLABEL))
		size = mydta.d_length;
	else
		size = -1;

	setDrive(drive);
	Fsetdta(olddta);
	return size;
}

word getBaseName(char *base, const char *path)
{
	char *cp;
	
	*base = '\0';
	cp = strrchr(path,'\\');
	if (path[strlen(path)-1] == '\\')
	{						/* folder ENDS with backslash */
		char *pt = cp;
		
		*pt = '\0';
		if ((cp = strrchr(path,'\\')) != NULL)
		{
			strcpy(base,cp+1);
			*pt = '\\';
		}
		else
		{
			*pt = '\\';
			return FALSE;
		}
	}
	else
	{
		if (cp != NULL)
			strcpy(base,cp+1);
		else
		{
			strncpy(base, path, 12);
			base[12] = '\0';
		}
	}
	return TRUE;
}

word ValidFileName(const char *name)
{
	char file[MAX_FILENAME_LEN];
	
	if (getBaseName(file, name))
	{
		remchr(file, ' ');
		remchr(file, '.');
		return ((strlen(file) > 0)
				&& (strpbrk(file, "?*:") == NULL));
	}
	return FALSE;
}


word stripFileName(char *path)
{
	char *cp;
	
	if((cp = strrchr(path,'\\')) != NULL)
	{
		cp[1] = '\0';
		return TRUE;
	}
	return FALSE;
}

word stripFolderName(char *path)
{
	char *cp1,*cp2;
	
	if((cp1 = strrchr(path,'\\')) != NULL)
	{
		*cp1 = '\0';
		if((cp2 = strrchr(path,'\\')) != NULL)
		{
			cp2[1] = '\0';
			return TRUE;
		}
		else
			*cp1 = '\\';		/* restore path */
	}
	return FALSE;
}

void addFileName(char *path,const char *name)
{
	if (path[strlen(path)-1] != '\\')
		strcat(path,"\\");
	strcat(path,name);
}

void addFolderName(char *path,const char *name)
{
	if (path[strlen(path)-1] != '\\')
		strcat(path,"\\");
	strcat(path,name);
	strcat(path,"\\");
}

/*
 * void dateString(char *cp, word date)
 * makes a string out of a TOS-date-integer
 */
void dateString(char *cp, word date)
{
	register word year,month,day;
	register char *pt;

	cp[8] = '\0';
	pt = &cp[7];
	
	year = (((date & 0xFE00) >> 9) + 80) % 100;
	month = (date & 0x01E0) >> 5;
	day = date & 0x001F;
#if GERMAN
	*(pt--) = "0123456789"[year % 10];
	year /= 10;
	*(pt--) = "0123456789"[year % 10];
	*(pt--) = '.';
	*(pt--) = "0123456789"[month % 10];
	*(pt--) = "0123456789"[month / 10];
	*(pt--) = '.';
	*(pt--) = "0123456789"[day % 10];
	*(pt--) = "0123456789"[day / 10];
#endif
#if ENGLISH
	*(pt--) = "0123456789"[year % 10];
	year /= 10;
	*(pt--) = "0123456789"[year % 10];
	*(pt--) = '/';
	*(pt--) = "0123456789"[day % 10];
	*(pt--) = "0123456789"[day / 10];
	*(pt--) = '/';
	*(pt--) = "0123456789"[month % 10];
	*(pt--) = "0123456789"[month / 10];
#endif
}

/*
 * void timeString(char *cp, word time)
 * makes a string out of a TOS-time-integer
 */
void timeString(char *cp, word time)
{
	register word hour,min,sec;
	register char *pt;

	cp[8] = '\0';
	pt = &cp[7];
	
	hour = ((time & 0xF800) >> 11) % 24;
	min = ((time & 0x07E0) >> 5) % 60;
	sec = ((time & 0x001F) << 1) % 60;
	*(pt--) = "0123456789"[sec % 10];
	*(pt--) = "0123456789"[sec / 10];
	*(pt--) = ':';
	*(pt--) = "0123456789"[min % 10];
	*(pt--) = "0123456789"[min / 10];
	*(pt--) = ':';
	*(pt--) = "0123456789"[hour % 10];
	*(pt--) = "0123456789"[hour / 10];
}

/*
 * word getLabel(word drive,char *labelname)
 * return the Label(name) of a drive, if it has one
 */
word getLabel(word drive,char *labelname)
{
	DTA mydta,*old_dta;
	char tmp[MAXLEN];
	word retcode,ok = TRUE;
	
	old_dta = Fgetdta();
	Fsetdta(&mydta);
	tmp[0] = drive + 'A';
	strcpy(tmp+1,":\\*.*");
	if((retcode = Fsfirst(tmp,FA_LABEL)) == 0)
	{
		strncpy (labelname, mydta.d_fname, MAX_FILENAME_LEN - 1);
		labelname[MAX_FILENAME_LEN - 1] = '\0';
	}
	else
	{
		if(retcode != -33)
		{
			sysError(retcode);
			ok = FALSE;
		}
		*labelname = '\0';
	}
	Fsetdta(old_dta);
	return ok;
}

word getFullPath(char *path)
{
	word drive,retcode;
	
	drive = Dgetdrv();
	path[0] = (char)drive + 'A';
	path[1] = ':';
	retcode = Dgetpath(&path[2],drive +1);
	if(!retcode && path[strlen(path)-1] != '\\')
		strcat(path,"\\");

	return retcode;
}

word setDrive(const char drive)
{
	word drv;
	
	drv = drive - 'A';
	
	if (legalDrive(drv))
	{
		Dsetdrv(drv);
		return TRUE;
	}
	else
		return FALSE;
}

char getDrive(void)
{
	return Dgetdrv() + 'A';
}

word setFullPath(const char *path)
{
	char tmp[MAXLEN],new[MAXLEN];
	word ok,drive;
	size_t len;

	drive = path[0] - 'A';
	if(legalDrive(drive))
		Dsetdrv(drive);
	else
		return FALSE;

	Dgetpath(tmp,drive+1);
	if(!strlen(tmp))
		strcpy(tmp,"\\");			/* Oh, how I hate it! */

	strcpy(new,path);
	len = strlen(new) - 1;
	if((len > 2) && (new[len] == '\\'))
		new[len] = '\0';		/* kein backslash am Ende */
		
	ok = (Dsetpath(&new[2]) >= 0);
	if(!ok)
	{
		Dsetdrv(drive);
		Dsetpath("\\");
		Dsetpath(tmp);
	}

	return ok;
}

/*
 * word isExecutable(const char *name)
 * test in environmentstring SUFFIX, if it includes the extendion
 * if not available, test default (PRG,APP,TOS,TTP)
 */
word isExecutable(const char *name)
{
	static const char defSuffix[] = {"PRG;APP;TTP;TOS"};
	const char *pSuffix;
	char *mySuffix,*cp,*pext;
	size_t len;
	
	pext = strrchr(name,'.');
	if(pext)
		pext++;
	else
		return FALSE;		/* no extension, not executable */
		
	pSuffix = getenv("SUFFIX");
	if(pSuffix)
	{
		len = strlen(pSuffix) + 1;
	}
	else
	{
		len = strlen(defSuffix) + 1;
		pSuffix = defSuffix;
	}
	cp = tmpmalloc(len);
	if(cp)
	{
		strcpy(cp,pSuffix);
		strupr(cp);
		mySuffix = cp;
		
		if((cp = strtok(mySuffix,";,")) != NULL)
		{
			do
			{
				if(!strcmp(cp,pext))
				{
					tmpfree(mySuffix);
					return TRUE;
				}
					
			} while((cp = strtok(NULL,";,")) != NULL);

		}
		tmpfree(mySuffix);
	}
	return FALSE;		/* malloc failed or fallen through */
}

/*
 * word sameLabel(const char *label1,const char *label2)
 * vergleicht zwei Label. Ist ein String leer,
 * so wird TRUE zurckgeliefert
 */
word sameLabel(const char *label1,const char *label2)
{
	if(strlen(label1) && strlen(label2))
	{
		return (!strcmp(label1,label2));
	}
	return TRUE;
}

/*
 * word isEmptyDir(const char *path)
 * check if directory is empty
 */
word isEmptyDir(const char *path)
{
	DTA mydta,*olddta;
	char tmp[MAXLEN];
	word empty = TRUE;
	
	olddta = Fgetdta();
	Fsetdta(&mydta);
	strcpy(tmp,path);
	addFileName(tmp,"*.*");
	if(!Fsfirst(tmp,FA_NOLABEL))
	{
		do
		{
			empty = !strcmp (mydta.d_fname,".")
					|| !strcmp (mydta.d_fname,"..")
					|| (mydta.d_attrib & FA_LABEL);
		}
		while(!Fsnext() && empty);
	}
	
	Fsetdta(olddta);
	return empty;
}

static word getBootDev(void)
{
	word *oldstack;
	word bootdev,*p_bootdev = (word *)0x446L;
	
	oldstack = (word *)Super(0L);
	bootdev = *p_bootdev;
	Super(oldstack);
	return bootdev;
}

/*
 * void getSPath(char **path)
 * fill path with the AES-Scrapdir, if this is empty
 * try to set it by yourself and look in environment for
 * "CLIPBRD"
 */
void getSPath(char *path)
{
	word bootdev,found = FALSE;
	char tmp[MAXLEN],*penv;
	
	
	scrp_read(path);
	if ((!strlen(path)) || (!isDirectory(path)))
	{
		getFullPath(tmp);

		if((penv = getenv("CLIPBRD")) == NULL)
			penv = getenv("clipbrd");
	
		if(penv)
		{
			strcpy(path,penv);
			strupr(path);
			if(path[strlen(path)-1] != '\\')
				strcat(path,"\\");
			found = isDirectory(path);
		}
		if(!found && (bootdev = getBootDev()) >= 2) /* no Floppy */
		{
			path[0] = (char)bootdev + 'A';	/* try in root */
			path[1] = '\0';
			strcat(path,":\\CLIPBRD\\");
			if(!isDirectory(path))
			{
				stripFolderName(path);
				setFullPath(path);
				found = (Dcreate("CLIPBRD") >= 0);
				addFolderName(path,"CLIPBRD");
			}
			else
				found = TRUE;
		}
		if(!found)
		{
			strcpy(path,bootpath);			/* try in own dir */
			addFolderName(path,"CLIPBRD");
			if(!isDirectory(path))
			{
				setFullPath(bootpath);
				if(Dcreate("CLIPBRD") < 0)
					venusErr(NlsStr(T_CLIPDIR));
			}
		}
		scrp_write(path);
		setFullPath(tmp);
	}
	if(strchr(path,'*') || strchr(path,'?'))	/* wildcards? */
		stripFileName(path);
	if(path[strlen(path)-1] != '\\')
		strcat(path,"\\");
}

/*
 * void getTPath(char *path)
 * get path for trashdir, look in environment for
 * variable "TRASHDIR"
 */
void getTPath(char *path)
{
	const char *penv;
	char tmp[MAXLEN];
	
	if((penv = getenv("TRASHDIR")) == NULL)
		penv = getenv("trashdir");
	
	if(penv && isDirectory(penv))
	{
		strcpy(path,penv);
		strupr(path);
		if(path[strlen(path)-1] != '\\')
			strcat(path,"\\");
	}
	else
	{
		getFullPath(tmp);
		strcpy(path,bootpath);
		setFullPath(bootpath);
		addFolderName(path,"TRASHDIR");
		if(!isDirectory(path))
		{
			if(Dcreate("TRASHDIR") < 0)
				venusErr(NlsStr(T_TRASHDIR));
		}
		setFullPath(tmp);
	}
}

/*
 * void forceMediaChange(word drive)
 * force a media change usually on floppies,
 * so drive should be 0 or 1!
 */
word forceMediaChange(word drive)
{
/*	Rwabs(0,NULL,2,2,drive);
 */
	extern word cdecl mediach(word drive);
	
	return mediach(drive);
}

/*
 * word getDiskSpace(word drive,unsigned long *bytesused,
 * 						unsigned long *bytesfree)
 * get the free and used space on disk drive: 
 */
word getDiskSpace(word drive, long *bytesused,
						long *bytesfree)
{
	DISKINFO di;
	long bytespercluster;
	word ok, trymedia = TRUE;
	
	do
	{
		if (Dfree (&di, drive + 1) < 0)
			ok = FALSE;
		else
		{
			bytespercluster = di.b_secsiz * di.b_clsiz;
			*bytesfree = di.b_free * bytespercluster;
			*bytesused = (di.b_total - di.b_free - 2) * bytespercluster;

			if (*bytesused < 0)
			{
				*bytesused = 0;
			}
			ok = (*bytesfree >= 0);
		}
		
		if(!ok)
		{
			if(trymedia)
			{
				forceMediaChange(drive);
				trymedia = FALSE;
			}
			else
				return FALSE;
		}
		
	} 
	while (!ok);

	return TRUE;
}

word fileRename(char *oldname,char *newname)
{
	return Frename(0,oldname,newname);
}

word setFileAttribut(char *name,word attrib)
{
	return Fattrib(name,TRUE,attrib);
}

word dirCreate(char *path)
{
	if (fileExists (path))
		return EACCDN;
	else
		return Dcreate(path);
}

word isAccessory(const char *filename)
{
	const char *cp,*ep;
	
	if((cp = strrchr(filename,'\\')) == NULL)	/* get name part */
		cp = filename;
	if((ep = strrchr(cp,'.')) != NULL)			/* get extension */
	{
		return (!strcmp(ep+1,"ACC"));
	}
	return FALSE;
}

word tosVersion(void)
{
	SYSHDR *sysp,**syspp = (SYSHDR **)0x4F2;
	word *oldstack,version;
	
	oldstack = (word *)Super(0L);
	sysp = *syspp;
	version = sysp->os_version;
	Super(oldstack);
	return version;
}

word delFile(const char *fname)
{
	return !Fdelete (fname);
}

void char2LongKey(char c, long *key)
{
	KEYTAB *kt;
	char *cp, value;
	word i;
	
	
	value = tolower(c);
	kt = Keytbl((void *)-1L, (void *)-1L, (void *)-1L);
	cp = kt->unshift;
	for (i=0; i < 128; ++i)
	{
		if (cp[i] == value)
		{
			*key = ((long)i) << 16;
			break;
		}
	}
	*key |= (long)c;
}

word Fputs(word fhandle, char *string)
{
	long size;
	
	size = strlen(string);
	if (size == Fwrite(fhandle, size, string))
	{
		return (2 == Fwrite(fhandle, 2L, "\r\n"));
	}
	return FALSE;
}