/*
 * @(#) Gemini\util.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: different functions 
 */

#include <tos.h>
#include <stddef.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\evntevnt.h>

#include "vs.h"
#include "menu.rh"

#include "util.h"
#include "fileutil.h"
#include "myalloc.h"
#include "redraw.h"
#include "venuserr.h"
#include "iconrule.h"
#include "stand.h"

store_sccs_id(util);

/* externals
 */
extern WindInfo wnull;
extern DeskInfo NewDesk;
extern word deskx,desky,deskw,deskh,wchar;
extern word handle;
extern OBJECT *pmenu;
#if MERGED
extern struct CommInfo CommInfo;
#endif

/* internals
 */
#define MAXPATHS	32
static char *pathStack[MAXPATHS];
static word pathStackTop = 0;

/* calc p1 * p2 / p3 with long arithmetic
 */
long scale123(register long p1,register long p2,register long p3)
{
	p2 = p1 *= p2;
	p1 /= p3;
	if(p2 && ((p2 % p3) * 2) >= p3)
		p1++;
	return p1; 
}

/*
 * MUPFEL_ONLY:
 * 	0 wenn's auch mit anderen Shells gehen soll
 * 	1 fr Checks ob Mupfel wirklich da ist
 */
#define MUPFEL_ONLY 0

#define _SHELL_P ((long *)0x4f6L)

#if MUPFEL_ONLY
#define SHELL_OK	((do_sys!=NULL) && (!strncmp(xbra_id,"XBRAGMNI",8) \
						|| !strncmp(xbra_id,"XBRAMUPF",8)))
#else

#define SHELL_OK	(do_sys!=NULL)

#endif

/*
 * word system(const char *cmd)
 * Fhrt ein Kommando ber die in _shell_p installierte Shell aus.
 * Ohne Shell gibt's -1 als Returnwert, ansonsten
 * den Returncode des ausgefhrten Kommandos.
 * Die Mupfel-interne Routine erwartet den Pointer auf die Kommando-
 * zeile auf dem Stack und gibt den Returncode des ausgefhrten
 * Kommandos in Register D0.W zurck.
 */
word system(const char *cmd)
{
	/* Parameter auf dem Stack bergeben! */
	word cdecl (*do_sys)(const char *cmd);
#if MUPFEL_ONLY
	char *xbra_id;
#endif
	long oldssp = Super(0L);
	
	do_sys = (void (*))*_SHELL_P;
	Super((void *)oldssp);

#if MUPFEL_ONLY
	xbra_id = (char *)((long)do_sys - 12L);
#endif

	if (cmd==NULL)
		return SHELL_OK;

#if MERGED
	CommInfo.errmsg = NULL;
#endif

	if (SHELL_OK)
		return do_sys(cmd);
	else
		return -1;
}


void fulldraw(OBJECT *tree,word objnr)
{
	word obx,oby;
	
	objc_offset(tree,objnr,&obx,&oby);
	ObjcDraw(tree,objnr,MAX_DEPTH,obx,oby,
				tree[objnr].ob_width,tree[objnr].ob_height);
}

void pathUpdate(char *path1,char *path2)
{
	IconInfo *pii;
	WindInfo *wp;
	
	wp = wnull.nextwind;
	while(wp)
	{
		if ((wp->kind == WK_FILE) &&
			(*path1 && strstr(wp->path,path1))
			||(*path2 && strstr(wp->path,path2)))
		{
			fileChanged(wp);
		}
		wp = wp->nextwind;
	}
	if(NewDesk.scrapNr > 0)
	{
		pii = getIconInfo(&wnull.tree[NewDesk.scrapNr]);
		if((*path1 && strstr(pii->path,path1))
			||(*path2 && strstr(pii->path,path2)))
			updateSpecialIcon(NewDesk.scrapNr);
	}
	if(NewDesk.trashNr > 0)
	{
		pii = getIconInfo(&wnull.tree[NewDesk.trashNr]);
		if((*path1 && strstr(pii->path,path1))
			||(*path2 && strstr(pii->path,path2)))
			updateSpecialIcon(NewDesk.trashNr);
	}
}

/*
 * void deskAlign(OBJECT *deskobj)
 * align an object lying on the desktop so that it 
 * fits perfectly well in this area
 */
