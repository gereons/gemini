/*
 * @(#) Gemini\Appledit.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: functions to manage list of applications
 *
 */

#include <stdlib.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\listman.h>
#include <nls\nls.h>

#include "applbox.rh"
#include "appledit.rh"

#include "vs.h"
#include "appledit.h"
#include "util.h"
#include "fileutil.h"
#include "myalloc.h"
#include "venuserr.h"
#include "redraw.h"

store_sccs_id(appledit);

/* externals
 */
extern OBJECT *papplbox, *pappledit;

/* internal texts
 */
#define NlsLocalSection "G.appledit.c"
enum NlsLocalText{
T_MEMERR,	/*Die Operation muž leider aus Speichermagel 
abgebrochen werden!*/
T_NOAPPL,	/*Es sind keine Applikationen angemeldet! 
Selektieren Sie ein Programm, um dies zu tun.*/
T_CLIP,		/*Dieses Klemmbrett enth„lt die zuletzt von 
Ihnen gel”schte Regel. Momentan k”nnen Sie 
es nicht benutzen, da es leer ist.*/
};

/* internals
 */
ApplInfo *applList = NULL;



/*
 * find ApplInfo-structure by the application-name
 * return NULL, if nothing found
 */
ApplInfo *getApplInfo(ApplInfo *List, const char *name)
{
	ApplInfo *pai;
	
	pai = List;
	while(pai)
	{
		if(!strcmp(name,pai->name))
			return pai;
		pai = pai->nextappl;
	}
	return NULL;
}


/*
 * insert new Applinfo with given values
 */
word insertApplInfo(ApplInfo **List, ApplInfo *prev, ApplInfo *ai)
{
	ApplInfo *myai;
	
	myai = tmpmalloc(sizeof(ApplInfo));
	if(!myai)
	{
		venusErr(NlsStr(T_MEMERR));
		return FALSE;
	}

	*myai = *ai;

	if (prev)
	{
		myai->nextappl = prev->nextappl;
		prev->nextappl = myai;
	}
	else
	{
		myai->nextappl = *List;
		*List = myai;
	}
	
	return TRUE;
}


/*
 * remove an ApplInfo from the List
 */
word deleteApplInfo(ApplInfo **List, const char *name)
{
	ApplInfo *pai, **prevai;
	
	pai = *List;
	prevai = List;
	
	while(pai)
	{
		if(!strcmp(name,pai->name))
			break;
		prevai = &pai->nextappl;
		pai = pai->nextappl;
	}
	
	if(pai)
	{
		*prevai = pai->nextappl;
		tmpfree(pai);
	}
	else
	{
		venusDebug("can't find ApplInfo.");
		return FALSE;
	}
	return TRUE;
}


/* 
 * edit a single ApplInfo structure
 * return if OK was pressed
 */
word editSingleApplInfo(ApplInfo *pai)
{
	DIALINFO d;
	word startmode, retcode, redraw;
	char *pfilename, *pwildcard;
	char tmp[16];
	int edit_object = APPLSPEX;
	

	pfilename = pappledit[APPLNAME].ob_spec.tedinfo->te_ptext;
	pwildcard = pappledit[APPLSPEX].ob_spec.tedinfo->te_ptext;

	strcpy(tmp, pai->name);
	makeEditName(tmp);
	strcpy(pfilename, tmp);
	
	if (pai->startmode & GEM_START)
		pai->startmode |= WCLOSE_START;
	startmode = pai->startmode;
	strcpy(pwildcard, pai->wildcard);

	setDisabled(pappledit, APPLCLOS, startmode & GEM_START);
	setSelected(pappledit, APPLGEM, startmode & GEM_START);
	setSelected(pappledit, APPLCOMM, startmode & TTP_START);
	setSelected(pappledit, APPLCLOS, !(startmode & WCLOSE_START));
	setSelected(pappledit, APPLWAIT, startmode & WAIT_KEY);
	setSelected(pappledit, APPLOVL, startmode & OVL_START);
	
	DialCenter(pappledit);
	redraw = !DialStart(pappledit, &d);
	DialDraw(&d);
	
	do
	{
		retcode = DialDo(&d, &edit_object) & 0x7FFF;
		
		if (retcode == APPLGEM)
		{
			word isgem = isSelected(pappledit, APPLGEM);

			setDisabled(pappledit, APPLCLOS, isgem);
			if (isgem)
				setSelected(pappledit, APPLCLOS, FALSE);
			fulldraw(pappledit, APPLCLOS);
		}
	
	} while (retcode == APPLGEM);
	
	setSelected(pappledit, retcode, FALSE);
	DialEnd(&d);
	
	startmode = 0;
	if (isSelected(pappledit, APPLGEM))
		startmode |= GEM_START;
	if (isSelected(pappledit, APPLCOMM))
		startmode |= TTP_START;
	if (!isSelected(pappledit, APPLCLOS))
		startmode |= WCLOSE_START;
	if (isSelected(pappledit, APPLWAIT))
		startmode |= WAIT_KEY;
	if (isSelected(pappledit, APPLOVL))
		startmode |= OVL_START;

	if (retcode == APPLOK)
	{
		pai->startmode = startmode;
		if (*pwildcard == '@')
			pai->wildcard[0] = '\0';
		else
			strcpy(pai->wildcard, pwildcard);
		strupr(pai->wildcard);

		
		return redraw? -1 : 1;
	}
	return redraw? -1 : 0;
}


