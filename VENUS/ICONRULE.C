/*
 * @(#) Gemini\Iconrule.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: functions to administrate icons
 *
 */
 
#include <stdlib.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\listman.h>
#include <nls\nls.h>

#include "vs.h"
#include "rulebox.rh"
#include "stdbox.rh"
#include "ruleedit.rh"

#include "iconrule.h"
#include "myalloc.h"
#include "util.h"
#include "venuserr.h"
#include "iconhash.h"
#include "wildcard.h"
#include "redraw.h"
#include "color.h"
#include "select.h"
#include "fileutil.h"
#include "files.h"

store_sccs_id(iconrule);


#define TEXTOFFSET		80

 
/*
 * internal texts
 */
#define NlsLocalSection "G.iconrule"
enum NlsLocalText{
T_NOMEMEDIT,	/*Es ist nicht genug Speicher vorhanden, um die 
Regeln zu editieren!*/
T_ICONS,		/*Es sind zuwenig Icons in der Datei %s.*/
T_ICONNR,		/*Die Icon-Information in der Regel %s stimmt 
nicht. Das normale Icon wird dafÅr eingesetzt.*/
};


#define DEFAULTFOLDER	2
#define DEFAULTDATA		1
#define DEFAULTDRIVE	6

#define DESKTREE	0		/* trees in venusicn.h */
#define FILENORM	1
#define FILEMINI	2

/* externals
 */
extern ShowInfo show;
extern OBJECT *prulebox, *pstdbox, *pruleedit;
extern word deskx,desky,deskw,deskh;
extern word deskAnz,normAnz,miniAnz;

/* internals
 */
/* Breite eines Zeichens in der Fahne eines Icons
 */
static word oneCharWidth;

/* trees from file 'GEMINIIC.RSC'
 */
static OBJECT *pdeskicon,*pnormfile,*pminifile;	

/* Die grîûten Objekte im normalen und kleinen Iconbaum
 */
static OBJECT StdBigIcon, StdMiniIcon;

/* Standardregeln
 */
static DisplayRule stdDataRule = {NULL, NULL, NULL, FALSE, "*",
									FALSE, DEFAULTDATA, 0x10, 0x10};
static DisplayRule stdFoldRule = {NULL, NULL, NULL, FALSE, "*",
									TRUE, DEFAULTFOLDER, 0x10, 0x10};

/* AufhÑnger der Regel-fÅr-Icons-Liste
 */
static DisplayRule *firstRule = NULL;


/* ermittle das grîûte Icon in tree und speichere es in po
 */
static void setStdIcon(OBJECT *tree, word anz, OBJECT *po)
{
	word i;
	*po = tree[1];

	for (i = 2; i < anz; ++i)
	{
		if (po->ob_width < tree[i].ob_width)
			po->ob_width = tree[i].ob_width;

		if (po->ob_height < tree[i].ob_height)
			po->ob_height = tree[i].ob_height;
	}
}

/* Setze die Breite der Textfahne des Icons der LÑnge
 * des Textes entsprechend
 */
void SetIconTextWidth(ICONBLK *pib)
{
	word center;
	
	center = pib->ib_xtext + (pib->ib_wtext / 2);
	
	pib->ib_wtext = ((word)strlen(pib->ib_ptext)+1) * oneCharWidth;
	pib->ib_xtext = center - (pib->ib_wtext / 2);
}

static void setBarHeight(OBJECT *tree)
{
	word height = pstdbox[STDICON].ob_spec.iconblk->ib_htext;
	word i, diff;
	
	for (i=tree[0].ob_head; i > 0; i = tree[i].ob_next)
	{
		SetIconTextWidth(tree[i].ob_spec.iconblk);
		
		diff = height - tree[i].ob_spec.iconblk->ib_htext;
		tree[i].ob_spec.iconblk->ib_htext = height;
		diff = (tree[i].ob_spec.iconblk->ib_ytext 
				+ tree[i].ob_spec.iconblk->ib_htext) - tree[i].ob_height;
		if (diff > 0)
			tree[i].ob_height += diff;
	}
}

