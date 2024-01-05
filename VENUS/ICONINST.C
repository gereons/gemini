/*
 * @(#) Gemini\Iconinst.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: Installation von Icons auf dem Desktop
 *
 */

/* Schrittweite der Vergrîûerung des Desktopbaums
 */
#define DESKINCR		10

/* defaultmÑûige Farbe fÅr Hintergrundicons
 */
#define DEFCOLOR		0x10

#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <flydial\flydial.h>
#include <flydial\listman.h>
#include <nls\nls.h>

#include "vs.h"
#include "instbox.rh"
#include "trashbox.rh"
#include "scrapbox.rh"
#include "shredbox.rh"

#include "venuserr.h"
#include "iconinst.h"
#include "iconrule.h"
#include "myalloc.h"
#include "util.h"
#include "fileutil.h"
#include "redraw.h"
#include "select.h"
#include "stand.h"
#include "gemtrees.h"
#include "scut.h"
#include "color.h"

store_sccs_id(iconinst);

/* externals
 */
extern word deskx,desky,deskw,deskh;
extern DeskInfo NewDesk;
extern WindInfo wnull;
extern ShowInfo show;
extern OBJECT *pinstbox,*pshredbox,*ptrashbox,*pscrapbox;
extern char bootpath[MAXLEN];
extern word deskAnz, handle;

/*
 * internal strings
 */
#define NlsLocalSection "G.iconinst"
enum NlsLocalText{
T_ERASER,	/*REISSWOLF*/
T_PAPER,	/*PAPIERKORB*/
T_CLIP,		/*KLEMMBRETT*/
T_DRIVE,	/*Sie mÅssen einen Laufwerksbuchstaben zwischen 
'A' und 'Z' angeben!*/
};
 
/*
 * void copyNewDesk(OBJECT *pnewdesk)
 * switch RSC-NewDesk on own array, so we can fumble around
 * with it
 */
void copyNewDesk(OBJECT *pnewdesk)
{
	NewDesk.maxobj = NewDesk.objanz = countObjects(pnewdesk,0);
	if(NewDesk.objanz < DESKINCR)
		NewDesk.maxobj = DESKINCR;
	
	NewDesk.tree = (OBJECT *)malloc(NewDesk.maxobj * sizeof(OBJECT));
	
	memcpy(NewDesk.tree,pnewdesk,NewDesk.objanz * sizeof(OBJECT));
}

/*
 * static word getHiddenObject(OBJECT *tree)
 * get number of first HIDETREE Object in tree
 * or -1 if none exists
 */
static word getHiddenObject(OBJECT *tree)
{
	word index = 1;
	
	if(tree[0].ob_head != -1)
	{
		do
		{
			if(tree[index].ob_flags & HIDETREE)
				return index;
			index = tree[index].ob_next;
		} while(index != 0);			/* back at root of tree */
	}
	return -1;
}

/*
 * add an icon to the object tree for window 0
 */