static void freeListmanList(LISTSPEC *list)
{
	LISTSPEC *pl;
	
	while (list)
	{
		if (list->entry)
			tmpfree(list->entry);
		pl = list->next;
		tmpfree(list);
		list = pl;
	}
}

static LISTSPEC *buildListmanList(ApplInfo *aList, ApplInfo *selai,
								LISTSPEC **psel)
{
	LISTSPEC *list, *pl, **prev;
	ApplInfo *pai, **pprevai, *head;
	word sel = FALSE;
	
	pprevai = &head;
	list = NULL;
	prev = &list;
	
	while (aList)
	{
		pl = tmpcalloc(1, sizeof(LISTSPEC));
		if (!pl)
		{
			freeListmanList(list);
			return NULL;
		}
		pai = tmpmalloc(sizeof(ApplInfo));
		if(!pai)
		{
			tmpfree(pl);
			freeListmanList(list);
			return NULL;
		}
		*prev = pl;
		pl->next = NULL;
		
		*pai = *aList;
		pl->entry = pai;
		*pprevai = pai;
		pprevai = &pai->nextappl;
		pai->nextappl = NULL;
		
		if (aList == selai)
		{
			pl->flags.selected = 1;
			sel = TRUE;
			*psel = pl;
		}
		
		prev = &pl->next;
		aList = aList->nextappl;
	}
	
	if (list && !sel)
	{
		list->flags.selected = 1;
		*psel = list;
	}

	return list;
}

/*
 * Zeichne die ApplInfo <entry>
 */
static void drawApplInfo(LISTSPEC *l, word x, word y, word offset,
				GRECT *clip, word how)
{
	ApplInfo *pai;
	word dummy, xy[8];
	
	if (!l)
		return;
		
	pai = l->entry;

	RectGRECT2VDI(clip, xy);
	vs_clip(DialWk, 1, xy);
	
	if (how & LISTDRAWREDRAW)
	{
		vswr_mode(DialWk, MD_REPLACE);
		vst_alignment(DialWk, 0, 5, &dummy, &dummy);
		vst_effects(DialWk, 0);
		v_gtext(DialWk, x-offset+HandXSize, y, pai->name);
		v_gtext(DialWk, x-offset+(MAX_FILENAME_LEN*HandXSize), 
			y, pai->wildcard);
	}
	
	if ((l->flags.selected & 1) || !(how & LISTDRAWREDRAW))
	{
		MFDB screen;
	
		screen.fd_addr = 0L;
		memcpy (&xy[4], xy, 4*sizeof(int));	
		vro_cpyfm (DialWk, D_INVERT, xy, &screen, &screen); 
	}
	
	vs_clip (DialWk, 0, xy);
}

