/*
 * @(#) Gemini\files.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: Filesstuff in windows for venus
 */

#include <tos.h>
#include <stddef.h>
#include <string.h>

#include "vs.h"
#include "stdbox.rh"
#include "menu.rh"

#include "files.h"
#include "fileutil.h"
#include "util.h"
#include "myalloc.h"
#include "venuserr.h"
#include "window.h"
#include "iconrule.h"
#include "filedraw.h"
#include "wildcard.h"
#define SORT_TYPE	FileInfo
#include "sortfile.h"

store_sccs_id(files);


/* externals
*/
extern wchar;
extern OBJECT *pstdbox;
extern ShowInfo show;
extern DeskInfo NewDesk;

static void fetchobj(WindInfo *wp);

FileInfo *GetSelectedFileInfo(WindInfo *wp)
{
	FileBucket *bucket;
	uword i;
	
	if (wp->kind != WK_FILE)
		return NULL;
		
	bucket = wp->files;
	while (bucket)
	{
		for (i = 0; i < bucket->usedcount; ++i)
		{
			if (bucket->finfo[i].flags.selected)
				return &(bucket->finfo[i]);
		}
	
		bucket = bucket->nextbucket;
	}
	
	return NULL;
}


/*
 * copy informations from DTA d to FileInfo structure p
 */
static void fillFileInfo (FileInfo *p, uword nummer, DTA *d)
{
	char filename[MAX_FILENAME_LEN];
	char *cs;

	strncpy (filename, d->d_fname, MAX_FILENAME_LEN - 1);
	filename[MAX_FILENAME_LEN - 1] = '\0';
	strcpy (p->fullname, filename);
	
	if ((strcmp (p->fullname, "..")
		&& (cs = strchr (filename, '.')) != NULL))
	{
		*cs++ = '\0';
		strcpy (p->ext, cs);
	}
	else
		p->ext[0] = '\0';
		
	strcpy (p->name, filename);
	p->attrib = d->d_attrib;
	p->size = d->d_length;
	p->date = d->d_date;
	p->time = d->d_time;
	p->flags.noObject = TRUE;
	p->flags.selected = FALSE;
	p->flags.dragged = FALSE;
	strcpy (p->magic, "FilE");
	p->number = nummer;
}

static word isValidAttribute(word attr)
{
	if (attr & FA_HIDDEN)
		return NewDesk.showHidden;

/*	if (attr & FA_LABEL)
		return FALSE;
*/	
	return TRUE;
}		

/*
 * get Files in Directory wp->path and build a linked list
 * of Fileinfo structures starting at wp->files
 * return if list has changed, so window had to be redrawn
 */
word getfiles (WindInfo *wp)
{
	FileBucket *newList;
	FileBucket *aktBucket, **previousBucket;
	char buffer[MAXLEN];
	word ok;
	long newDirSize;
	DTA *olddta, d;
	uword gesamtZahl, aktIndex;
	char getallfiles = !strcmp (wp->wildcard, "*");

	previousBucket = &newList;
	aktBucket = newList = NULL;
	newDirSize = 0L;
	gesamtZahl = 0;
	aktIndex = FILEBUCKETSIZE;

	getLabel (wp->path[0] - 'A', buffer);
	if(!sameLabel (buffer, wp->label))		/* Label ge„ndert */
	{
		while (!setFullPath (wp->path))			/* Pfad setzbar? */
		{
			if (!stripFolderName (wp->title)) 	/* cd .. machbar? */
				break;
			stripFolderName (wp->path);
		}		
	}
	if (strcmp (wp->label, buffer))
		strcpy (wp->label, buffer);			/* label merken */
		
	strcpy (buffer, wp->path);
	strcat (buffer, "*.*");		/* wildcard anh„ngen */
	olddta = Fgetdta ();
	Fsetdta (&d);				/* it's my own */
	
	if (wp->files)
	{
		freefiles (wp);		/* alte Files freigeben */
	}

	ok = Fsfirst (buffer, FA_NOLABEL);	/* mit directories */
	while (!ok)
	{
		if (isValidAttribute (d.d_attrib)
			&& strcmp (d.d_fname, ".")
			&& (getallfiles || (d.d_attrib & 0x10) 
				|| filterFile (wp->wildcard, d.d_fname)))
		{
			if (aktIndex == FILEBUCKETSIZE)
			{
				if (aktBucket)
					aktBucket->usedcount = aktIndex;
				
				aktBucket = tmpmalloc (sizeof (FileBucket));
				*previousBucket = aktBucket;
				
				if (!aktBucket)
					break;
				
				previousBucket = &(aktBucket->nextbucket);
				aktIndex = 0;
				memset (aktBucket, '\0', sizeof(FileBucket));
			}

			fillFileInfo (&aktBucket->finfo[aktIndex], 
				++gesamtZahl, &d); 
			newDirSize += d.d_length;
			++aktIndex;
		}
		ok = Fsnext ();
	}

	if (aktBucket)
		aktBucket->usedcount = aktIndex;
	
	wp->files = newList;
	wp->fileanz = gesamtZahl;
	wp->dirsize = newDirSize;
	wp->sorttype = UNSORT;			/* still unsorted */

	Fsetdta (olddta);
	return TRUE;
}