void deskAlign(OBJECT *deskobj)
{
	word zw,zh;
	
	if(deskobj->ob_x < 0)
		deskobj->ob_x  = 0;
	if(deskobj->ob_y < 0)
		deskobj->ob_y  = 0;
	
	zw = deskw - (deskobj->ob_x + deskobj->ob_width);
	zh = deskh - (deskobj->ob_y + deskobj->ob_height);
	
	if(zw < 0)
		deskobj->ob_x += zw;
	if(zh < 0)
		deskobj->ob_y += zh;
}

/*
 * char *getDraggedNames(WindInfo *wp)
 * built string of dragged Files with Path
 * string must be freed after use!!!
 */
char *getDraggedNames(WindInfo *wp)
{
	FileInfo *pf;
	IconInfo *pii;
	char *cp;
	word i, nr;
	size_t len, pathlen;
	
	pathlen = strlen(wp->path);
	
	nr = 0;
	len = 1;
	for (i = wp->tree[0].ob_head; i > 0; i = wp->tree[i].ob_next)
	{
		if ((wp->tree[i].ob_type != (G_USERDEF|DRAG))
			&& (wp->tree[i].ob_type != (G_ICON|DRAG)))
			continue;
		nr++;
		switch(wp->kind)
		{
			case WK_FILE:
				pf = getfinfo(&wp->tree[i]);
				len += pathlen + strlen(pf->fullname) + 3;
				if (pf->attrib & FA_FOLDER)
					++len;
				break;
			case WK_DESK:
				pii = getIconInfo(&wp->tree[i]);
				if (strlen(pii->path))
					len += strlen(pii->path) + 3;
				break;
		}
	}		/* now we got the size */
	
	cp = malloc(len * sizeof(char));
	
	if(cp)
	{
		*cp = '\0';
		for (i = wp->tree[0].ob_head; 
			(i > 0) && nr; i = wp->tree[i].ob_next)
		{
			if ((wp->tree[i].ob_type != (G_USERDEF|DRAG))
				&& (wp->tree[i].ob_type != (G_ICON|DRAG)))
				continue;
			nr--;
			switch(wp->kind)
			{
				case WK_FILE:
					pf = getfinfo(&wp->tree[i]);
					strcat(cp,"\'");
					strcat(cp,wp->path);
					strcat(cp,pf->fullname);
					if (pf->attrib & FA_FOLDER)
						strcat(cp,"\\\'");
					else
						strcat(cp,"\'");
					break;
				case WK_DESK:
					pii = getIconInfo(&wp->tree[i]);
					if (strlen(pii->path))
					{
						strcat(cp,"\'");
						strcat(cp,pii->path);
						strcat(cp,"\'");
					}
					break;
			}
			strcat(cp," ");
		}
	}
	
	return cp;
}

word GotBlitter(void)
{
	return Blitmode(-1) & 0x0002;
}

word SetBlitter(word mode)
{
	word flag;
	
	flag = Blitmode(-1);
	switch (mode)
	{
		case -1:
			return flag & 0x01;
		case 0:
			flag &= ~0x01;
			break;
		default:
			flag |= 0x01;
			break;
	}
	Blitmode(flag);
	return TRUE;
}

/*
 * word killEvents(word eventtypes, word maxtimes)
 * kill pending events like MU_MESAG or MU_KEYBD
 * at most maxtimes times, return times used
 */
word killEvents(word eventtypes, word maxtimes)
{
	MEVENT E;
	word times, events;
	word messbuff[8];
	
	if(eventtypes & (MU_TIMER|MU_M1|MU_M2|MU_BUTTON))
		return 0;			/* this events can't be pending */
	E.e_flags = (eventtypes|MU_TIMER);
	E.e_time = 0L;
	E.e_mepbuf = messbuff;
	times = 0;
		
	do
	{
		events = evnt_event(&E);
		times++;
		
	} while((events & eventtypes) && (times < maxtimes));
	
	return times;
}

void WaitKeyButton(void)
{
	MEVENT E;
	
	E.e_flags = (MU_BUTTON|MU_KEYBD);
	E.e_bclk = 2;
	E.e_bmsk = E.e_bst = 1;
	evnt_event(&E);
}

word ButtonPressed(word *mx, word *my, word *bstate, word *kstate)
{
	MEVENT E;
	word etype;
	
	E.e_flags = (MU_BUTTON|MU_TIMER);
	E.e_bclk = 2;
	E.e_bmsk = 1;
	E.e_bst = 1;
	E.e_time = 0L;
	etype = evnt_event(&E);
	*kstate = E.e_ks;
	*mx = E.e_mx;
	*my = E.e_my;
	if (etype & MU_BUTTON)
	{
		*bstate = E.e_mb;
		return TRUE;
	}
	else
	{
		*bstate = 0;
		return FALSE;
	}
}