static LISTSPEC *addNewAppl(LISTINFO *L, LISTSPEC *act,
						 ApplInfo *appl)
{
	word bx, by, box[4];
	word tmpx, tmpy;
	long itemnr;
	LISTSPEC *pl;
	
	if (!appl)
		return act;
		
	objc_offset(papplbox, APBOX, &box[0], &box[1]);
	box[2] = papplbox[APBOX].ob_width;
	box[3] = papplbox[APBOX].ob_height;
	objc_offset(papplbox, APNEW1, &tmpx, &tmpy);
	
	graf_dragbox(papplbox[APNEW1].ob_width, papplbox[APNEW1].ob_height, 
			tmpx, tmpy, papplbox[0].ob_x, papplbox[0].ob_y,
			papplbox[0].ob_width, papplbox[0].ob_height,  &bx, &by);
	
	if (!pointInRect(bx, by, box))
		return act;
	
	objc_offset(papplbox, APBOX, &tmpx, &tmpy);
	bx -= tmpx;
	by -= tmpy;
	
	tmpy = papplbox[APSPEC].ob_height;
	itemnr = by / tmpy;
	if ((by % tmpy) < (tmpy / 2))
		--itemnr;
	
	pl = tmpcalloc(1, sizeof(LISTSPEC));
	if (!pl)
		return act;
		
	itemnr += L->startindex;
	pl->flags.selected = 1;
	pl->entry = appl;
	
	if ((itemnr < 0) || (L->listlength < 1))
	{
		pl->next = L->list;
		L->list = pl;
	}
	else
	{
		LISTSPEC *tmp;
		
		if (itemnr >= L->listlength)
			itemnr = L->listlength - 1;
		tmp = ListIndex2List(L->list, itemnr);
		
		pl->next = tmp->next;
		tmp->next = pl;
	}
	
	if (act)
		act->flags.selected = 0;
	
	return pl;
}

/*
 * l”sche den LISTSPEC <act> im LISTINFO <L>
 */
static LISTSPEC *deleteListSpec(LISTINFO *L, LISTSPEC *act)
{
	LISTSPEC *pl, **prevp, *prevspec;
	
	pl = L->list;
	prevp = &L->list;
	prevspec = NULL;
	while (pl)
	{
		if (pl == act)
			break;
		
		prevp = &pl->next;
		prevspec = pl;
		pl = pl->next;
	}
	
	*prevp = act->next;
	tmpfree(act);
	
	if (*prevp)
		pl = (*prevp);
	else if (prevspec)
		pl = prevspec;
	else
		return NULL;
	
	pl->flags.selected = 1;
	
	return pl;
}

void FreeApplList(ApplInfo **list)
{
	ApplInfo *pai, *next;
	
	pai = *list;
	*list = NULL;
	
	while(pai)
	{
		next = pai->nextappl;
		tmpfree(pai);
		pai = next;
	}
}

static void makeNewList(ApplInfo **head, LISTSPEC *list)
{
	ApplInfo **prev;
	
	FreeApplList(head);

	prev = head;
	while (list)
	{
		*prev = list->entry;
		prev = &(((ApplInfo *)list->entry)->nextappl);
		list->entry = NULL;
		
		list = list->next;
	}
	
	*prev = NULL;
}

static void switchClip(word full)
{
	word obx, oby, item;
	
	setHideTree(papplbox, APNEW1, full);
	setHideTree(papplbox, APNEW2, !full);
	
	item = full? APNEW2 : APNEW1;
	objc_offset(papplbox, item, &obx, &oby);
	ObjcDraw(papplbox, 0, MAX_DEPTH, obx, oby,
				papplbox[item].ob_width, papplbox[item].ob_height);
	
}