/* Bearbeitet die Icons im nachhineine noch ein biûchen, so daû
 * alle Fahnen gleich hoch sind und auch schon das grîûte Objekt
 * bekannt ist.
 */
void FixIcons(void)
{
	oneCharWidth = pstdbox[STDICON].ob_spec.iconblk->ib_wtext / 13;
	
	/* Setze die Hîhe der Icon-Fahnen */
	setBarHeight(pdeskicon);
	setBarHeight(pnormfile);
	setBarHeight(pminifile);
			
	/* hole die grîûten Objekte der BÑume */
	setStdIcon(pnormfile, normAnz, &StdBigIcon);
	setStdIcon(pminifile, normAnz, &StdMiniIcon);
}

/*
 * word initIcons(const char *name)
 * load file <name> and initialize the treepointers
 */
word initIcons(const char *name)
{
	word ok = TRUE;
	
	if(rsrc_load(name))
	{
		ok = rsrc_gaddr(R_TREE,DESKTREE,&pdeskicon);
		if(ok)
			ok = rsrc_gaddr(R_TREE,FILENORM,&pnormfile);
		if(ok)
			if (!rsrc_gaddr(R_TREE,FILEMINI,&pminifile)
				|| (pminifile == NULL))
				pminifile = pnormfile;
				
		if(ok)
		{
			firstRule = NULL;
			deskAnz = countObjects(pdeskicon,0);
			normAnz = countObjects(pnormfile,0);
			miniAnz = countObjects(pminifile,0);
			normAnz = miniAnz = min(normAnz,miniAnz);
			
			if((normAnz < 3)||(deskAnz < 7))
			{
				venusErr(NlsStr(T_ICONS), name);
				return FALSE;
			}
		}
		return ok;
	}
	else
		return FALSE;
}

/*
 * gibt die Regel fÅr den Datei-/Ordnernamen <fname> zurÅck
 */
static DisplayRule *getDisplayRule(word isFolder,char *fname)
{
	DisplayRule *actrule = firstRule, *pd;

	if ((pd = getHashedRule(isFolder, fname)) != NULL)
		return pd;
	else if(isFolder)
	{
		while (actrule != NULL)
		{
			if ((actrule->isFolder) && (!actrule->wasHashed)
				&&(filterFile(actrule->wildcard, fname)))
			{
				return actrule;
			}
			actrule = actrule->nextrule;
		}
	}
	else
	{
		while (actrule != NULL)
		{
			if ((!actrule->wasHashed) && (!actrule->isFolder)
				&&(filterFile(actrule->wildcard, fname)))
			{
				return actrule;
			}
			actrule = actrule->nextrule;
		}
	}
	return NULL;
}

/*
 * static word getIconNr(word isFolder,char *fname)
 * get number and color of the icon for file fname by looking at
 * the filerules
 */
static word getIconNr(word isFolder,char *fname, char *color)
{
	DisplayRule *rule;
	word iconnr;
	
	rule = getDisplayRule(isFolder, fname);
	
	if (!rule)
	{
		*color = 0x10;
		return (isFolder)? DEFAULTFOLDER : DEFAULTDATA;
	}

	iconnr = rule->iconNr;
	*color = rule->color;
	
	if(iconnr < normAnz)			/* gÅltige Iconnummer */
		return iconnr;
	else
	{
		venusDebug("IconNr zu hoch!");
		return (isFolder)? DEFAULTFOLDER : DEFAULTDATA;
	}
}

/*
 * return pointer to object of icon for file fname
 */
OBJECT *getIconObject(word isFolder, char *fname, char *color)
{
	OBJECT *pakt;
	word nr;
	
	pakt = (show.normicon)? pnormfile : pminifile;
	nr = getIconNr(isFolder, fname, color);

	return &pakt[nr];
}

OBJECT *getBigIconObject(word isFolder, char *fname, char *color)
{
	word nr;

	nr = getIconNr(isFolder, fname, color);
	return &pnormfile[nr];
}