static void fillFileUnion(OBJECT *pStdObject, FileInfo *pf)
{
	OBJECT *po;
	char *pc, *pt, color;

	if(!show.showtext)
	{
		word xdiff, ydiff;
		
 		po = getIconObject(pf->attrib & FA_FOLDER,
 							pf->fullname, &color);

		memcpy(&pf->o.i, po->ob_spec.iconblk, sizeof(ICONBLK));
		pf->o.i.ib_ptext = pf->fullname;
		pf->o.i.ib_char = (color << 8)|(pf->o.i.ib_char & 0x00FF);
		
		/* Setze Offset des Icons im Objekt, damit das Icon unten
		 * zentriert im Fenster erscheint.
		 */
		xdiff = (pStdObject->ob_width - po->ob_width) / 2;
		if (xdiff)
		{
			pf->o.i.ib_xicon += xdiff;
			pf->o.i.ib_xtext += xdiff;
		}
		ydiff = pStdObject->ob_height - po->ob_height;
		if (ydiff)
		{
			pf->o.i.ib_yicon += ydiff;
			pf->o.i.ib_ytext += ydiff;
		}

		/* Setze Breite der Icon-Fahne
		 */
		SetIconTextWidth(&pf->o.i);
	}
	else
	{								/* is Textdisplay */
		char dirchar = pstdbox[FISTRING].ob_spec.free_string[0];

		pc = &pf->o.t.text[2];
		memset(&pf->o.t.text,(char)' ',TEXTLEN);
		if(pf->attrib & FA_FOLDER)			/* is Folder */
			pf->o.t.text[0] = dirchar;
		strcpy(pc,pf->name);
		pc[strlen(pc)] = ' ';			/* kill '\0' */
		pc += 9;
		strcpy(pc,pf->ext);
		pc[strlen(pc)] = ' ';
		pc += 4;
		if(show.fsize)
		{
			pt = pc += 8;
			pt--;
			if(pf->size)
			{
				long size = pf->size;
				
				while(size)
				{
					*(pt--) = (char)(size % 10) + '0';
					size /= 10;
				}
			}
			else if(!(pf->attrib & FA_FOLDER))
			{
				*pt = '0';
			}
		}
		if(show.fdate)
		{
			word year,month,day;
			
			pt = pc += 9;
			pt--;
#if ENGLISH
			year = ((pf->date & 0xFE00) >> 9) + 80;
			month = (pf->date & 0x01E0) >> 5;
			day = pf->date & 0x001F;
			*(pt--) = (char)(year % 10) + '0';
			year /= 10;
			*(pt--) = (char)(year % 10) + '0';
			*(pt--) = '/';
			*(pt--) = (char)(day % 10) + '0';
			day /= 10;
			*(pt--) = (char)(day) + '0';
			*(pt--) = '/';
			*(pt--) = (char)(month % 10) + '0';
			month /= 10;
			*(pt--) = (char)(month) + '0';
#endif
#if GERMAN
			year = ((pf->date & 0xFE00) >> 9) + 80;
			month = (pf->date & 0x01E0) >> 5;
			day = pf->date & 0x001F;
			*(pt--) = (char)(year % 10) + '0';
			year /= 10;
			*(pt--) = (char)(year % 10) + '0';
			*(pt--) = '.';
			*(pt--) = (char)(month % 10) + '0';
			month /= 10;
			*(pt--) = (char)(month) + '0';
			*(pt--) = '.';
			*(pt--) = (char)(day % 10) + '0';
			day /= 10;
			*(pt--) = (char)(day) + '0';
#endif

		}
		if(show.ftime)
		{
			word hour,min,sec;
			
			pt = pc += 9;
			pt--;
			hour = ((pf->time & 0xF800) >> 11) % 24;
			min = ((pf->time & 0x07E0) >> 5) % 60;
			sec = ((pf->time & 0x001F) << 1) % 60;
			*(pt--) = (char)(sec % 10) + '0';
			sec /= 10;
			*(pt--) = (char)(sec) + '0';
			*(pt--) = ':';
			*(pt--) = (char)(min % 10) + '0';
			min /= 10;
			*(pt--) = (char)(min) + '0';
			*(pt--) = ':';
			*(pt--) = (char)(hour % 10) + '0';
			hour /= 10;
			*(pt--) = (char)(hour) + '0';
		}
		*pc = '\0';
	}
	pf->flags.noObject = FALSE;
}

