/*
 * @(#) Gemini\undo.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: functions to undo/redo things
 */

#include <string.h>

#include "vs.h"
#include "menu.rh"

#include "undo.h"
#include "window.h"
#include "redraw.h"
#include "windstak.h"
#include "fileutil.h"
#include "venuserr.h"

store_sccs_id(undo);

#define UNDOWOPEN	0x0001
#define UNDOICON	0x0002
#define UNDOWMOVE	0x0004
#define UNDOPATH	0x0008
#define UNDOWFULL	0x0010

/* externals
 */
extern OBJECT *pmenu;


struct UndoWOpen
{
	word whandle;
	word x,y,w,h,slpos,kind;
	char wildcard[13];
	char label[13];
	char path[MAXLEN];
	char title[MAXLEN];
};

struct UndoWMove
{
	word whandle;		/* handle des windows */
	word wasSized;		/* merken, ob sizen oder nur moven */
	word r[4];			/* rechteck des windows */
};

struct UndoPath
{
	word whandle;		/* handle des windows */
	word yskip;		/* position des vertikalen Sliders */
	char folder[MAX_FILENAME_LEN];	/* name of directory  */
};

struct UndoIcon
{
	word x,y;
	word type;
	word iconNr;
	char iconname[MAX_FILENAME_LEN];
	char path[MAXLEN];
};

static struct UndoInfo
{
	word type;				/* typ der structure in der union */
	word isUndo;			/* Undo oder Redo */
	union
	{
		struct UndoWOpen wo;	/* ™ffnen und Schliežen von windows */
		struct UndoWMove wm;	/* Verschieben von windows */
		struct UndoPath p;		/* cd undo */
		struct UndoIcon i;		/* De/Installieren von Icons */
	}u;
}ui;

static word ignore = FALSE;

void storeWOpenUndo(word open,WindInfo *wp)
{
	struct UndoWOpen *p = &ui.u.wo;

	if (ignore || (wp->kind == WK_ACC))
		return;
		
	ui.isUndo = open;
	ui.type = UNDOWOPEN;
	p->slpos = wp->vslpos;
	p->kind = wp->kind;
	p->x = wp->windx;
	p->y = wp->windy;
	p->w = wp->windw;
	p->h = wp->windh;
	p->whandle = wp->handle;
	strcpy(p->path,wp->path);
	strcpy(p->title,wp->title);
	strcpy(p->wildcard,wp->wildcard);	
	strcpy(p->label,wp->label);
	menu_ienable(pmenu,DOUNDO,TRUE);
}

void storeWFullUndo(WindInfo *wp)
{
	struct UndoWMove *p = &ui.u.wm;
	
	if (ignore)
		return;
		
	ui.type = UNDOWFULL;
	p->whandle = wp->handle;
	menu_ienable(pmenu, DOUNDO, TRUE);
}

void storeWMoveUndo(WindInfo *wp,word wasSized)
{
	struct UndoWMove *p = &ui.u.wm;
	
	if (ignore)
		return;
		
	ui.type = UNDOWMOVE;
	p->whandle = wp->handle;
	p->wasSized = wasSized;
	p->r[0] = wp->windx;
	p->r[1] = wp->windy;
	p->r[2] = wp->windw;
	p->r[3] = wp->windh;
	menu_ienable(pmenu, DOUNDO, TRUE);
}

void storePathUndo(WindInfo *wp,const char *folder,word isUndo)
{
	struct UndoPath *p = &ui.u.p;
	
	if (ignore)
		return;
		
	ui.type = UNDOPATH;
	ui.isUndo = isUndo;
	p->whandle = wp->handle;
	p->yskip = wp->yskip;
	strcpy(p->folder, folder);
	menu_ienable(pmenu, DOUNDO, TRUE);
}

static void undoWOpen(void)
{
	struct UndoWOpen *p = &ui.u.wo;
	word foobar;
	
	if(ui.isUndo)
	{
		deleteWindow(p->whandle);
	}
	else
	{
		openWindow(p->x,p->y,p->w,p->h,p->slpos,p->path,
					p->wildcard,p->title,p->label,p->kind);
		if (p->kind == WK_FILE)
			popWindBox(&foobar,&foobar,&foobar,&foobar);
		wind_get(0,WF_TOP,&p->whandle);
	}
}

static void undoWMove(void)
{
	WindInfo *wp;
	struct UndoWMove *p = &ui.u.wm;
	word r[4];

	r[0] = p->r[0];
	r[1] = p->r[1];
	if(p->wasSized)
	{
		r[2] = p->r[2];
		r[3] = p->r[3];
		sizeWindow(p->whandle,r);
	}
	else
	{
		if ((wp = getwp(p->whandle)) != NULL)
		{
			r[2] = wp->windw;
			r[3] = wp->windh;
			moveWindow(wp->handle, r);
		}
	}
}

static void undoWFull(void)
{
	struct UndoWMove *p = &ui.u.wm;

	fullWindow(p->whandle);
}

static void undoPath(void)
{
	struct UndoPath *p = &ui.u.p;
	WindInfo *wp;
	word lastskip;
	
	if((wp = getwp(p->whandle)) != NULL)
	{
		if(ui.isUndo)
		{
			stripFolderName(wp->title);
			stripFolderName(wp->path);
			lastskip = wp->yskip;
			wp->yskip = p->yskip;
			fileChanged(wp);
			p->yskip = lastskip;
			ui.isUndo = FALSE;
		}
		else 
		{
			addFolderName(wp->title,p->folder);
			addFolderName(wp->path,p->folder);
			lastskip = wp->yskip;
			wp->yskip = p->yskip;
			fileChanged(wp);
			p->yskip = lastskip;
			ui.isUndo = TRUE;
		}
	}
}

static void undoIconInst(void)
{
	if(ui.isUndo)
	{
	}
	else
	{
	}
}

void doUndo(void)
{
	if (ignore)
		return;
		
	switch(ui.type)
	{
		case UNDOWOPEN:
			undoWOpen();
			break;
		case UNDOWMOVE:
			undoWMove();
			break;
		case UNDOWFULL:
			undoWFull();
			break;
		case UNDOPATH:
			undoPath();
			break;
		case UNDOICON:
			undoIconInst();
			break;
		default:
			venusDebug("unbekannter Undotyp!");
			break;
	}
}

/*
 * void clearUndo(void)
 * clear undo disable menu
 */
void clearUndo(void)
{
	ui.type = 0;
	menu_ienable(pmenu,DOUNDO,FALSE);
}

/*
 * void ignoreUndos(word yesno)
 * switches the ignorance of all undo functions
 */
void ignoreUndos(word yesno)
{
	ignore = yesno;
}

void UndoFolderVanished(const char *path)
{
	struct UndoPath *p = &ui.u.p;
	WindInfo *wp;
	
	if (ui.type != UNDOPATH) 
		return;

	if((wp = getwp(p->whandle)) != NULL)
	{
		if(!ui.isUndo)
		{
			char dir[MAXLEN];
			
			strcpy(dir, wp->path);
			if (path[strlen(path)-1] != '\\')
				addFileName(dir, p->folder);
			else
				addFolderName(dir, p->folder);
			if (!strcmp(dir, path))
				clearUndo();
		}
	}
}