/*
 * word escapeKeyPressed(void)
 * return, if the escape key was pressed
 */
word escapeKeyPressed(void)
{
	MEVENT E;
	word events;
	
	E.e_flags = (MU_TIMER|MU_KEYBD);
	E.e_time = 0L;
	
	events = evnt_event(&E);

	if(events & MU_KEYBD)
		return ((E.e_kr & 0xFF) == 0x1B);
	else
		return FALSE;
}

static word getSpecialDeskIcon(word type)
{
	IconInfo *pii;
	register word i;
	
	for(i = wnull.tree[0].ob_head; i > 0; i = wnull.tree[i].ob_next)
	{
		pii = getIconInfo(&wnull.tree[i]);
		if(pii->type == type)
			return i;
	}
	return -1;		/* -1 is an illegal number */
}

/*
 * word getScrapIcon(void)
 * get objectnumber for scrapicon
 * or -1 if not found
 */
word getScrapIcon(void)
{
	return getSpecialDeskIcon(DI_SCRAPDIR);
}

/*
 * word getTrashIcon(void)
 * get objectnumber for scrapicon
 * or -1 if not found
 */
word getTrashIcon(void)
{
	return getSpecialDeskIcon(DI_TRASHCAN);
}

/*
 * word updateSpecialIcon(word iconNr)
 * for Scrap- and Trashicons: look if Dir is empty
 * return icon changed
 */
word updateSpecialIcon(word iconNr)
{
	OBJECT *po;
	ICONBLK *ibp;
	IconInfo *pii;
	
	if(iconNr > 0)
	{
		po = &wnull.tree[iconNr];
		pii = getIconInfo(po);
		if (!pii)
			return FALSE;
			
		if ((pii->type != DI_TRASHCAN) && (pii->type != DI_SCRAPDIR))
			return FALSE;
			
		if(isEmptyDir(pii->path))
			po = getDeskObject(pii->defnumber);
		else
			po = getDeskObject(pii->altnumber);
		ibp = po->ob_spec.iconblk;

		if(pii->ib.ib_pdata != ibp->ib_pdata) /* verschiedene Icons*/
		{
			word c;
			
			redrawObj(&wnull,iconNr);

			c = pii->ib.ib_char;
			memcpy(&pii->ib,ibp,sizeof(ICONBLK));
			pii->ib.ib_ptext = pii->iconname;
			pii->ib.ib_char = c;
			wnull.tree[iconNr].ob_width = po->ob_width;
			wnull.tree[iconNr].ob_height = po->ob_height;
			deskAlign(&wnull.tree[iconNr]);
			SetIconTextWidth(&pii->ib);
			
			redrawObj(&wnull,iconNr);
			flushredraw();
			return TRUE;
		}
	}
	return FALSE;
}


word buffPathUpdate(const char *path)
{
	char *cp;
	word i;
	
	/*
	 * look if path is already stored
	 */
	for (i=0; i < pathStackTop; i++)
	{
		if (strstr(path,pathStack[i]) == path)
			return TRUE;
	}
	if(pathStackTop < MAXPATHS)
	{
		cp = tmpmalloc(strlen(path)+1);
		if(cp == NULL)
			return FALSE;
		
		strcpy(cp,path);
		pathStack[pathStackTop++] = cp;
		return TRUE;
	}
	else
		return FALSE;
}

void flushPathUpdate(void)
{
	word i;
	
	for(i=pathStackTop-1; i >= 0; i--)
	{
		if (pathStack[i])
		{
			pathUpdate(pathStack[i],"");
			tmpfree(pathStack[i]);
			pathStack[i] = NULL;
		}
	}
	pathStackTop = 0;
}

/*
 * void setSelected(OBJECT *tree,word objnr, word flag)
 * set SELECTED Bit in tree[objnr].ob_state depending on flag
 */
void setSelected(OBJECT *tree, word objnr, word flag)
{
	if(flag)
		tree[objnr].ob_state |= SELECTED;
	else
		tree[objnr].ob_state &= ~SELECTED;
}

word isSelected(OBJECT *tree, word objnr)
{
	return tree[objnr].ob_state & SELECTED;
}

void setHideTree(OBJECT *tree, word objnr, word flag)
{
	if(flag)
		tree[objnr].ob_flags |= HIDETREE;
	else
		tree[objnr].ob_flags &= ~HIDETREE;
}

word isHidden(OBJECT *tree, word objnr)
{
	return tree[objnr].ob_flags & HIDETREE;
}