static void addDeskIcon(word obx, word oby, word todraw,
						OBJECT *po, ICONBLK *pi)
{
	OBJECT *tmp;
	word objnr, oldnext;
	
	if((objnr = getHiddenObject(NewDesk.tree)) > 0)
	{	
		/* ein hidden Objekt gefunden
		 */
		oldnext = NewDesk.tree[objnr].ob_next;
		memcpy(&NewDesk.tree[objnr], po,sizeof(OBJECT));
		NewDesk.tree[objnr].ob_next = oldnext;
	}
	else 
	{
		/* Array vergrîûern
		 */
		if(NewDesk.objanz == NewDesk.maxobj)
		{
			NewDesk.maxobj += DESKINCR;
			tmp = (OBJECT *)malloc(NewDesk.maxobj * sizeof(OBJECT));
			if(tmp == NULL)
			{
				/* nicht genug Speicher
				 */
				NewDesk.maxobj -= DESKINCR;
				return;	
			}
			memcpy(tmp, NewDesk.tree, NewDesk.objanz * sizeof(OBJECT));
			free(NewDesk.tree);
			NewDesk.tree = tmp;
			wnull.tree = NewDesk.tree;
			wind_set(0, WF_NEWDESK, wnull.tree, 0);
		}
		memcpy(&NewDesk.tree[NewDesk.objanz], po, sizeof(OBJECT));
	
		/* Tree war leer
		 */
		if(NewDesk.tree[0].ob_head == -1)
		{
			objnr = NewDesk.objanz;
			NewDesk.tree[0].ob_head = objnr;
		}
		else
		{
			/* fÅge es in die Kette von Objekten ein
			 */
			objnr = NewDesk.tree[0].ob_tail;
			objnr = NewDesk.tree[objnr].ob_next = NewDesk.objanz;
		}
		NewDesk.tree[0].ob_tail = objnr;
		NewDesk.tree[objnr].ob_next = 0;
		NewDesk.tree[objnr].ob_head = -1;
		NewDesk.tree[objnr].ob_tail = -1;
	}

	NewDesk.objanz = wnull.objanz = countObjects(NewDesk.tree, 0);
	NewDesk.tree[objnr].ob_x = obx;
	NewDesk.tree[objnr].ob_y = oby;
	NewDesk.tree[objnr].ob_spec.iconblk = pi;
	
	/* Setze die Breite der Iconfahne
	 */
	SetIconTextWidth(pi);
	
	/* Plaziere es im sichtbaren Bereich
	 */
	deskAlign(&NewDesk.tree[objnr]);

	sortTree(NewDesk.tree, 0, 1, D_SOUTH, D_WEST);
	
	/* Markiere das letzte Objekt mit LASTOB
	 */
	SetLastObject(NewDesk.tree);
	
	if(todraw)
	{
		redrawObj(&wnull,objnr);
		flushredraw();
	}
}

/*
 * install icon for scrapdir on desktop at position obx,oby
 * with name 'name'
 */
void instScrapIcon(word obx, word oby, const char *name,
					word shortcut, char truecolor)
{
	IconInfo *pi;
	OBJECT *po;
	
	if((pi = (IconInfo *)malloc(sizeof(IconInfo))) != NULL)
	{
		strcpy(pi->magic,"IcoN");
		pi->type = DI_SCRAPDIR;
		pi->defnumber = 4;
		pi->altnumber = 5;
		pi->shortcut = shortcut;
		pi->truecolor = truecolor;
		SCutInstall(pi->shortcut);

		getSPath(pi->path);
		getLabel(pi->path[0] - 'A',pi->label);

		if(isEmptyDir(pi->path))
			po = getDeskObject(pi->defnumber);
		else
			po = getDeskObject(pi->altnumber);
			
		memcpy(&pi->ib,po->ob_spec.iconblk,sizeof(ICONBLK));
		strcpy(pi->iconname,name);
		pi->ib.ib_ptext = pi->iconname;
		pi->ib.ib_char = (pi->ib.ib_char & 0xFF)
							| (ColorMap(truecolor)<<8);

		addDeskIcon(obx, oby, FALSE, po, &pi->ib);
		NewDesk.scrapNr = getScrapIcon();
	}
}

/*
 * install shredder icon on desktop
 */
void instShredderIcon(word obx, word oby, const char *name, char truecolor)
{
	IconInfo *pi;
	OBJECT *po;
	
	if((pi = (IconInfo *)malloc(sizeof(IconInfo))) != NULL)
	{
		strcpy(pi->magic, "IcoN");
		pi->type = DI_SHREDDER;
		pi->defnumber = 1;
		pi->altnumber = 0;
		pi->shortcut = 0;
		pi->truecolor = truecolor;
		
		po = getDeskObject(pi->defnumber);
		memcpy(&pi->ib, po->ob_spec.iconblk, sizeof(ICONBLK));
		strcpy(pi->iconname,name);
		pi->path[0] = '\0';
		pi->ib.ib_ptext = pi->iconname;
		pi->ib.ib_char = (pi->ib.ib_char & 0xFF)
							| (ColorMap(truecolor)<<8);
		addDeskIcon(obx, oby, FALSE, po, &pi->ib);
	}
}

/*
 * install icon for paperbasket on desktop
 */