OBJECT *getSmallIconObject(word isFolder, char *fname, char *color)
{
	word nr;

	nr = getIconNr(isFolder, fname, color);
	return &pminifile[nr];
}

/*
 * OBJECT *getStdFileIcon(void)
 * get object structure fo first object in icontree
 */
OBJECT *getStdFileIcon(void)
{
	return (show.normicon)? &StdBigIcon : &StdMiniIcon;
}



/*
 * static void drawFileIcon(...)
 * display icon in listdialog
 */
static void drawFileIcon(LISTSPEC *l, word x, word y, word offset,
				GRECT *clip, word how)
{
	word nr, xy[8];
	word len, smalloff;
	OBJECT o;
	MFDB screen;
	
	(void)how;
	if (!l)
		return;
	
	nr = (word)(l->entry);

	RectGRECT2VDI(clip, xy);
	vs_clip(DialWk, 1, xy);

	screen.fd_addr = 0L;
	memcpy (&xy[4], xy, 4*sizeof(int));	
	vro_cpyfm (DialWk, (l->flags.selected & 1)? ALL_BLACK : ALL_WHITE,
				 xy, &screen, &screen); 

	vs_clip (DialWk, 0, xy);

	/* offset zum zweiten Icon */
	smalloff = (pruleedit[RESPEC].ob_width / 3) * 2;
	
	memcpy(&o, &pnormfile[nr], sizeof(OBJECT));
	len = (smalloff - o.ob_width) / 2;
	o.ob_x = x + 1 - offset + len;
	len = (pruleedit[RESPEC].ob_height - o.ob_height) / 2;
	o.ob_y = y + 1 + len;
	o.ob_head = o.ob_tail = o.ob_head = -1;
	
	len = min(clip->g_w, smalloff);
	objc_draw(&o, 0, 0, clip->g_x, clip->g_y, len, clip->g_h);

	memcpy(&o, &pminifile[nr], sizeof(OBJECT));
	len = (pruleedit[RESPEC].ob_width - smalloff - o.ob_width) / 2;
	o.ob_x = x + 1 - offset + len + smalloff;
	len = (pruleedit[RESPEC].ob_height - o.ob_height) / 2;
	o.ob_y = y + 1 + len;
	o.ob_head = o.ob_tail = o.ob_head = -1;
	
	objc_draw(&o, 0, 0, clip->g_x, clip->g_y, clip->g_w, clip->g_h);
}


static LISTSPEC *buildFileIconList(word selindex)
{
	LISTSPEC *l;
	word i, maxindex = normAnz - 2;
	
	l = tmpcalloc(maxindex + 1, sizeof(LISTSPEC));
	if (!l)
		return NULL;
	
	for (i = 0; i < maxindex; ++i)
	{
		l[i].next = &l[i+1];
		l[i].entry = (void *)(i+1);
	}
	l[maxindex].next = NULL;
	l[maxindex].entry = (void *)(maxindex + 1);
	
	/* setze selektiert */
	if (selindex <= maxindex)
		l[selindex].flags.selected = 1;
	
	return l;
}

/*
 * edit a single DisplayRule and return if something
 * has changed
 */
