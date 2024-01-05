/*
 * gemtrees.c
 *
 * project: venus
 *
 * @(#) Gemini\gemtrees.c
 * @(#) Stefan Eissing, 24. M„rz 1991
 *
 * description: functions to work on AES trees
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <aes.h>
#include <flydial\flydial.h>

#include "vs.h"
#include "myalloc.h"
#include "gemtrees.h"

store_sccs_id(gemtrees);

#define NO_OBJ		-1

#define STRETCHZ	9
#define STRETCHN	8

/*
 * externals
 */
extern word hbox;		/* height of an AES-box for a character */
extern word handle;

/*
 * internals
 */
static word vstretchz,vstretchn;	/* vertical stretch factor z/n */
static word retcode;


/*
 * void walkGemTree(OBJECT *po, word objnr,
 *			void (*walkfunc)(OBJECT *po,word objnr))
 *
 * a simple depth-first walk algorithm for
 * AES-trees
 */
void walkGemTree(OBJECT *po, word objnr,
				void (*walkfunc)(OBJECT *po,word objnr))
{
	word i;
	
	if(po[objnr].ob_head != NO_OBJ)
	{
		for (i=po[objnr].ob_head; (i != objnr); i = po[i].ob_next)
		{
			walkGemTree(po,i,walkfunc);
		}
	}
	walkfunc(po,objnr);		/* and do it for yourself */
}

/*
 * static word objcVStretch(OBJECT *po, word objnr)
 * 
 * stretch the y-location of an object by
 * vstretchz/vstretchn
 */
static void objcVStretch(OBJECT *po, word objnr)
{
	if(objnr)			/* not the main box */
	{
		po[objnr].ob_y = (po[objnr].ob_y * vstretchz) / vstretchn;
	}
	switch (po[objnr].ob_type & 0x00FF)
	{
		case G_BOX:			/* align the height of a box */
		case G_IBOX:
			if (!(po[objnr].ob_state & CROSSED))
				po[objnr].ob_height = (po[objnr].ob_height
										 * vstretchz) / vstretchn;
			break;
	}
}

/*
 * void vStretchTree(OBJECT *po,word objnr,word z,word n)
 * stretch a tree vertically by z/n
 */
void vStretchTree(OBJECT *po,word objnr,word z,word n)
{
	vstretchz = z;
	vstretchn = n;
	
	walkGemTree(po,objnr,objcVStretch);
}

/* Richtung, in der sortiert werden soll
 */
static word sortgrav1, sortgrav2;

static word cmpObjects(OBJECT **po1, OBJECT **po2)
{
	word xdiff, ydiff;
	word first, second;
	
	ydiff = (*po1)->ob_y - (*po2)->ob_y;
	xdiff = (*po1)->ob_x - (*po2)->ob_x;

	switch (sortgrav1)
	{
		case D_SOUTH:
			ydiff = -ydiff;
		case D_NORTH:
			first = ydiff;
			if (sortgrav2 == D_EAST)
				xdiff = -xdiff;
			second = xdiff;
			break;

		case D_EAST:
			xdiff = -xdiff;
		case D_WEST:
			first = xdiff;
			if (sortgrav2 == D_SOUTH)
				ydiff = -ydiff;
			second = ydiff;
			break;
	}
	
	if (first)
		return first;
	else
		return second;
}

/*
 * sortiert die Objekte in tree
 * startet mit objekt startobj und geht
 * maxlevel tief
 */
word sortTree(OBJECT *tree, word startobj, word maxlevel, 
					word grav1, word grav2)
{
	OBJECT **objlist;
	word i,nr,anz,*lastptr;
	ptrdiff_t objnr;

	sortgrav1 = grav1;
	sortgrav2 = grav2;
	
	if ((maxlevel < 1) || (tree[startobj].ob_head == NO_OBJ))
		return TRUE;
		
	anz = 0;
	for(nr=tree[startobj].ob_head;
		nr != startobj; nr=tree[nr].ob_next)
		++anz;

	objlist = tmpmalloc(anz * sizeof(OBJECT *));
	if (!objlist)
		return FALSE;
	
	for(nr=tree[startobj].ob_head,i=0;
		nr != startobj; nr=tree[nr].ob_next, ++i)
	{
		if (tree[nr].ob_head != NO_OBJ)
			sortTree(tree,nr,maxlevel-1, grav1, grav2);
			
		objlist[i] = &tree[nr];
	}

	qsort (objlist, anz, sizeof (OBJECT *), cmpObjects);

	lastptr = &tree[startobj].ob_head;
	
	for(i=0; i<anz; ++i)
	{
		objnr = objlist[i] - tree;
		*lastptr = (word)objnr;
		lastptr = &tree[objnr].ob_next;
	}
	*lastptr = startobj;
	tree[startobj].ob_tail = (word)objnr;
	tmpfree(objlist);
	return TRUE;
}

static word calcChilds(OBJECT *tree, word index, word *height)
{
	word anz;
	word last;
	
	if (tree[index].ob_head < 0)
		return 0;
	
	last = tree[index].ob_tail;
	index = tree[index].ob_head;
	*height = tree[index].ob_height;
	anz = 1;
	
	while (index != last)
	{
		index = tree[index].ob_next;
		*height += tree[index].ob_height;
		++anz;
	}
	return anz;
}

static void fixInButton(OBJECT *tree, word index)
{
	word childs, last, h, y, step;
	
	if (tree[index].ob_type != (0x1400|G_USERDEF))
		return;
	
	childs = calcChilds(tree, index, &h);
	if (childs)
	{
		step = (tree[index].ob_height - h - HandYSize) / (childs+1);
		if (step <= 0)
			return;
		
		y = step + HandYSize;
		last = tree[index].ob_tail;
		index = tree[index].ob_head;
		tree[index].ob_y = y;
		y += step + tree[index].ob_height;
		
		while (last != index)
		{
			index = tree[index].ob_next;
			tree[index].ob_y = y;
			y += step + tree[index].ob_height;
		}
	}
}

void FixTree(OBJECT *tree)
{
	ObjcTreeInit(tree);
	walkGemTree(tree, 0, fixInButton);
}

static void getMaxNo(OBJECT *tree, word index)
{
	(void)tree;
	if (index > retcode)
		retcode = index;
}

/* markiere das letzte Objekt in einem Baum mit
 * LASTOB.
 */
void SetLastObject(OBJECT *tree)
{
	word i;
	
	retcode = 0;
	walkGemTree(tree, 0, getMaxNo);
	
	for (i = 0; i < retcode; ++i)
		tree[i].ob_flags &= ~LASTOB;
	
	tree[retcode].ob_flags |= LASTOB;
}