void instTrashIcon(word obx, word oby, const char *name,
					word shortcut, char truecolor)
{
	IconInfo *pii;
	OBJECT *po;
	
	if((pii = (IconInfo *)malloc(sizeof(IconInfo))) != NULL)
	{
		strcpy(pii->magic,"IcoN");
		pii->type = DI_TRASHCAN;
		pii->defnumber = 2;
		pii->altnumber = 3;
		pii->shortcut = shortcut;
		pii->truecolor = truecolor;
		SCutInstall(pii->shortcut);
		
		strcpy(pii->iconname,name);

		getTPath(pii->path);
		getLabel(pii->path[0] - 'A',pii->label);

		if(isEmptyDir(pii->path))
			po = getDeskObject(pii->defnumber);
		else
			po = getDeskObject(pii->altnumber);
		memcpy(&pii->ib, po->ob_spec.iconblk, sizeof(ICONBLK));

		pii->ib.ib_ptext = pii->iconname;
		pii->ib.ib_char = (pii->ib.ib_char & 0xFF)
							| (ColorMap(truecolor)<<8);
		addDeskIcon(obx, oby, FALSE, po, &pii->ib);
		NewDesk.trashNr = getTrashIcon();
	}
}

/*
 * fill IconInfo structure, type will be FILESYSTEM
 */ 
static void setDriveIcon(IconInfo *pii, word iconNr, char drive,
					 const char *name, word shortcut, OBJECT **ppo,
					 char truecolor)
{
	OBJECT *po;
	
	pii->type = DI_FILESYSTEM;
	pii->defnumber = iconNr;
	pii->altnumber = 0;
	pii->shortcut = shortcut;
	pii->truecolor = truecolor;
	SCutInstall(pii->shortcut);
	
	po = getDeskObject(pii->defnumber);
	
	memcpy(&pii->ib, po->ob_spec.iconblk, sizeof(ICONBLK));
	strcpy(pii->iconname, name);
	pii->ib.ib_char = ((word)drive)|(ColorMap(truecolor)<<8);
	pii->path[0] = drive;
	pii->path[1] = ':';
	pii->path[2] = '\\';
	pii->path[3] = '\0';
	pii->label[0] = '\0';			/* Label ist egal */
	pii->ib.ib_ptext = pii->iconname;
	
	*ppo = po;	/* gib das Object zur Installation zurÅck */
}

/*
 * install icon for a drive on desktop
 */
void instDriveIcon(word obx,word oby,word todraw,word iconNr,
					char drive, const char *name, word shortcut,
					char truecolor)
{
	IconInfo *pi;
	OBJECT *po;
	
	if((pi = (IconInfo *)malloc(sizeof(IconInfo))) != NULL)
	{
		strcpy(pi->magic,"IcoN");
		setDriveIcon(pi, iconNr, drive, name, shortcut, &po,
						truecolor);
		addDeskIcon(obx, oby, todraw, po, &pi->ib);
	}
}

/*
 * install icon for program on desktop
 */
void instPrgIcon(word obx,word oby,word todraw,word normicon,
				word isfolder,char* path,const char *name,char *label,
				word shortcut)
{
	IconInfo *pi;
	OBJECT *po;
	word save;
	char fname[MAX_FILENAME_LEN];
	
	if((pi = (IconInfo *)malloc(sizeof(IconInfo))) != NULL)
	{
		char color;
		
		getBaseName(fname,path);
		strcpy(pi->magic,"IcoN");
		pi->type = (isfolder)? DI_FOLDER : DI_PROGRAM;
		pi->defnumber = normicon;	/* groû oder klein */
		pi->altnumber = 0;
		pi->shortcut = shortcut;
		SCutInstall(pi->shortcut);
		
		save = show.normicon;
		show.normicon = normicon;
		po = getIconObject(isfolder, fname, &color);
		show.normicon = save;
		memcpy(&pi->ib, po->ob_spec.iconblk, sizeof(ICONBLK));
		strcpy(pi->iconname, name);
		strcpy(pi->path,path);
		pi->ib.ib_ptext = pi->iconname;
		pi->ib.ib_char = (color << 8) | (pi->ib.ib_char & 0x00FF);

		if(label)
			strcpy(pi->label,label);
		else
			pi->label[0] = '\0';

		addDeskIcon(obx, oby, todraw, po, &pi->ib);
	}
}