static word editSingleRule(DisplayRule *rule)
{
	DIALINFO d;
	LISTINFO L;
	LISTSPEC *list;
	word iconNr, redraw;
	word retcode, clicks;
	long listresult;
	word draw, exit, circle;
	char *wildcard;
	char color;
	int edit_object = REWILD;
	
	iconNr = rule->iconNr - 1;
	wildcard = pruleedit[REWILD].ob_spec.tedinfo->te_ptext;
	strcpy(wildcard, rule->wildcard);
	setSelected(pruleedit, REFOLDER, rule->isFolder);
	
	list = buildFileIconList(iconNr);
	if (!list)
	{
		venusErr(NlsStr(T_NOMEMEDIT));
		return FALSE;
	}
	
	color = rule->truecolor;
	ColorSetup(color, pruleedit, RECOLFG,
				RECLFBOX, RECOLBG, RECLBBOX);
				
	DialCenter(pruleedit);

	ListStdInit(&L, pruleedit, REBOX, REBGBOX, drawFileIcon, 
				list, 0, iconNr, 1);
	ListInit(&L);

	redraw = !DialStart(pruleedit,&d);
	DialDraw(&d);
	ListDraw(&L);
	
	exit = FALSE;
	do
	{
		circle = draw = FALSE;
		
		retcode = DialDo(&d, &edit_object);
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;

		switch(retcode)
		{
			case REBOX:
			case REBGBOX:
				listresult = ListClick(&L, clicks);
				if (listresult >= 0)
				{
					iconNr = (word)listresult;
					exit = (clicks == 2);
					retcode = REOK;
				}
				break;
			case RECOLFG0:
				circle = TRUE;
			case RECOLFG:
			case REFGSC:
				color = ColorSelect(color, TRUE, pruleedit,
				 			RECOLFG, RECLFBOX, circle);
				break;
			case RECOLBG0:
				circle = TRUE;
			case RECOLBG:
			case REBGSC:
				color = ColorSelect(color, FALSE, pruleedit,
				 			RECOLBG, RECLBBOX, circle);
				break;
			default:
				draw = exit = TRUE;
				break;
		}

		if (draw)
		{				
			setSelected(pruleedit, retcode, FALSE);
			fulldraw(pruleedit, retcode);
		}

	} while(!exit);

	ListExit(&L);
	tmpfree(list);
	DialEnd(&d);
	
	if (retcode == REOK)
	{
		strupr(wildcard);
		strcpy(rule->wildcard, wildcard);
		rule->isFolder = isSelected(pruleedit, REFOLDER);
		rule->iconNr = iconNr + 1;
		rule->truecolor = color;
		rule->color = ColorMap(rule->truecolor);
		rule->wasHashed = FALSE;
		
		return redraw? -1 : 1;
	}
	return redraw? -1 : 0;
}


/*
 * insert a at start of the list of rules if prev is NULL
 * else insert after prev
 */
static DisplayRule *insertIconRule(DisplayRule **plist,
				 		DisplayRule *prev, DisplayRule *pdr)
{
	DisplayRule *pa;
	
	pa = tmpmalloc(sizeof(DisplayRule));
	if(!pa)
		return NULL;
	memcpy(pa, pdr, sizeof(DisplayRule));
	
	if (prev)
	{
		pa->nextrule = prev->nextrule;
		pa->prevrule = prev;
		if (pa->nextrule)
			pa->nextrule->prevrule = pa;
		prev->nextrule = pa;
	}
	else
	{
		pa->nextrule = *plist;
		pa->nextHash = pa->prevrule = NULL;
		if (*plist != NULL)
			(*plist)->prevrule = pa;
		*plist = pa;
	}
	pa->wasHashed = FALSE;
	
	return pa;
}

/*
 * delete the DisplayRule <pdr> from the list <plist>
 */
static word deleteIconRule(DisplayRule **plist, DisplayRule *pdr)
{
	if (pdr->prevrule)
		pdr->prevrule->nextrule = pdr->nextrule;
	else
		*plist = pdr->nextrule;
	
	if (pdr->nextrule)
		pdr->nextrule->prevrule = pdr->prevrule;
	
	tmpfree(pdr);
	return TRUE;
}

/*
 * static void freeIconRuleList(DisplayRule **liststart)
 * free list of rules starting at liststart
 */
static void freeIconRuleList(DisplayRule **liststart)
{
	DisplayRule *aktRule,*prevRule;
	
	aktRule = *liststart;
	*liststart = NULL;
	while(aktRule != NULL)
	{
		prevRule = aktRule;
		aktRule = aktRule->nextrule;
		tmpfree(prevRule);
	}
}

/*
 * void insDefIconRules(void)
 * insert the default rules (for *)
 */
void insDefIconRules(void)
{
	insertIconRule(&firstRule, NULL, &stdFoldRule);
	insertIconRule(&firstRule, NULL, &stdDataRule);
}

/*
 * void freeIconRules(void)
 * free the normal rulelist (starts at firstrule)
 */
