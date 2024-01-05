/*
 * @(#) Gemini\select.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: stuff for selecting objects etc.
 */

#include <string.h>

#include "vs.h"
#include "menu.rh"

#include "select.h"
#include "venuserr.h"
#include "redraw.h"
#include "util.h"
#include "scroll.h"
#include "myalloc.h"
#include "window.h"

store_sccs_id(select);

/* externals
 */
extern word apid;
extern WindInfo wnull;
extern DeskInfo NewDesk;
extern ShowInfo show;


void objdeselect(WindInfo *wp, word objnr)
{
	FileInfo *pfi;
	
	if (wp->kind == WK_FILE)
	{
		pfi = getfinfo(&wp->tree[objnr]);
		pfi->flags.selected = FALSE;
		wp->selectSize -= pfi->size;
		wp->update |= WINDINFO;
	}
	wp->tree[objnr].ob_state &= ~SELECTED;		/* Objekt (de)selektieren */
	redrawObj(wp, objnr);
	NewDesk.selectAnz--;
	wp->selectAnz--;
	if(NewDesk.selectAnz < 0)
		NewDesk.selectAnz = 0;
}

static void objSelNoDraw(WindInfo *wp, word objnr)
{
	FileInfo *pfi;
	
	if (wp->kind == WK_FILE)
	{
		pfi = getfinfo(&wp->tree[objnr]);
		pfi->flags.selected = TRUE;
		wp->selectSize += pfi->size;
		wp->update |= WINDINFO;
	}
	wp->tree[objnr].ob_state |= SELECTED;		/* Objekt (de)selektieren */
	NewDesk.selectAnz++;
	wp->selectAnz++;
}

void objselect(WindInfo *wp,word objnr)
{
	objSelNoDraw(wp, objnr);
	redrawObj(wp,objnr);
}

void desWindObjects(WindInfo *wp, word type)
{
	register word j;
	OBJECT *tree;
	
	if(!wp->selectAnz)	/* not here */
		return;
	tree = wp->tree;
	for(j = tree[0].ob_head;j > 0; j = tree[j].ob_next)
	{
		/*
		 * deselektiere type, oder wenn type null ist
		 * deselektiere alle Objekte
		 */
		if((tree[j].ob_state & SELECTED)
			&&((type == 0)||(tree[j].ob_type == type)))	
		{
			objdeselect(wp,j);
		}
	}
	if (wp->selectAnz && (wp->kind == WK_FILE))
	{
		FileBucket *bucket = wp->files;
		uword i;
		
		while(bucket)
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				bucket->finfo[i].flags.selected = FALSE;
			}
			bucket = bucket->nextbucket;
		}
		NewDesk.selectAnz -= wp->selectAnz;
		wp->selectAnz = 0;
		wp->selectSize = 0L;
		wp->update |= WINDINFO;
	}
	flushredraw();
}

void desObjExceptWind(WindInfo *wp, word type)
{
	WindInfo *awp;

	awp = &wnull;
	do
	{
		if((awp->selectAnz) && (awp != wp))
			desWindObjects(awp,type);
		awp = awp->nextwind;

	} while(awp != NULL);	
}

void deselectObjects(word type)
{
	WindInfo *wp;
	
	wp = &wnull;
	do
	{
		if(wp->selectAnz)
			desWindObjects(wp,type);
		wp = wp->nextwind;

	} while(wp != NULL);	
}

word thereAreSelected(WindInfo **wpp)
{
	if (NewDesk.selectAnz > 0)
	{
		*wpp = &wnull;
		do
		{
			if ((*wpp)->selectAnz)
				return TRUE;
			*wpp = (*wpp)->nextwind;
		}
		while (*wpp != NULL);
	}

	return FALSE;
}

/* get the only selected Object, if there is one
 */
word getOnlySelected(WindInfo **wpp,word *objnr)
{
	register WindInfo *awp;
	register word i;
	
	if(NewDesk.selectAnz != 1)
		return FALSE;
	
	awp = &wnull;
	while(awp != NULL)
	{
		if(awp->selectAnz)
		{
			*wpp = awp;
			*objnr = -1;

			for(i=awp->tree[0].ob_head; i>0;i=awp->tree[i].ob_next)
			{
				if(awp->tree[i].ob_state & SELECTED)
					*objnr = i;
			}
			return TRUE;
		}
		awp = awp->nextwind;
	}
	return FALSE;		/* defensive programming */
}