/*
 * void addDefIcons(void)
 * install default icon on desktop, called if there is
 * nothing in venus.inf
 */
void addDefIcons(void)
{
	OBJECT *obp;
	
	obp = getStdDeskIcon();
	
	instShredderIcon(0, 0, NlsStr(T_ERASER), DEFCOLOR);
	instTrashIcon(0, obp->ob_height, NlsStr(T_PAPER), 0, DEFCOLOR);
	instScrapIcon(0, 2 * obp->ob_height, NlsStr(T_CLIP), 0, DEFCOLOR);
	instDriveIcon(0, 3 * obp->ob_height, FALSE, 6, bootpath[0],
					DEFAULTDEVICE, 0, DEFCOLOR);
}

/*
 * void removeDeskIcon(word objnr)
 * remove an icon from desktop object tree
 */
void removeDeskIcon(word objnr)
{
	IconInfo *pii;
	
	pii = getIconInfo(&wnull.tree[objnr]);
	if (pii)
	{
		SCutRemove(pii->shortcut);
		free(pii);
	}
	NewDesk.tree[objnr].ob_flags |= HIDETREE;
	NewDesk.tree[objnr].ob_type = 0;		/* Object has no type */
	NewDesk.tree[objnr].ob_state = 0;		/* and no state */
	NewDesk.tree[objnr].ob_spec.free_string = NULL;
	
	NewDesk.objanz = wnull.objanz = countObjects(wnull.tree,0);
}

/*
 * static void drawDeskIcon(...)
 * display icon in listdialog
 */
static void drawDeskIcon(LISTSPEC *l, word x, word y, word offset,
				GRECT *clip, word how)
{
	word nr;
	MFDB screen;
	OBJECT *po, o;
	word len, xy[8];
	
	(void)how;
	if (!l)
		return;
		
	nr = (word)(l->entry);
	
		RectGRECT2VDI(clip, xy);
		vs_clip(handle, 1, xy);
	
	screen.fd_addr = 0L;
	memcpy (&xy[4], xy, 4*sizeof(int));	
	vro_cpyfm (handle, (l->flags.selected)? ALL_BLACK : ALL_WHITE,
				xy, &screen, &screen); 

	po = getDeskObject(nr);
	memcpy(&o, po, sizeof(OBJECT));
	len = (pinstbox[INSTSPEC].ob_width - o.ob_width) / 2;
	o.ob_x = x + offset + len;
	len = (pinstbox[INSTSPEC].ob_height - o.ob_height) / 2;
	o.ob_y = y + 1 + len;
	o.ob_head = o.ob_tail = o.ob_head = -1;
	objc_draw(&o, 0, 0, clip->g_x, clip->g_y, clip->g_w, clip->g_h);

	vs_clip (handle, 0, xy);
}


static LISTSPEC *buildDeskIconList(word selindex)
{
	LISTSPEC *l;
	word i, max = deskAnz - 7;
	
	l = tmpcalloc(max+1, sizeof(LISTSPEC));
	if (!l)
		return NULL;
	
	for (i = 0; i < max; ++i)
	{
		l[i].next = &l[i+1];
		l[i].entry = (void *)(i+6);
	}
	l[max].next = NULL;
	l[max].entry = (void *)(max+6);
	
	/* setze selektiert */
	if (selindex <= max)
		l[selindex].flags.selected = 1;
	
	return l;
}

/*
 * static void doDriveInstDialog(IconInfo *pii,word objnr)
 * dialog for installing drive icons
 */