void freeIconRules(void)
{
	clearHashTable();
	freeIconRuleList(&firstRule);
}

/* 
 * static DisplayRule *dupRules(DisplayRule *liststart)
 * duplicate a list of rules (for editing)
 */
static DisplayRule *dupRules(DisplayRule *liststart)
{
	DisplayRule *newlist,*pa,*prev;

	newlist = prev = NULL;
	while(liststart != NULL)
	{

		pa = (DisplayRule *)tmpmalloc(sizeof(DisplayRule));
		if(newlist == NULL)
			newlist = pa;
			
		if(pa == NULL)
			return NULL;
		memcpy(pa,liststart,sizeof(DisplayRule));
	
		if(prev)
		{
			pa->prevrule = firstRule;
			prev->nextrule = pa;
		}
		pa->prevrule = prev;
		pa->nextHash = pa->nextrule = NULL;
		pa->wasHashed = FALSE;
		prev = pa;
		liststart = liststart->nextrule;
	}

	return newlist;
}

static void freeListList(LISTSPEC *head)
{
	LISTSPEC *tmp;
	
	while (head)
	{
		tmp = head->next;
		tmpfree(head);
		head = tmp;
	}	
}

static LISTSPEC *buildListList(DisplayRule *head)
{
	LISTSPEC *start, **prev, *act;
	
	start = NULL;
	prev = &start;
	
	while (head)
	{
		act = tmpcalloc(1, sizeof(LISTINFO));
		if (!act)
		{
			freeListList(start);
			return NULL;
		}
		act->entry = head;
		*prev = act;
		prev = &act->next;
	
		head = head->nextrule;
	}
	
	return start;
}

/* selektiere die Regel, die fÅr ein selektiertes Objekt
 * zutrifft, wenn es ein solches gibt.
 */
LISTSPEC *selectListSpec(LISTSPEC *head)
{
	WindInfo *wp;
	word objnr;
	
	if (getOnlySelected(&wp, &objnr))
	{
		FileInfo *pfi;
		IconInfo *pii;
		word isFolder = FALSE;
		word found = FALSE;
		char filename[MAX_FILENAME_LEN];
		
		switch (wp->kind)
		{
			case WK_FILE:
				pfi = GetSelectedFileInfo(wp);
				if (pfi != NULL)
				{
					isFolder = pfi->attrib & FA_FOLDER;
					strcpy(filename, pfi->fullname);
					found = TRUE;
				}
				break;
			case WK_DESK:
				if ((pii = getIconInfo(&wp->tree[objnr])) != NULL)
				{
					switch(pii->type)
					{
						case DI_FOLDER:
							isFolder = TRUE;
						case DI_PROGRAM:
							getBaseName(filename, pii->path);
							found = TRUE;
							break;
					}
				}
				break;
		}
		
		if (found)
		{
			DisplayRule *orig, *new;
			
			if ((orig = getDisplayRule(isFolder, filename)) != NULL)
			{
				LISTSPEC *pls = head;

				while (pls)
				{
					new = pls->entry;
					if (!strcmp(new->wildcard, orig->wildcard)
						&& (new->isFolder == orig->isFolder))
					{
						pls->flags.selected = 1;
						return pls;
					}
					pls = pls->next;
				}
			}
		}
	}
	
	if (head)
		head->flags.selected = 1;
	
	return head;
}

/*
 * display Rule in listdialog
 */