/*
 * void desNotDragged(WindInfo *wp)
 * deselect all objects in wp which were not dragged
 */
void desNotDragged(WindInfo *wp)
{
	register word j;
	OBJECT *tree;
	
	if(!wp->selectAnz)	/* not here */
		return;
	tree = wp->tree;
	for(j=tree[0].ob_head; j>0; j=tree[j].ob_next)
	{
		if((tree[j].ob_state & SELECTED)
			&&(!(tree[j].ob_type & DRAG)))
		{
			objdeselect(wp,j);
		}
	}
	if (wp->selectAnz && (wp->kind == WK_FILE))
	{
		FileBucket *bucket = wp->files;
		uword i;
		
		while(bucket)
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				if (bucket->finfo[i].flags.selected
					&& (!bucket->finfo[i].flags.dragged))
				{
					bucket->finfo[i].flags.selected = FALSE;
					--NewDesk.selectAnz;
					--wp->selectAnz;
					wp->selectSize -= bucket->finfo[i].size;
				}
			}
			bucket = bucket->nextbucket;
		}

		wp->update |= WINDINFO;
	}
	flushredraw();
}

word charSelect(WindInfo *wp, char c, word forfolder)
{
	FileInfo *pf;
	register word i,ok = TRUE;
	char *cp;
	
	if(wp->kind == WK_FILE)
	{
		wp->update |= WINDINFO;

		for(i=wp->tree[0].ob_head; (i>0)&&ok; i=wp->tree[i].ob_next)
		{
			pf = getfinfo(&wp->tree[i]);
			if (!pf)
				continue;
			
			if(((pf->attrib & FA_FOLDER) && forfolder)
				||(!(pf->attrib & FA_FOLDER) && !forfolder))
			{
				if(show.sortentry == SORTNAME)
					cp = pf->fullname;
				else
					cp = pf->ext;
				ok = ((*cp < c) || (*cp != '.'));
				if(*cp == c)
				{
					deselectObjects(0);
					objselect(wp,i);
					flushredraw();

					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

/*
 * word stringSelect(WindInfo *wp,const char *name)
 * select an Object in window wp by giving its name
 * works only in file windows
 */
word stringSelect(WindInfo *wp, const char *name, word redraw)
{
	FileBucket *bucket;
	uword nr, aktIndex;
	word currline,found;
	
	if (wp->kind != WK_FILE)
		return FALSE;
		
	currline = 0;
	nr = aktIndex = 0;
	found = FALSE;
	bucket = wp->files;					/* start of list */

	while (!found && (bucket != NULL))
	{
		for (aktIndex = 0; aktIndex < bucket->usedcount; ++aktIndex)
		{
			if (!strcmp(name, bucket->finfo[aktIndex].fullname))	
			{
				found = TRUE;
				break;
			}
			
			nr++;
			if(nr % wp->xanz == 0)
				currline++;
		}
		
		bucket = bucket->nextbucket;
	}

	if(found)
	{
		word offset;
		
		wp->update |= WINDINFO;

		offset = nr - (wp->xanz * wp->yskip) + 1;
											/* offset in wp->tree */
		if(offset < 0 || offset >= wp->objanz)
		{
			word newyskip;
			
			newyskip = calcYSkip(wp,currline);
	
			if(newyskip != wp->yskip)
			{
				word divskip;
				
				divskip = wp->yskip - newyskip;
				wp->yskip = newyskip;
				scrollit(wp, divskip, redraw);
			}
		}
		
		nr = nr - (wp->xanz * wp->yskip) + 1;/* nummer in wp->tree */
		deselectObjects(0);
		if (redraw)
			objselect(wp, nr);
		else
			objSelNoDraw(wp, nr);
		flushredraw();
	}
	return found;
}

char *GetSelectedObjects(void)
{
	WindInfo *wp;
	char *str;
	word i, nr;
	size_t len, pathlen;
	
	if(NewDesk.selectAnz < 1)
		return NULL;
	
	wp = &wnull;
	while (wp)
	{
		if(wp->selectAnz)
			break;
			
		wp = wp->nextwind;
	}
	if (!wp)		/* Ende der Liste erreicht */
		return NULL;
	
	pathlen = strlen(wp->path);
	
	nr = 0;
	len = 1;

	if (wp->kind == WK_FILE)
	{
		FileBucket *bucket = wp->files;
		
		while (bucket)
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				if (bucket->finfo[i].flags.selected)
				{
					len += pathlen 
						+ strlen(bucket->finfo[i].fullname) + 1;
					if (bucket->finfo[i].attrib & FA_FOLDER)
						++len;
				}
			}
		}
	}
	else if (wp->kind == WK_DESK)
	{
		IconInfo *pii;

		for (i = wp->tree[0].ob_head; i > 0; i = wp->tree[i].ob_next)
		{
			if (!isSelected(wp->tree, i))
				continue;
			
			nr++;
			pii = getIconInfo(&wp->tree[i]);
			if (strlen(pii->path))
				len += strlen(pii->path) + 1;
		}		
	}
	
	str = tmpmalloc(len);
	
	if (!str)
		return NULL;

	str[0] = '\0';

	if (wp->kind == WK_FILE)
	{
		FileBucket *bucket = wp->files;
		
		while (bucket)
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				if (bucket->finfo[i].flags.selected)
				{
					--nr;
					strcat(str, wp->path);
					strcat(str, bucket->finfo[i].fullname);
					if (bucket->finfo[i].attrib & FA_FOLDER)
						strcat(str, "\\");
					strcat(str," ");
				}
			}
		}
	}
	else if (wp->kind == WK_DESK)
	{		
		IconInfo *pii;

		for (i = wp->tree[0].ob_head; 
				i > 0; i = wp->tree[i].ob_next)
		{
			if (!isSelected(wp->tree, i))
				continue;

			nr--;
			pii = getIconInfo(&wp->tree[i]);
			strcat(str, pii->path);
			strcat(str," ");
		}
	}
	
	return str;
}

static word selectAllInFileWindow(WindInfo *wp)
{
	FileBucket *bucket;
	uword i;
	
	desObjExceptWind(wp, 0);
	
	bucket = wp->files;
	
	while(bucket)
	{
		for (i = 0; i < bucket->usedcount; ++i)
		{
			if (bucket->finfo[i].flags.selected)
				continue;
			if (!strcmp(bucket->finfo[i].fullname, ".."))
				continue;
			
			bucket->finfo[i].flags.selected = TRUE;
			++wp->selectAnz;
			++NewDesk.selectAnz;
			wp->selectSize += bucket->finfo[i].size;
		}
		
		bucket = bucket->nextbucket;
	}

	rewindow(wp, WINDINFO);
	return TRUE;
}

static word selectAllInDeskWindow(WindInfo *wp)
{
	int i;
	
	desObjExceptWind(wp, 0);
	
	for (i = wp->tree[0].ob_head; i > 0; i = wp->tree[i].ob_next)
	{
		if (isSelected(wp->tree, i))
			continue;

		setSelected(wp->tree, i, TRUE);
		redrawObj(wp, i);
		++wp->selectAnz;
		++NewDesk.selectAnz;
	}
	
	flushredraw();
	return TRUE;
}

word SelectAllInTopWindow(void)
{
	WindInfo *wp;
	word windhandle;
	
	wind_get(0, WF_TOP, &windhandle);
	wp = getwp(windhandle);
	if (!wp || (wp->owner != apid))
		return FALSE;
	
	switch (wp->kind)
	{
		case WK_FILE:
			return selectAllInFileWindow(wp);

		case WK_DESK:
			return selectAllInDeskWindow(wp);

		default:
			return FALSE;
	}
}

void MarkDraggedObjects(WindInfo *wp)
{
	OBJECT *tree;
	word i;
	
	tree = wp->tree;
	for(i=1;i<wp->objanz;i++)
	{
		if (tree[i].ob_state & SELECTED)
			tree[i].ob_type |= DRAG;
	}
	if (wp->kind == WK_FILE)
	{
		FileBucket *bucket;
		
		bucket = wp->files;
		while (bucket)
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				if (bucket->finfo[i].flags.selected)
					bucket->finfo[i].flags.dragged = TRUE;
			}
			bucket = bucket->nextbucket;
		}
	}
}

/*
 * rub out marks 'DRAG' from all objects in window wp
 */
void UnMarkDraggedObjects(WindInfo *wp)
{
	OBJECT *tree;
	word i;
	
	tree = wp->tree;
	for(i=1;i<wp->objanz;i++)
	{
		tree[i].ob_type &= ~DRAG;
	}
	if (wp->kind == WK_FILE)
	{
		FileBucket *bucket;
		
		bucket = wp->files;
		while (bucket)
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				bucket->finfo[i].flags.dragged = FALSE;
			}
			bucket = bucket->nextbucket;
		}
	}
}

