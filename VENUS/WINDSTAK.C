/*
 * @(#) Gemini\windstak.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: keep stack of windows
 */

#include "vs.h"
#include "windstak.h"
#include "myalloc.h"
#include "util.h"
#include "fileutil.h"


store_sccs_id(windstak);

/* externals
 */
extern word deskx,desky,deskw,deskh;

/* internals
 */
struct windBox
{
	struct windBox *nextbox;
	word x,y,w,h;		/* Koordinaten */
};
typedef struct windBox WindBox;

static WindBox *boxList = NULL;

/*
 * void pushWindBox(word x,word y,word w,word h)
 * push a WindBox onto the stack
 */
void pushWindBox(word x,word y,word w,word h)
{
	WindBox *pwb;
	
	if((pwb = malloc(sizeof(WindBox))) != NULL)
	{
		pwb->nextbox = boxList;
		boxList = pwb;
		pwb->x = x;
		pwb->y = y;
		pwb->w = w;
		pwb->h = h;
	}
}

/*
 * word popWindBox(word *x,word *y,word *w,word *h)
 * pop a WindBox from the stack
 */
word popWindBox(word *x,word *y,word *w,word *h)
{
	WindBox *pwb;
	
	if(boxList)
	{
		pwb = boxList;
		boxList = boxList->nextbox;
		*x = pwb->x;
		*y = pwb->y;
		*w = pwb->w;
		*h = pwb->h;
		free(pwb);
		return TRUE;
	}
	else
		return FALSE;
}

/*
 * static void writeRekursivBoxes(FILE *fp,WindBox *pwb)
 * write recursiv Box-coordinates in file fp
 */
static word writeRekursivBoxes(WindBox *pwb,
							 word fhandle, char *buffer)
{
	word x,y,w,h;
	
	if(pwb)
	{
		if (!writeRekursivBoxes(pwb->nextbox, fhandle, buffer))
			return FALSE;
		x = (word)scale123(1000L,pwb->x - deskx,deskw);
		w = (word)scale123(1000L,pwb->w - deskx,deskw);
		y = (word)scale123(1000L,pwb->y - desky,deskh);
		h = (word)scale123(1000L,pwb->h - desky,deskh);

		sprintf(buffer,"#B@%d@%d@%d@%d",x,y,w,h);
		return Fputs(fhandle, buffer);
	}
	return TRUE;
}

/*
 * write all Boxes in File fp
 */
word writeBoxes(word fhandle, char *buffer)
{
	return writeRekursivBoxes(boxList, fhandle, buffer);	
}

void freeWBoxes(void)
{
	WindBox *pb,*cp;
	
	pb = boxList;
	while(pb != NULL)
	{
		cp = pb;
		pb = pb->nextbox;
		free(cp);
	}
}