static void drawRule(LISTSPEC *l, word x, word y, word offset,
				GRECT *clip, word how)
{
	DisplayRule *pr;
	OBJECT o;
	ICONBLK i;
	MFDB screen;
	word dummy, xy[8], width;
	char line[WILDLEN + 3];
	
	if (!l)
		return;
		
	(void)how;
	pr = l->entry;
	
	RectGRECT2VDI(clip, xy);
	vs_clip(DialWk, 1, xy);

	screen.fd_addr = 0L;
	memcpy (&xy[4], xy, 4*sizeof(int));	
	vro_cpyfm (DialWk, (l->flags.selected)? ALL_BLACK : ALL_WHITE,
				 xy, &screen, &screen); 


	memcpy(&o, &pnormfile[pr->iconNr], sizeof(OBJECT));
	width = (TEXTOFFSET - o.ob_width) / 2;
	o.ob_x = x + 1 - offset + width;
	width = (prulebox[RUSPEC].ob_height - o.ob_height) / 2;
	o.ob_y = y + 1 + width;
	
	memcpy(&i, o.ob_spec.iconblk, sizeof(ICONBLK));
	o.ob_spec.iconblk = &i;
	i.ib_char = (pr->color << 8) | (i.ib_char & 0x00FF);
	o.ob_head = o.ob_tail = o.ob_head = -1;
	
	width = min(clip->g_w, TEXTOFFSET);
	objc_draw(&o, 0, 0, clip->g_x, clip->g_y, width, clip->g_h);

	strcpy(line, "   ");
	if (pr->isFolder)
		line[1] = pstdbox[FISTRING].ob_spec.free_string[0];
	strcat(line, pr->wildcard);
	
	vswr_mode(DialWk, MD_TRANS);
	vst_color(DialWk, (l->flags.selected)? 0:1);
	vst_alignment(DialWk, 0, 5, &dummy, &dummy);
	vst_effects(DialWk, 0);
	v_gtext(DialWk, x-offset+TEXTOFFSET, y + HandBYSize, line);
	
	vst_color(DialWk, 1);
	vs_clip (DialWk, 0, xy);
}

/*
 * lîsche die DisplayRule <entry> in der Liste <head>, die alle
 * in der LISTSPEC-Liste <list> hÑngen.
 */
static LISTSPEC *deleteListSpec(LISTSPEC **list,
					DisplayRule **head, LISTSPEC *act)
{
	LISTSPEC *pl, **prevp, *prevspec;
	DisplayRule *entry = act->entry;
	
	pl = *list;
	prevp = list;
	prevspec = NULL;
	while (pl)
	{
		if (pl->entry == entry)
			break;
		
		prevp = &pl->next;
		prevspec = pl;
		pl = pl->next;
	}
	
	*prevp = pl->next;
	deleteIconRule(head, pl->entry);
	tmpfree(pl);
	
	if (*prevp)
		pl = (*prevp);
	else if (prevspec)
		pl = prevspec;
	else
		return NULL;
	
	pl->flags.selected = 1;
	
	return pl;
}

static LISTSPEC *addNewRule(LISTINFO *L, DisplayRule **head,
					 LISTSPEC *act, DisplayRule *rule)
{
	word bx, by, box[4];
	word tmpx, tmpy;
	long itemnr;
	LISTSPEC *pl;
	
	objc_offset(prulebox, RUBOX, &box[0], &box[1]);
	box[2] = prulebox[RUBOX].ob_width;
	box[3] = prulebox[RUBOX].ob_height;
	objc_offset(prulebox, RUNEW, &tmpx, &tmpy);
	
	graf_dragbox(prulebox[RUNEW].ob_width, prulebox[RUNEW].ob_height, 
			tmpx, tmpy, prulebox[0].ob_x, prulebox[0].ob_y,
			prulebox[0].ob_width, prulebox[0].ob_height,  &bx, &by);
	
	if (!pointInRect(bx, by, box))
		return act;
	
	objc_offset(prulebox, RUBOX, &tmpx, &tmpy);
	bx -= tmpx;
	by -= tmpy;
	
	tmpy = prulebox[RUSPEC].ob_height;
	itemnr = by / tmpy;
	if ((by % tmpy) < (tmpy / 2))
		--itemnr;
	
	pl = tmpcalloc(1, sizeof(LISTSPEC));
	if (!pl)
		return act;
		
	itemnr += L->startindex;
	pl->flags.selected = 1;
	
	if (itemnr < 0)
	{
		pl->entry = insertIconRule(head, NULL, rule);
		if (!pl->entry)
		{
			tmpfree(pl);
			return act;
		}
		pl->next = L->list;
		L->list = pl;
	}
	else
	{
		LISTSPEC *tmp;
		
		if (itemnr >= L->listlength)
			itemnr = L->listlength - 1;
		tmp = ListIndex2List(L->list, itemnr);
		
		pl->entry = insertIconRule(head, tmp->entry, rule);
		if (!pl->entry)
		{
			tmpfree(pl);
			return act;
		}
		pl->next = tmp->next;
		tmp->next = pl;
	}
	act->flags.selected = 0;
	
	return pl;
}