static void doDriveInstDialog(IconInfo *pii, word objnr)
{
	DIALINFO d;
	LISTINFO L;
	LISTSPEC *list;
	static char drive[2] = "";
	static char name[13] = "";
	word goticon;
	word retcode, clicks, shortcut = 0;
	static word defx = 0, defy = 0;
	long listresult, iconNr = 6L;
	word check, draw, exit;
	word origscut, retcut, circle;
	char color;
	int edit_object = INSTDRV;
	
	pinstbox[INSTNAME].ob_spec.tedinfo->te_ptext = name;
	pinstbox[INSTDRV].ob_spec.tedinfo->te_ptext = drive;

	goticon = (pii != NULL);
	setDisabled(pinstbox, INSTDEL, !goticon);
	if(goticon)
	{
		strcpy(name,pii->iconname);
		*drive = *pii->path;
		iconNr = pii->defnumber;
		shortcut = pii->shortcut;
		color = pii->truecolor;
	}
	else
		color = 0x10;
	
	list = buildDeskIconList((word)iconNr-6);
	if (!list)
		return;
	
	origscut = shortcut;
	pinstbox[INSTSC].ob_spec = SCut2Obspec(shortcut);
	ColorSetup(color, pinstbox, INSTFORE, INSTFBOX,
							INSTBACK, INSTBBOX);
	
	DialCenter(pinstbox);

	ListStdInit(&L, pinstbox, INSTB, INSTBG, drawDeskIcon, 
				list, 0, iconNr-6, 1);
	ListInit(&L);

	DialStart(pinstbox,&d);
	DialDraw(&d);
	ListDraw(&L);
	
	exit = FALSE;
	do
	{
		circle = check = draw = FALSE;
		
		retcode = DialDo (&d, &edit_object);
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;

		switch(retcode)
		{
			case INSTB:
			case INSTBG:
				listresult = ListClick(&L, clicks);
				if (listresult >= 0)
				{
					iconNr = listresult + 6L;
					check = exit = (clicks == 2);
				}
				break;
			case INSTSC0:
				circle = TRUE;
			case INSTSC:
			case INSTSCSC:
				retcut = SCutSelect(pinstbox, INSTSC, shortcut,
									 origscut, circle);
				if (retcut != shortcut)
				{
					shortcut = retcut;
					pinstbox[INSTSC].ob_spec = SCut2Obspec(shortcut);
					fulldraw(pinstbox, INSTSC);
				}
				break;
			case INSTFR0:
				circle = TRUE;
			case INSTFGSC:
			case INSTFORE:
				color = ColorSelect(color, TRUE, pinstbox,
				 			INSTFORE, INSTFBOX, circle);
				 break;
			case INSTBK0:
				circle = TRUE;
			case INSTBACK:
			case INSTBGSC:
				color = ColorSelect(color, FALSE, pinstbox,
				 			INSTBACK, INSTBBOX, circle);
				break;
			case INSTOK:
				check = TRUE;
			default:
				draw = exit = TRUE;
				break;
		}

		if (check)
		{
			drive[0] = toupper(drive[0]);
			if ((drive[0] < 'A') || (drive[0] > 'Z'))
			{
				venusErr(NlsStr(T_DRIVE));
				exit = FALSE;
			}
		}
		
		if (draw)
		{				
			setSelected(pinstbox,retcode,FALSE);
			fulldraw(pinstbox,retcode);
		}

	} while(!exit);

	ListExit(&L);
	tmpfree(list);
	DialEnd(&d);
	
	if (check)
	{		
		/* versuche es zu installieren
		 */
		if(goticon)
		{
			OBJECT *po;
			
			redrawObj(&wnull, objnr);

			if (shortcut != origscut)
			{
				SCutRemove(origscut);
			}
			
			setDriveIcon(pii, (word)iconNr, drive[0], name,
						 shortcut, &po, color);
			wnull.tree[objnr].ob_width = po->ob_width;
			wnull.tree[objnr].ob_height = po->ob_height;
			deskAlign(&wnull.tree[objnr]);
			SetIconTextWidth(&pii->ib);
			
			redrawObj(&wnull, objnr);
			flushredraw();
		}
		else
		{
			/* installiere das neue Objekt
			 */
			instDriveIcon(defx, defy, TRUE, (word)iconNr,
						 drive[0], name, shortcut, color);
		}
	}
	else if(retcode == INSTDEL)
	{
		word r[4];
		
		desWindObjects(&wnull,0);
		objc_offset(wnull.tree,objnr,&r[0],&r[1]);
		defx = r[0] - deskx;
		defy = r[1] - desky;
		r[2] = wnull.tree[objnr].ob_width;
		r[3] = wnull.tree[objnr].ob_height;
		
		removeDeskIcon(objnr);
		redraw(&wnull,r);
	}
}