static word editApplList(ApplInfo *pai)
{
	DIALINFO D;
	LISTINFO L;
	LISTSPEC *Llist, *actlist;
	ApplInfo *clipAppl, *actAppl;
	word retcode, draw;
	word clicks;
	long listresult;
	
	if(applList == NULL)
	{
		venusInfo(NlsStr(T_NOAPPL));
		return FALSE;
	}
	
	Llist = buildListmanList(applList, pai, &actlist);
	if (!Llist)
	{
		venusErr(NlsStr(T_MEMERR));
		return FALSE;
	}
	actAppl = actlist->entry;
	
	clipAppl = NULL;
	setHideTree(papplbox, APNEW1, FALSE);
	setHideTree(papplbox, APNEW2, TRUE);
	setDisabled(papplbox, APDEL, FALSE);
	setDisabled(papplbox, APEDIT, FALSE);

	DialCenter(papplbox);

	ListStdInit(&L, papplbox, APBOX, APBGBOX, drawApplInfo, 
				Llist, 0, 0, 1);
	L.maxwidth = HandXSize * (WILDLEN + 15);
	L.hstep = HandXSize;
	ListInit(&L);
	
	WindUpdate(BEG_MCTRL);
	
	DialStart(papplbox,&D);
	DialDraw(&D);
	ListScroll2Selection(&L);
	ListDraw(&L);
	
	do
	{
		draw = TRUE;
		
		retcode = DialDo(&D, 0);
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;

		switch(retcode)
		{
			case APBOX:
			case APBGBOX:
				listresult = ListClick(&L, clicks);
				draw = FALSE;
				if (listresult >= 0)
				{
					actlist = ListIndex2List(L.list, listresult);
					actAppl = actlist->entry;
					if (clicks == 2)
					{
						draw = TRUE;
						retcode = APEDIT;
						setSelected(papplbox, APEDIT, TRUE);
						fulldraw(papplbox, APEDIT);
					}
				}
				break;
			case APNEW1:
			case APNEW2:
				if (clipAppl)
				{
					actlist = addNewAppl(&L, actlist, clipAppl);
				
					/* ist wirklich eingefgt worden?
					 */
					if (actlist && (actlist->entry != actAppl))
					{
						switchClip(FALSE);
						clipAppl = NULL;
						
						if (isDisabled(papplbox, APDEL))
						{
							setDisabled(papplbox, APDEL, FALSE);
							fulldraw(papplbox, APDEL);
							setDisabled(papplbox, APEDIT, FALSE);
							fulldraw(papplbox, APEDIT);
						}

						actAppl = actlist->entry;
						ListExit(&L);
						ListInit(&L);
						ListScroll2Selection(&L);
						ListDraw(&L);
					}
				}
				else
					venusInfo(NlsStr(T_CLIP));
				break;
			case APDEL:
				if (clipAppl)
				{
					tmpfree(clipAppl);
					clipAppl = NULL;
				}
				else
				{
					switchClip(TRUE);
				}

				clipAppl = actAppl;

				actlist = deleteListSpec(&L, actlist);

				if (actlist)
					actAppl = actlist->entry;
				else
				{
					actAppl = NULL;
					setDisabled(papplbox, APDEL, TRUE);
					fulldraw(papplbox, APDEL);
					setDisabled(papplbox, APEDIT, TRUE);
					fulldraw(papplbox, APEDIT);
				}

				ListExit(&L);
				ListInit(&L);
				ListScroll2Selection(&L);
				ListDraw(&L);
				break;
		}
		
		if (retcode == APEDIT)
		{
			word ret = editSingleApplInfo(actAppl);
			
			if (ret)
			{
				if (ret < 0)
					DialDraw(&D);
				ListExit(&L);
				ListInit(&L);
				ListScroll2Selection(&L);
				ListDraw(&L);
			}
		}
		
		if (draw)
		{
			setSelected(papplbox, retcode, FALSE);
			fulldraw(papplbox, retcode);
		}
		
	} 
	while (retcode != APOK && retcode != APCANCEL);
	
	ListExit(&L);
	DialEnd(&D);

	WindUpdate(END_MCTRL);

	if (clipAppl)			/* noch was im Clipboard? */
		tmpfree(clipAppl);
	
	if (retcode == APOK)
	{
		makeNewList(&applList, L.list);
	}

	freeListmanList(L.list);
	
	return (retcode == APOK);
}

/*
 * make dialog for application installation
 */
void EditApplList(const char *name, const char *path,
					const char *label, word defstartmode)
{
	ApplInfo myai, *pai;
	word newrule;
	
	pai = NULL;
	if (name)
	{
		pai = getApplInfo(applList, name);

		newrule = (pai == NULL);
	
		if (newrule)
		{
			strcpy(myai.path, path);
			strcpy(myai.name, name);
			strcpy(myai.label, label);
			myai.wildcard[0] = '\0';
			myai.nextappl = NULL;
			myai.startmode = defstartmode;
			pai = &myai;
		}

		if (editSingleApplInfo(pai))
		{
			if (newrule)
			{
				if (!insertApplInfo(&applList, NULL, pai))
					venusDebug("inserting appl failed!");
			}
		}
		else
			return;
	}
	
	editApplList(pai);
}