/*
 * word editRule(void)
 * let user edit the rules with dialog and such things
 * and return if something has changed
 */
word editIconRule(void)
{
	DIALINFO d;
	LISTINFO L;
	LISTSPEC *Llist, *actlist;
	DisplayRule *actualRule;
	DisplayRule myRule, *tmplist;
	word retcode, draw;
	word clicks, rulecopied;
	long listresult;
	
	if(firstRule == NULL)
	{
		venusDebug("got no Rules!");
		return FALSE;
	}
	
	tmplist = dupRules(firstRule);
	if (!tmplist)
	{
		venusErr(NlsStr(T_NOMEMEDIT));
		return FALSE;
	}
	
	actlist = Llist = buildListList(tmplist);
	if (!Llist)
	{
		freeIconRuleList(&tmplist);
		venusErr(NlsStr(T_NOMEMEDIT));
		return FALSE;
	}
	
	actlist = selectListSpec(Llist);
	actualRule = actlist->entry;
	
	setDisabled(prulebox, RUDEL, actualRule->nextrule == NULL);
	memcpy(&myRule,&stdDataRule,sizeof(DisplayRule));
	rulecopied = FALSE;

	DialCenter(prulebox);

	ListStdInit(&L, prulebox, RUBOX, RUBGBOX, drawRule, 
				Llist, 0, 0, 1);
	L.maxwidth = TEXTOFFSET + (HandXSize * (3+WILDLEN));
	L.hstep = HandXSize;
	ListInit(&L);
		
	WindUpdate(BEG_MCTRL);

	DialStart(prulebox,&d);
	DialDraw(&d);
	ListScroll2Selection(&L);
	ListDraw(&L);
	
	do
	{
		draw = TRUE;
		
		retcode = DialDo(&d, 0);
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;

		switch(retcode)
		{
			case RUBOX:
			case RUBGBOX:
				listresult = ListClick(&L, clicks);
				draw = FALSE;
				if (listresult >= 0)
				{
					actlist = ListIndex2List(L.list, listresult);
					actualRule = (DisplayRule *)(actlist->entry);
					if (clicks == 2)
					{
						draw = TRUE;
						retcode = RUEDIT;
						setSelected(prulebox, RUEDIT, TRUE);
						fulldraw(prulebox, RUEDIT);
					}
				}
				break;
			case RUINS:
				memcpy(&myRule, actualRule, sizeof(DisplayRule));
				if (!rulecopied)
					rulecopied = TRUE;
				break;
			case RUNEW:
				actlist = addNewRule(&L, &tmplist, actlist, &myRule);
				
				/* ist wirklich eingefÅgt worden?
				 */
				if (actlist->entry != actualRule)
				{
					actualRule = actlist->entry;
					if (isDisabled(prulebox, RUDEL))
					{
						setDisabled(prulebox, RUDEL, FALSE);
						fulldraw(prulebox,RUDEL);
					}
					ListExit(&L);
					ListInit(&L);
					ListScroll2Selection(&L);
					ListDraw(&L);
				}
				break;
			case RUDEL:
				memcpy(&myRule, actualRule, sizeof(DisplayRule));
				if (!rulecopied)
					rulecopied = TRUE;

				actlist = deleteListSpec(&L.list, &tmplist, actlist);
				actualRule = actlist->entry;

				if (tmplist->nextrule == NULL)
				{
					setDisabled(prulebox, RUDEL, TRUE);
					fulldraw(prulebox,RUDEL);
				}
				
				ListExit(&L);
				ListInit(&L);
				ListScroll2Selection(&L);
				ListDraw(&L);
				break;
		}
		
		if (retcode == RUEDIT)
		{
			word ret = editSingleRule(actualRule);
			
			if (ret)
			{
				if (ret < 0)
					DialDraw(&d);
				ListExit(&L);
				ListInit(&L);
				ListScroll2Selection(&L);
				ListDraw(&L);
			}
		}
		
		if (draw)
		{
			setSelected(prulebox, retcode, FALSE);
			fulldraw(prulebox, retcode);
		}
		
	} 
	while (retcode != RUOK && retcode != RUCANCEL);
	
	ListExit(&L);
	freeListList(L.list);
	DialEnd(&d);

	WindUpdate(END_MCTRL);

	if (retcode == RUOK)
	{
		actualRule = firstRule;
		firstRule = tmplist;
		freeIconRuleList(&actualRule);
		builtIconHash();
		return TRUE;
	}
	else
	{
		freeIconRuleList(&tmplist);
		return FALSE;
	}
}