void doDefDialog(IconInfo *pii, word objnr, OBJECT *tree,
					const word nameindex, const word okindex, 
					const word foreindex, const word fboxindex,
					const word backindex, const word bboxindex,
					const word scindex, const word forecirc,
					const word backcirc, const word sccirc,
					const word forescsc, const word backscsc,
					const word shortscsc)
{
	DIALINFO d;
	char dialogname[MAX_FILENAME_LEN];
	word retcode, color, exit;
	word shortcut, circle;
	int edit_object = nameindex;
	

	strcpy(dialogname, pii->iconname);
	tree[nameindex].ob_spec.tedinfo->te_ptext = dialogname;
	
	shortcut = pii->shortcut;
	if (scindex > 0)
		tree[scindex].ob_spec = SCut2Obspec(shortcut);
	
	color = pii->truecolor;
	ColorSetup(color, tree, foreindex, fboxindex,
							backindex, bboxindex);
	
	DialCenter(tree);
	DialStart(tree, &d);
	DialDraw(&d);
	
	do
	{
		circle = FALSE;
		exit = TRUE;
		
		retcode = DialDo(&d, &edit_object) & 0x7FFF;
		setSelected(tree, retcode, FALSE);
		
		if (retcode == forecirc)
		{
			retcode = foreindex;
			circle = TRUE;
		}
		
		if ((retcode == foreindex) || (retcode == forescsc))
		{
			color = ColorSelect(color, TRUE, tree,
			 			foreindex, fboxindex, circle);
			 exit = FALSE;
		}
		
		if (retcode == backcirc)
		{
			retcode = backindex;
			circle = TRUE;
		}
		
		if ((retcode == backindex) || (retcode == backscsc))
		{
			color = ColorSelect(color, FALSE, tree,
			 			backindex, bboxindex, circle);
			 exit = FALSE;
		}
		
		if (retcode == sccirc)
		{
			retcode = scindex;
			circle = TRUE;
		}
		
		if ((retcode == scindex) || (retcode == shortscsc))
		{
			word retcut;
			
			retcut = SCutSelect(tree, scindex, 
								shortcut, pii->shortcut, circle);
			if (retcut != shortcut)
			{
				shortcut = retcut;
				tree[scindex].ob_spec = SCut2Obspec(shortcut);
				fulldraw(tree, scindex);
			}
			 exit = FALSE;
		}
	}
	while (!exit);
	
	DialEnd(&d);

	if(retcode == okindex)
	{
		strcpy(pii->iconname, dialogname);
		pii->shortcut = shortcut;
		pii->truecolor = color;
		pii->ib.ib_char = (pii->ib.ib_char & 0xFF)
							| (ColorMap(color)<<8);
		SetIconTextWidth(&pii->ib);
		redrawObj(&wnull,objnr);
		flushredraw();
	}
}

void doShredderDialog(IconInfo *pii, word objnr)
{
	doDefDialog(pii, objnr, pshredbox, SHREDNAM, SHREDOK,
			SHREFORE, SHREFBOX, SHREBACK, SHREBBOX,
			-1, SHREFR0, SHREBK0, -1, SHRDFGSC, SHRDBGSC, -1);
}

/*
 * void doInstDialog(void)
 * do dialog for installing or editing deskicons
 */
void doInstDialog(void)
{
	WindInfo *wp;
	IconInfo *pii;
	word objnr;
	
	if(getOnlySelected(&wp,&objnr) && (wp->kind == WK_DESK))
	{
		pii = getIconInfo(&wp->tree[objnr]);
		switch(pii->type)
		{
			case DI_FILESYSTEM:
				doDriveInstDialog(pii, objnr);
				break;
			case DI_SHREDDER:
				doShredderDialog(pii, objnr);
				break;
			case DI_TRASHCAN:
				doDefDialog(pii, objnr, ptrashbox, TRASHNAM, TRASHOK,
						TRASFORE, TRASFBOX, TRASBACK, TRASBBOX,
						TRASHSC, TRASFR0, TRASBK0, TRASHSC0,
						TRSHFGSC, TRSHBGSC, TRSHSCSC);
				break;
			case DI_SCRAPDIR:
				doDefDialog(pii, objnr, pscrapbox, SCRAPNAM, SCRAPOK,
						SCRAFORE, SCRAFBOX, SCRABACK, SCRABBOX,
						SCRAPSC, SCRAFR0, SCRABK0, SCRAPSC0,
						SCRPFGSC, SCRPBGSC, SCRPSCSC);
				break;
			default:
				doDriveInstDialog(NULL,0);
		}
	}
	else
		doDriveInstDialog(NULL,0);
}

