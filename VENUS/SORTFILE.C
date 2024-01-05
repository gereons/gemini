/*
 * @(#) Gemini\sortfile.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: enth„lt Funktionen zum Sortieren von FileInfolisten 
 */

#include <string.h>
#include <nls\nls.h>

#include "vs.h"
#include "menu.rh"

#include "myalloc.h"
#include "venuserr.h"
#define SORT_TYPE	FileInfo
#include "sortfile.h"

store_sccs_id(sortfile);

/* externals
 */
extern ShowInfo show;

/* internal texts
 */
#define NlsLocalSection		"Gmni.sortfile"
enum NlsLocalText{
T_MEMERR,	/*Es ist nicht genug Speicher vorhanden, um alle
 Dateien anzuzeigen!*/
};


static word cmpnumber(FileInfo *f1, FileInfo *f2)
{
	return ((word)f1->number - (word)f2->number);
}

static word cmpname(FileInfo *f1, FileInfo *f2)
{
	word i;
	word folderdiff;
	
	folderdiff = (f2->attrib & FA_FOLDER) - (f1->attrib & FA_FOLDER);
	if (folderdiff)
		return folderdiff;
	
	if((i = strcmp(f1->name,f2->name)) == 0)
	{
		i = strcmp(f1->ext,f2->ext);
	}
	return i;
}

static word cmpsize(FileInfo *f1, FileInfo *f2)
{
	register long l;
	word folderdiff;
	
	folderdiff = (f2->attrib & FA_FOLDER) - (f1->attrib & FA_FOLDER);
	if (folderdiff)
		return folderdiff;

	l =  (f2->size - f1->size);
	if(l < 0)
		return -1;
	if(l > 0)
		return 1;
	return cmpname(f1,f2);
}

static word cmpdate(FileInfo *f1, FileInfo *f2)
{
	register long l;
	word folderdiff;
	
	folderdiff = (f2->attrib & FA_FOLDER) - (f1->attrib & FA_FOLDER);
	if (folderdiff)
		return folderdiff;
	
	if(f1->date == f2->date)
		l = ((long)f1->time) - f2->time;
	else
		l = ((long)f1->date) - f2->date;
	if(l < 0)
		return 1;
	if(l > 0)
		return -1;	
	return cmpname(f1,f2);
}

static word cmptype(FileInfo *f1, FileInfo *f2)
{
	register word i;
	word folderdiff;
	
	folderdiff = (f2->attrib & FA_FOLDER) - (f1->attrib & FA_FOLDER);
	if (folderdiff)
		return folderdiff;
		
	if(show.showtext)
	{
		if((i = strcmp(f1->ext,f2->ext)) == 0)
		{
			i = strcmp(f1->name,f2->name);
		}
	}
	else
	{
		i = (word) (f2->o.i.ib_pdata - f1->o.i.ib_pdata);
		if(i == 0)
		{
			i = cmpname(f1,f2);
		}
	}
	return i;
}

static void swapPointer(SORT_TYPE **list,word i, word j)
{
	register SORT_TYPE *tmp;
	
	tmp = list[i];
	list[i] = list[j];
	list[j] = tmp;
}

static void swapFileInfo(SORT_TYPE **list,word i, word j)
{
	FileInfo filebuffer, **filelist;
	
	filelist = (FileInfo **)list;
	filebuffer = *(filelist[i]);
	*(filelist[i]) = *(filelist[j]);
	*(filelist[j]) = filebuffer;
}

/* Quick-Sort aus K&R 2.edition S.110
*/
static void (* swapFunction)(SORT_TYPE **list,word i, word j);

static void myqSort(SORT_TYPE **list, CmpFkt cmpfiles,word left,word right)
{
	register word i,last;
	
	if(left >= right)
		return;
	
	swapFunction(list,left,(right + left)/2);
	last = left;
	for(i = left + 1; i <= right; i++)
	{
		if(cmpfiles(list[i],list[left]) < 0)
			swapFunction(list, ++last, i);
	}
	swapFunction(list, left, last);
	myqSort(list, cmpfiles, left, last - 1);
	myqSort(list, cmpfiles, last + 1, right);
}

void qSort(SORT_TYPE **list,CmpFkt cmpfiles,word left,word right)
{
	swapFunction = swapPointer;
	myqSort(list, cmpfiles, left, right);
}

static void filesort(WindInfo *wp, CmpFkt cmpfiles)
{
	FileBucket *bucket;
	FileInfo **fplist;				/* fr Array von FileInfopointern */
	word i, nr;
	
	if(wp->fileanz > 1)
	{
		fplist = tmpmalloc(wp->fileanz * sizeof(FileInfo *));
		if(fplist != NULL)
		{
			word start;
			
			bucket = wp->files;
			nr = 0;
			if ((bucket) 
				&& (bucket->finfo[0].attrib & FA_FOLDER)
				&& (!strcmp(bucket->finfo[0].fullname, "..")))
			{
				start = 1;
			}
			else
				start = 0;
			
			while (bucket)
			{
				for(i = 0; i < bucket->usedcount; ++i)
				{
					fplist[nr] = &(bucket->finfo[i]);
					++nr;
				}
				bucket = bucket->nextbucket;
			}
			
			swapFunction = swapFileInfo;
			myqSort(fplist, cmpfiles, start, wp->fileanz - 1);
			
			tmpfree(fplist);
		}
		else
		{
			venusErr(NlsStr (T_MEMERR));
		}
	}
}

void fileSort(WindInfo *wp,word sortmode)
{
	switch(sortmode)
	{
		case UNSORT:
			filesort(wp, cmpnumber);
			break;
		case SORTDATE:
			filesort(wp, cmpdate);
			break;
		case SORTNAME:
			filesort(wp, cmpname);
			break;
		case SORTTYPE:
			filesort(wp, cmptype);
			break;
		case SORTSIZE:
			filesort(wp, cmpsize);
			break;
	}

	wp->sorttype = sortmode;
}