void setDisabled(OBJECT *tree, word objnr, word flag)
{
	if(flag)
		tree[objnr].ob_state |= DISABLED;
	else
		tree[objnr].ob_state &= ~DISABLED;
}

word isDisabled(OBJECT *tree, word objnr)
{
	return tree[objnr].ob_state & DISABLED;
}

void doFullGrowBox(OBJECT *tree,word objnr)
{
	word x,y,w,h;
	
	objc_offset(tree,objnr,&x,&y);
	w = tree[objnr].ob_width;
	h = tree[objnr].ob_height;
	
	graf_growbox(x,y,w,h,deskx,desky,deskw,deskh);
}

static word changeOldPath(char *oldpath,
						const char *oldname,
						const char *newname)
{
	size_t len;
	
	if (strstr(oldpath,oldname) == oldpath) /* gleicher Anfang */
	{
		len = strlen(oldname);
		if (oldpath[len] == '\0')	/* sind identisch */
		{
			strcpy(oldpath,newname);
			return TRUE;
		}
		else if (oldpath[len] == '\\')	/* Pfad ge„ndert */
		{
			char tmp[MAXLEN];
			
			strcpy(tmp,newname);
			strcat(tmp,oldpath+len);
			strcpy(oldpath,tmp);
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * int fileWasMoved(const char *oldname,const char *newname)
 * look if moved file oldname is installed on the desktop and
 * change then its path to newname
 * return if change has happend
 */
int fileWasMoved(const char *oldname,const char *newname)
{
	WindInfo *wp;
	IconInfo *pii;
	word i,changed = FALSE;
	
	for(i = wnull.tree[0].ob_head; 
		i > 0; i = wnull.tree[i].ob_next)
	{
		if ((wnull.tree[i].ob_type != G_ICON)
			&& (wnull.tree[i].ob_type != (G_ICON|DRAG)))
			continue;
		
		pii = getIconInfo(&wnull.tree[i]);
		switch(pii->type)
		{
			case DI_SCRAPDIR:
			case DI_PROGRAM:
			case DI_FOLDER:
			case DI_TRASHCAN:
				if(changeOldPath(pii->path,oldname,newname))
				{						/* neuen Pfad eintragen */
					getLabel(newname[0] - 'A',pii->label);
					if (pii->type == DI_SCRAPDIR)
						scrp_write((char *)newname);
					changed = TRUE;
				}
				break;
		}
	}
	
	wp = wnull.nextwind;
	while (wp)
	{
		if (wp->kind == WK_FILE)
		{
			if (strstr(wp->path, oldname))
			{
				char tmp[MAXLEN];
				
				strcpy(tmp, &wp->path[strlen(oldname)]);
				strcpy(wp->path, newname);
				strcat(wp->path, tmp);
				strcpy(wp->title, wp->path);
				wp->update |= NEWWINDOW;
			}
		}
		wp = wp->nextwind;
	}
	return changed;
}

word countObjects(OBJECT *tree,word startobj)
{
	word currchild,anz = 1;
	
	if((currchild = tree[startobj].ob_head) > 0)
											/* Object hat Kinder */
	{
		while(currchild != startobj)		/* einmal rum */
		{
			anz += countObjects(tree,currchild);
			currchild = tree[currchild].ob_next;
		}
	}
	return anz;
}

/*
 * FileInfo *getfinfo(OBJECT *po)
 * get FileInfo pointer from Pointer to union
 */
FileInfo *getfinfo(OBJECT *po)
{
	FileInfo *pf;

	pf = (FileInfo *)(po->ob_spec.index - offsetof(FileInfo,o));
	if(strcmp(pf->magic,"FilE"))
		venusDebug("getfinfo failed!");
	return pf;
}

/*
 * void makeEditName(char *name)
 * convert the filename name into a name that takes exactly
 * 11 characters and has no point ('venus.c' -> 'venus   c  ')
 */
void makeEditName(char *name)
{
	char *cs,*pext,mystr[MAX_FILENAME_LEN];
	size_t l;
	register word i;
	
	if((cs = strchr(name,'.')) != NULL)	/* is there an extension? */
	{
		pext = cs+1;
		*cs = '\0';
		strcpy(mystr,name);				/* get the name part */

		l = strlen(mystr);				/* fill the string with ' ' */
		for(i = 8; i > l; )
			mystr[--i] = ' ';
		mystr[8] = '\0';
		
		strcat(mystr,pext);				/* append the extension */
		strcpy(name,mystr);				/* give it back */
	}
}

/*
 * void makeFullName(char *name)
 * reverse the effect of makeEditName
 * ('venus   c  ' -> 'venus.c')
 */
void makeFullName(char *name)
{
	char fname[13],fext[13];
	char *cs;
	
	*fname = *fext = '\0';
	
	if(strlen(name) > 8)		/* is there an extension? */
	{
		strcpy(fext,&name[8]);	/* copy the extension */
		name[8] = '\0';			/* look at name part */
		if((cs = strtok(name," ")) != NULL)
		{						/* there is a name part */
			strcpy(fname,cs);
			fname[8] = '\0';
		}
		strcpy(name,fname);		/* paste parts together */
		strcat(name,".");
		strcat(name,fext);
	}
}

word charAlign(word x)
{
	register word tmp;
	
	tmp = (((x + 1)/ wchar) * wchar) - 1;
	if(((x + 1) % wchar) * 2 > wchar)
		tmp += wchar;
	if(tmp < 0)
		tmp = wchar - 1;
	return tmp;
}

/*
 * IconInfo *getIconInfo(OBJECT *po)
 * make Pointer to IconInfo from pointer
 * to its inner ICONBLK
 */
IconInfo *getIconInfo(OBJECT *po)
{
	IconInfo *pii;
	pii = (IconInfo *)(po->ob_spec.index - offsetof(IconInfo,ib));
	if(strcmp(pii->magic,"IcoN"))
		venusDebug("getIconInfo failed!");
	return pii;
}

/*
 * word checkPromille(word pm, word default)
 * check pm against [0,1000] and return default
 * if out if range, pm otherwise
 */
word checkPromille(word pm, word def)
{
	if(pm < 0 || pm > 1000)
		return def;
	
	return pm;
}

/*
 * word getBootPath(char *path,const char *fname)
 * search for the file named <fname> and return it's path
 * in <path>. function value is success.
 */
word getBootPath(char *path,const char *fname)
{
	strcpy(path,fname);
	if (shel_find(path))
	{
		if (strcmp(path,fname))
			stripFileName(path);
		else
			getFullPath(path);
		return TRUE;
	}
	else
	{
		getFullPath(path);
		addFileName(path,fname);
		if (fileExists(path))
		{
			stripFileName(path);
			return TRUE;
		}
	}
	return FALSE;
}

void setBigClip(word handle)
{
	word pxy[4];
	
	pxy[0] = deskx;
	pxy[1] = desky;
	pxy[2] = deskx + deskw - 1;
	pxy[3] = desky + deskh - 1;
	vs_clip(handle,TRUE,pxy);
}

void remchr(char *str, char c)
{
	char *cp;
	
	while ((cp = strchr(str, c)) != NULL)
	{
		memmove(cp,cp+1,strlen(cp));
	}
}

word GotGEM14(void)
{
	static word version = 0;
	
	if (!version)
		version = _GemParBlk.global[0];
	
	return (version >= 0x140) && (version != 0x210);
}

long GetHz200(void)
{
	long oldssp = Super(0L);
	long time;
	
	time = *((long *) 0x4baL);
	Super((void *)oldssp);
	
	return time;
}

#if MERGED
int CallMupfelFunction(MFunction mf, const char *fpath,
						const char *tpath, word *memfailed)
{
	word retcode;
	char *argv[5] = {NULL};
	int argc;
	MupfelFunction f;
	
	*memfailed = FALSE;
	switch (mf)
	{
		case M_CP:
			f = CP;
			argv[1] = "-d";
			argc = 2;
			break;
		case M_MV:
			f = MV;
			argc = 1;
			break;
		case M_RM:
			f = RM;
			argc = 1;
			break;
		default:
			venusDebug("illegaler opcode in CallMupfelFunction");
			return 1;
	}
	
	argv[argc] = tmpmalloc(strlen(fpath)+1);
	if (!argv[argc])
	{
		*memfailed = TRUE;
		return 1;
	}
	strcpy(argv[argc++], fpath);

	if (tpath)
	{
		argv[argc] = tmpmalloc(strlen(tpath)+1);
		if (!argv[argc])
		{
			tmpfree(argv[argc-1]);
			*memfailed = TRUE;
			return 1;
		}
		strcpy(argv[argc++], tpath);
	}
	
	retcode = MupfelCommand(f, argc, argv);
	
	tmpfree(argv[--argc]);
	if (tpath)
		tmpfree(argv[--argc]);
	
	return retcode;
}
#endif