/*
 * write informations about current desktopicons
 * to file *fp (probably venus.inf)
 */
word writeDeskIcons(word fhandle, char *buffer)
{
	IconInfo *pii;
	word i,obx,oby, write;
	
	for(i=wnull.tree[0].ob_head; i>0; i=wnull.tree[i].ob_next)
	{
		if(!(wnull.tree[i].ob_flags & HIDETREE)
			&& wnull.tree[i].ob_type == G_ICON)
		{
			pii = getIconInfo(&wnull.tree[i]);
			if (!pii)
				continue;
			objc_offset(wnull.tree,i,&obx,&oby);
			obx = (word)scale123(1000L,obx - deskx,deskw);
			oby = (word)scale123(1000L,oby - desky,deskh);

			write = TRUE;
			switch(pii->type)
			{
				case DI_FILESYSTEM:
					sprintf(buffer, "#D@%d@%d@%d@%c@%s@%d@%d",
						obx, oby, pii->defnumber, pii->path[0],
						pii->iconname, pii->shortcut,
						pii->truecolor);
					break;
				case DI_TRASHCAN:
					sprintf(buffer, "#T@%d@%d@%s@%d@%d", obx, oby,
							pii->iconname, pii->shortcut,
							(word)pii->truecolor);
					break;
				case DI_SHREDDER:
					sprintf(buffer, "#E@%d@%d@%s@%d", obx, oby,
							strlen(pii->iconname)? pii->iconname:" ",
							(word)pii->truecolor);
					break;
				case DI_SCRAPDIR:
					sprintf(buffer, "#S@%d@%d@%s@%d@%d", obx, oby,
							pii->iconname, pii->shortcut,
							(word)pii->truecolor);
					break;
				case DI_FOLDER:
				case DI_PROGRAM:
					sprintf(buffer, "#P@%d@%d@%d@%d@%s@%s@%s@%d",
							obx,oby,pii->defnumber,
							(pii->type == DI_FOLDER),
							pii->path, pii->iconname,
							strlen(pii->label)? pii->label : " ",
							pii->shortcut);
					break;
				default:
					write = FALSE;
					break;
			}
			if (write)
			{
				if (!Fputs(fhandle, buffer))
					return FALSE;
			}
		}
	}
	
	return TRUE;
}

/*
 * void instDraggedIcons(WindInfo *fwp,
 * 				word fromx,word fromy,word tox,word toy);
 * installiere gedraggte Icons auf dem Desktophintergrund
 */