/*
 * static void fetchobj(WindInfo *wp)
 * get AES objects (icons or text) for files in linked
 * list wp->files
 * type of object depends on global structure show.
 */
static void fetchobj(WindInfo *wp)
{
	FileBucket *bucket;
	word obw;
	
	if(!show.showtext)
	{
		wp->stdobj = getStdFileIcon();
	}
	else
	{
		wp->stdobj = &pstdbox[FISTRING];
		obw = 15 + (show.fsize? 8:0) + (show.fdate? 9:0)
				 + (show.ftime? 9:0);
		pstdbox[FISTRING].ob_width = obw * ff_wchar;
	}
	wp->xdist = (!show.showtext)? 2 : (2 * ff_wchar);
	wp->obw = wp->stdobj->ob_width;
	wp->obh = wp->stdobj->ob_height;

	if ((!show.showtext) && (show.sortentry == SORTTYPE))
	{
		for (bucket = wp->files; bucket != NULL; 
			bucket = bucket->nextbucket)
		{
			uword i;
			
			for (i = 0; i < bucket->usedcount; ++i)
				fillFileUnion(wp->stdobj, &bucket->finfo[i]);
		}
	}
	else
	{
		for (bucket = wp->files; bucket != NULL; 
			bucket = bucket->nextbucket)
		{
			word i;
			
			for (i = 0; i < bucket->usedcount; ++i)
				bucket->finfo[i].flags.noObject = TRUE;
		}
	}
	return;
}

/*
 * void makeftree(WindInfo *wp)
 * make an object tree from at wp->files starting FileInfos
 * and link this tree to wp->tree
 */