/*
 * void addRule(char *line)
 * add a rule at start of list
 * line comes probably from venus.inf
 */
void addIconRule(const char *line)
{
	DisplayRule pd;
	char *cp,*tp;
	
	cp = tmpmalloc((strlen(line)+1) * sizeof(char));
	if(cp)
	{
		strcpy(cp,line);
		strtok(cp,"@\n");	/* skip first */
		if((tp = strtok(NULL,"@\n")) != NULL)
		{
			strcpy(pd.wildcard,tp);
			if((tp = strtok(NULL,"@\n")) != NULL)
			{
				pd.isFolder = atoi(tp);
				if((tp = strtok(NULL,"@\n")) != NULL)
				{
					pd.iconNr = atoi(tp);
					if((pd.iconNr > normAnz) || (pd.iconNr < 1))
					{
						venusInfo(NlsStr(T_ICONNR), pd.wildcard);
						pd.iconNr = pd.isFolder? 2 : 1;
					}
					
					if((tp = strtok(NULL,"@\n")) != NULL)
						pd.truecolor = (char)atoi(tp);
					else
						pd.truecolor = 0x10;

					pd.color = ColorMap(pd.truecolor);
					insertIconRule(&firstRule, NULL, &pd);
				}
			}
		}
		tmpfree(cp);
	}
}

/*
 * write to the list of ConfInfo structures the rule pd
 */
static ConfInfo *writeRekursiv (ConfInfo *aktconf, DisplayRule *pd,
					char *buffer)
{
	if (pd == NULL)
		return aktconf;

	aktconf = writeRekursiv(aktconf,pd->nextrule, buffer);
	
	if ((aktconf->nextconf = 
		(ConfInfo *)malloc (sizeof (ConfInfo))) != NULL)
	{
		sprintf (buffer, "#R@%s@%d@%d@%d", pd->wildcard, pd->isFolder,
					pd->iconNr, (word)pd->truecolor);

		aktconf->nextconf->line = malloc (strlen (buffer)+1);
		if (aktconf->nextconf->line)
		{
			aktconf = aktconf->nextconf;
			strcpy (aktconf->line, buffer);
		}
		else		/* malloc failed */
		{
			free (aktconf->nextconf);
		}

		aktconf->nextconf = NULL;
		
	}
	
	return aktconf;
}

/*
 * write rules to list of ConfInfos and return new start of list
 */
ConfInfo *makeIconConf(ConfInfo *aktconf, char *buffer)
{
	return writeRekursiv (aktconf, firstRule, buffer);
}

/*
 * OBJECT *getStdDeskIcon(void)
 * get object structure of standard deskicon
 */
OBJECT *getStdDeskIcon(void)
{
	return &pdeskicon[1];
}

/*
 * get pointer to deskicon nr
 */
OBJECT *getDeskObject(word nr)
{
	if(nr >= deskAnz)
		nr = DEFAULTDRIVE;
	return &pdeskicon[nr];
}

DisplayRule *getFirstDisplayRule(void)
{
	return firstRule;
}