void instDraggedIcons(WindInfo *fwp,
				word fromx,word fromy,word tox,word toy)
{
	OBJECT *stdobj;
	FileInfo *pf;
	word i,obx,oby,startx,starty;
	word curx,cury;
	char path[MAXLEN];
	char first = TRUE;
	
	if (fwp->kind != WK_FILE)
		return;
		
	stdobj = getStdFileIcon();
	
	for(i=fwp->tree[0].ob_head; i>0; i=fwp->tree[i].ob_next)
	{
		if(fwp->tree[i].ob_type & DRAG)
		{
			pf = getfinfo(&fwp->tree[i]);
			strcpy(path,fwp->path);

			if(first)
			{
				first = FALSE;
				objc_offset(fwp->tree,i,&obx,&oby);
				curx = obx = tox + (obx - fromx) - deskx;
				cury = oby = toy + (oby - fromy) - desky;
				starty = fwp->tree[i].ob_y;
				startx = fwp->tree[i].ob_x;
				
				if(pf->attrib & FA_FOLDER)
					addFolderName(path,pf->fullname);
				else
					addFileName(path,pf->fullname);
					
				instPrgIcon(obx,oby,TRUE,show.normicon,
							pf->attrib & FA_FOLDER,
							path,pf->fullname,fwp->label, 0);
			}
			else
			{
				if(starty == fwp->tree[i].ob_y)
				{
					curx += stdobj->ob_width;
				}
				else
				{
					starty = fwp->tree[i].ob_y;
					if(startx != fwp->tree[i].ob_x)
					{						/* andere Spalten */
						word diff,times;
						
						diff = startx - fwp->tree[i].ob_x;
						times = diff / (fwp->obw + fwp->xdist - 1);
						curx = obx - (times * stdobj->ob_width);
					}
					else
						curx = obx;
					cury += stdobj->ob_height;
				}
				
				if(pf->attrib & FA_FOLDER)
					addFolderName(path,pf->fullname);
				else
					addFileName(path,pf->fullname);

				instPrgIcon(curx,cury,TRUE,show.normicon,
							pf->attrib & FA_FOLDER,
							path,pf->fullname,fwp->label, 0);
			}
		}
	}
}

void freeDeskTree(void)
{
	IconInfo *pii;
	OBJECT *po;
	int i;
	
	po = NewDesk.tree;
	
	for(i=po[0].ob_head; i>0; i=po[i].ob_next)
	{
		if (po[i].ob_flags & HIDETREE)
			continue;
		if (po[i].ob_type == G_ICON)
		{
			pii = getIconInfo(&po[i]);
			if (pii)
				free(pii);
		}
	}
	free(po);
	wnull.tree = NewDesk.tree = NULL;
	wnull.objanz = NewDesk.maxobj = 0;
	wnull.selectAnz = NewDesk.objanz = 0;
	NewDesk.selectAnz = 0;
}

void rehashDeskIcon(void)
{
	word i, newchar;
	ICONBLK *pib;
	OBJECT *po;
	IconInfo *pii;
	char fname[MAX_FILENAME_LEN], color;
	
	for (i=wnull.tree[0].ob_head; i>0; i=wnull.tree[i].ob_next)
	{
		if ((wnull.tree[i].ob_type != G_ICON)
			|| ((pii = getIconInfo(&wnull.tree[i])) == NULL)
			|| ((pii->type != DI_FOLDER) && (pii->type != DI_PROGRAM)))
			continue;
			
		getBaseName(fname,pii->path);
		if (pii->defnumber)
		{
			po = getBigIconObject(pii->type == DI_FOLDER, fname,
									&color);
		}
		else
		{
			po = getSmallIconObject(pii->type == DI_FOLDER, fname,
									&color);
		}
		
		pib = po->ob_spec.iconblk;
		newchar = (color << 8) | (pib->ib_char & 0x00FF);
		
		if ((pib->ib_pmask != pii->ib.ib_pmask)
			|| (pib->ib_pdata != pii->ib.ib_pdata)
			|| (newchar != pii->ib.ib_char))
		{
			redrawObj(&wnull,i);

			memcpy(&pii->ib, pib, sizeof(ICONBLK));
			pii->ib.ib_ptext = pii->iconname;
			pii->ib.ib_char = newchar;
			wnull.tree[i].ob_width = po->ob_width;
			wnull.tree[i].ob_height = po->ob_height;

			/* Im sichtbaren Bereich
			 */
			deskAlign(&wnull.tree[i]);
			/* Fahnenbreite anpassen
			 */
			SetIconTextWidth(&pii->ib);

			redrawObj(&wnull,i);
		}
	}
	flushredraw();
}

void DeskIconNewLabel(char drive, const char *oldlabel, 
						const char *newlabel)
{
	word i;
	IconInfo *pii;
	
	for (i=wnull.tree[0].ob_head; i>0; i=wnull.tree[i].ob_next)
	{
		if ((wnull.tree[i].ob_type != G_ICON)
			|| ((pii = getIconInfo(&wnull.tree[i])) == NULL))
			continue;
		
		if ((drive == pii->path[0])	
			&& (!strcmp(pii->label, oldlabel)))
		{
			strcpy(pii->label, newlabel);
		}
	}
}