void makeftree(WindInfo *wp)
{
	OBJECT *po;
	word currx, curry, gesamtanz, i;
	word skip;
	FileBucket *bucket;
	word aktIndex = 0;
	
	po = tmpmalloc((wp->fileanz + 1) * sizeof(OBJECT));
										/* ganzer Baum */
	if(po != NULL)
	{
		freeftree(wp);
		
		if(wp->update&SHOWTYPE)
		{
			fetchobj(wp);
		}
		if((show.sortentry != wp->sorttype)
			|| (wp->update & SHOWTYPE))
		{
			fileSort(wp, show.sortentry);
		}
		memcpy(po, pstdbox, sizeof(OBJECT));
		po[0].ob_next = po[0].ob_head = po[0].ob_tail = -1;
		po[0].ob_x = wp->workx;
		po[0].ob_y = wp->worky;
		po[0].ob_width = wp->workw;
		po[0].ob_height = wp->workh;
		
		wp->xanz = wp->workw / (wp->obw + wp->xdist);
		if(((wp->workw % (wp->obw + wp->xdist)) > (wp->obw * 2/3))
			||((wp->xanz == 0)&&(wp->fileanz != 0)))
			wp->xanz++;
			
		currx = wp->yanz = wp->workh / (wp->obh+1);
		if(wp->workh % (wp->obh+1))
			wp->yanz++;

		skip = wp->xanz * wp->yskip;

		wp->maxlines = wp->yanz;
		gesamtanz = wp->xanz * wp->yanz;
		curry = wp->fileanz - skip;

		if(gesamtanz > curry)
		{
			if(wp->yskip)
			{
				word usedlines,freelines;
			
				usedlines = curry / wp->xanz;
				if((curry > 0) && (curry % wp->xanz))
					usedlines++;
					
				freelines = currx - usedlines;
				
				if(freelines > 0)/* lines in window left blank */
				{
					wp->yskip -= freelines;
					if(wp->yskip < 0)
						wp->yskip = 0;

					skip = wp->xanz * wp->yskip;	/* neu berechnen */
					curry = wp->fileanz - skip;
				}

				if(gesamtanz > curry)
					gesamtanz = curry;	/* now it's exact */
			}
			else
				gesamtanz = curry;
		}

		bucket = wp->files;				/* berspringe Files */
		i = skip / FILEBUCKETSIZE;
		aktIndex = skip % FILEBUCKETSIZE;
		while(i)
		{
			bucket = bucket->nextbucket;
			--i;
		}

		if(gesamtanz)
		{
			FileInfo *pf;
			
			currx = curry = 0;
				
			for(i=1; i <= gesamtanz; i++)
			{
				memcpy(&po[i], wp->stdobj, sizeof(OBJECT));
				
				if (aktIndex == FILEBUCKETSIZE)
				{
					bucket = bucket->nextbucket;
					aktIndex = 0;
				}
				pf = &bucket->finfo[aktIndex];
				++aktIndex;
				
				/* dies ist noch nicht das letzte Objekt */
				po[i].ob_flags &= ~LASTOB;
				
				if (pf->flags.noObject)
					fillFileUnion(wp->stdobj, pf);
					
				if (pf->flags.selected)
					po[i].ob_state |= SELECTED;
				
				if (show.showtext)
				{
					po[i].ob_spec.userblk = &pf->o.t.ublk;
					pf->o.t.ublk.ub_code = drawFileText;
					pf->o.t.ublk.ub_parm = (long)pf->o.t.text;
					po[i].ob_type = G_USERDEF;
				}
				else
				{
					po[i].ob_spec.iconblk = &pf->o.i;
					pf->o.i.ib_ptext = pf->fullname;
				}
										 /* Pointer to iconblk */
										 
				po[i].ob_x = currx * (po[i].ob_width - 1 
								+ wp->xdist) + wp->xdist;
				po[i].ob_y = curry * (po[i].ob_height + 1) + 1;

				po[i].ob_head = po[i].ob_tail = -1;
				
				currx = (currx + 1) % wp->xanz;
				if (currx == 0)
					curry = (curry + 1) % wp->yanz;
				if (i>1)
					po[i-1].ob_next = i;
				else
					po[0].ob_head = 1;

			}

			/* setze LASTOB auf letztem Objekt des Baums */
			po[gesamtanz].ob_flags |= LASTOB;
			
			po[gesamtanz].ob_next = 0;
			po[0].ob_tail = gesamtanz;

			wp->yanz = gesamtanz / wp->xanz;
			if(gesamtanz % wp->xanz)
				wp->yanz++;
		}
	}
	else
	{
		wp->tree = NULL;
	}

	wp->objanz = gesamtanz + 1;
	wp->tree = po;
}

/*
 * free all FileInfos in list at wp->files
*/
void freefiles(WindInfo *wp)
{
	FileBucket *nextp, *p;

	p = wp->files;
	while(p != NULL)
	{
		nextp = p->nextbucket;
		tmpfree(p);
		p = nextp;
	}
	NewDesk.selectAnz -= wp->selectAnz;
	wp->selectAnz = 0;
	wp->selectSize = 0L;
}

/*
 * void freeftree(WindInfo *wp)
 * free object tree at wp->tree
 */
void freeftree(WindInfo *wp)
{
	if((wp->tree != pstdbox)&&(wp->tree != NULL))
	{
		tmpfree(wp->tree);
		wp->tree = NULL;
	}		
}

/*
 * Passt den Objektbaum der Breite und H”he des Fensters an. Wird
 * von window.c aufgerufen, wenn sich die Gr”že eines Fensters
 * ge„ndert hat.
 * Liefert zurck, ob sich an der Spaltenzahl etwas ver„ndert hat.
 * Nur dann ist es n”tig, den Inhalt des Fensters komplett neu zu
 * zeichnen.
 */
int SizeFileTree (WindInfo *wp)
{
	word old_columns;
	
	old_columns = wp->xanz;
	
	wp->update = (HOSLIDER|VESLIDER);
	calcWindData (wp);
	setWindData (wp);
	
	return (wp->xanz != old_columns);
}