/*
 * @(#) Gemini\Draglogi.c
 * @(#) Stefan Eissing, 11. April 1991
 *
 * description: logic to handle dragged icons
 *
 */

#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "renambox.rh"

#include "draglogi.h"
#include "redraw.h"
#include "util.h"
#include "venuserr.h"
#include "copymove.h"
#include "erase.h"
#include "select.h"
#include "fileutil.h"
#include "iconinst.h"
#include "myalloc.h"
#include "dispatch.h"
#include "icondrag.h"
#include "menu.h"
#include "applmana.h"
#include "stand.h"

store_sccs_id(draglogi);

/* internal texts
 */
#define NlsLocalSection "G.draglogi"
enum NlsLocalText{
T_RENAME,		/*Kann die Datei nicht zu %s umbenennen!*/
};

/* externals
 */
extern WindInfo wnull;
extern word deskx,desky,deskw,deskh;
extern OBJECT *prenamebox,*pmovebox,*pcopybox;
extern char *mvCommand;

/*
 * static void moveDeskIcons(WindInfo *wp,word dx,word dy)
 * move with 'DRAG' marked icons in Window wp about dx,dy
 */
static void moveDeskIcons(WindInfo *wp,word dx,word dy)
{
	OBJECT *tree;
	word i,r[4];
	
	if((dx||dy)&&(wp->kind == WK_DESK))
	{
		tree = wp->tree;
		for(i=wp->tree[0].ob_head;i > 0; i = wp->tree[i].ob_next)
		{
			if(tree[i].ob_type != (G_ICON|DRAG))
				continue;

			/* gedraggtes Object */
			objc_offset(tree,i,&r[0],&r[1]);
			r[2] = tree[i].ob_width;
			r[3] = tree[i].ob_height;
			tree[i].ob_x += dx;
			tree[i].ob_y += dy;
			buffredraw(wp,r);
				r[0] += dx;
				r[1] += dy;
			buffredraw(wp,r);
		}
		flushredraw();					/* to be shure */
	}
}

/*
 * static void doRename(WindInfo *wp)
 * rename with 'DRAG' marked files in window wp
 * (with dialog and such stuff...)
 */
static void doRename(WindInfo *wp)
{
	DIALINFO d;
	uword i;
	word ok;
	word obx,oby,obw,obh,retcode;
	FileBucket *bucket;
	char tmpstr[MAXLEN], tmp2[MAXLEN],oldname[13],newname[13];
	int edit_object = RNEWNAME;
	
	if (wp->kind != WK_FILE)		/* no file window */
		return;
		
	prenamebox[ROLDNAME].ob_spec.tedinfo->te_ptext = oldname;
	prenamebox[RNEWNAME].ob_spec.tedinfo->te_ptext = newname;

	DialCenter(prenamebox);
	DialStart(prenamebox,&d);
	DialDraw(&d);

	objc_offset(prenamebox,RNEWNAME,&obx,&oby);
	obw = obx + prenamebox[RNEWNAME].ob_width;
	obh = oby + prenamebox[RNEWNAME].ob_height;
	objc_offset(prenamebox,ROLDNAME,&obx,&oby);
	obw = obw - obx + 1;
	obh = obh - oby + 1;
	ok = TRUE;
	
	bucket = wp->files;
	
	while (bucket && ok)
	{
		for (i = 0; (i < bucket->usedcount) && ok; ++i)
		{
			if (!(bucket->finfo[i].flags.dragged
				&& bucket->finfo[i].flags.selected))
				continue;
	
			strcpy(tmpstr,bucket->finfo[i].fullname);
			makeEditName(tmpstr);
			strcpy(newname, tmpstr);
			strcpy(oldname, tmpstr);
			fulldraw(prenamebox, ROLDNAME);
			fulldraw(prenamebox, RNEWNAME);
	
			retcode = DialDo(&d, &edit_object) & 0x7FFF;
			setSelected(prenamebox, retcode, FALSE);			
			fulldraw(prenamebox, retcode);
			
			if(retcode == RENOK)
			{
			 	if(strcmp(oldname, newname))
				{
					MFORM tmpform;
					word tmpnum;
					
					GrafGetForm(&tmpnum, &tmpform);
					GrafMouse(HOURGLASS, NULL);
				
					makeFullName(oldname);
					makeFullName(newname);
					
					strcpy(tmpstr, wp->path);
					addFileName(tmpstr, oldname);
					
					strcpy(tmp2, wp->path);
					addFileName(tmp2, newname);
					if(fileRename(tmpstr, tmp2) != 0)
					{
						venusErr(NlsStr(T_RENAME), newname);
					}
					else
					{
						fileWasMoved(tmpstr, tmp2);
						ApRenamed(newname, bucket->finfo[i].fullname, 
									wp->path);
					}
					GrafMouse(tmpnum, &tmpform);
				}
			}
			else
				ok = FALSE;
		}
		
		bucket = bucket->nextbucket;
	}
	DialEnd(&d);
}

#if MERGED
void PasteString(const char *line)
{
	while (*line && (CommInfo.cmd != overlay))
	{
		CommInfo.cmd = feedKey;
		CommInfo.cmdArgs.key = (long)*line & 0xFFL;
		if (callMupfel() != 0)
			break;

		++line;
	}
}
#endif

static void pasteDragNames(WindInfo *fwp)
{
	char *filenames;
	
	filenames = getDraggedNames(fwp);
	if (filenames)
	{
#if STANDALONE
		venusDebug("%s",filenames);
#else
		remchr(filenames, '\'');
		PasteString(filenames);
#endif
		free(filenames);
	}
}

int PerformCopy (WindInfo *from, char *from_path, char *to_path, 
		int move)
{
	int was_action;
	
	was_action = doCopy (from, from_path, to_path, move);

	killEvents (MU_KEYBD, 20);
	desObjExceptWind (from, 0);
	desNotDragged (from);
	if (was_action)
	{
		if (move)
			pathUpdate (from_path, to_path);
		else
			pathUpdate (to_path, "");
	}
	
	return was_action;
}


/*
 * void doDragLogic(WindInfo *fwp,WindInfo *twp,word toobj,word kstate,
 *					word fromx,word fromy,word tox,word toy)
 * manage drag of icons from window fwp to window twp and decide,
 * if files should be copied, moved, renamed and so on.
 */
void doDragLogic(WindInfo *fwp,WindInfo *twp,word toobj,word kstate,
					word fromx,word fromy,word tox,word toy)
{
	word fromobj,fobj,ok,wasaction;
	char fpath[MAXLEN],tpath[MAXLEN];
	char *command;

	if((twp->kind != WK_FILE)&&(toobj == -1))
	{
		if (twp->kind == WK_DESK)
		{
			switch (fwp->kind)
			{
				case WK_DESK:
					moveDeskIcons(fwp,tox - fromx,toy - fromy);
					break;
				case WK_FILE:
					instDraggedIcons(fwp,fromx,fromy,tox,toy);
					break;
			}
		}
		else if (twp->kind == WK_MUPFEL)
		{
			pasteDragNames(fwp);
		}
	}
	else
	{
		fromobj = objc_find(fwp->tree,0,MAX_DEPTH,fromx,fromy);
		fobj = objc_find(twp->tree,0,MAX_DEPTH,tox,toy);
		
		if((fwp->handle != twp->handle)
			|| (fromobj != fobj)
			|| (twp->tree[fobj].ob_type == (G_ICON|DRAG)
				&& !isOnIcon(tox,toy,twp->tree,fobj)))
		{
			ok = TRUE;
			if(fwp->kind == WK_FILE)
			{
				strcpy(fpath,fwp->path);
			}
			else
				fpath[0] = '\0';
				
			if(twp->handle)
			{
				strcpy(tpath,twp->path);
				if(toobj > 0)
				{
					FileInfo *pf;
					
					pf = getfinfo(&twp->tree[toobj]);
					if(strcmp(pf->fullname,".."))
						addFolderName(tpath,pf->fullname);
					else
						stripFolderName(tpath);	/* folder .. */
				}
			}
			else
			{
				IconInfo *pii;
				char *cp;
				
				pii = getIconInfo(&twp->tree[toobj]);
				switch(pii->type)
				{
					case DI_FILESYSTEM:
					case DI_FOLDER:
					case DI_SCRAPDIR:
						strcpy(tpath,pii->path);
						break;
					case DI_TRASHCAN:
						strcpy(tpath,pii->path);
						kstate |= K_CTRL;
						break;
					case DI_PROGRAM:
						deselectObjects(0);
						command = getDraggedNames(fwp);
						strcpy(fpath,pii->path);
						cp = strrchr(fpath,'\\');
						strcpy(tpath,cp+1);
						stripFileName(fpath);
						WindUpdate(END_MCTRL);
						startFile(twp,toobj,TRUE,pii->label,
							fpath,tpath,command);
						WindUpdate(BEG_MCTRL);
						if(command)
							free(command);
						ok = FALSE;
						break;
					case DI_SHREDDER:
						ok = FALSE;
						wasaction = doErase(fwp,fpath);
						killEvents(MU_KEYBD,20);
						desObjExceptWind(fwp,0);
						desNotDragged(fwp);
						if(wasaction)
						{
							pathUpdate(fpath,"");
						}
						break;
					default:
						venusDebug("unbekannter Icontyp!");
						ok = FALSE;
						break;
				}
			}
			
			if(ok)
			{
				if((kstate & K_CTRL)
					&& *fpath
					&&(!strcmp(fpath,tpath)))
				{
					doRename(fwp);
					desObjExceptWind(fwp,0);
					desNotDragged(fwp);
					pathUpdate(fpath,"");
				}	
				else
				{
					PerformCopy (fwp, fpath, tpath, kstate & K_CTRL);
				}
			}
			
			if ((wnull.nextwind != NULL)
				&& (wnull.nextwind->kind == WK_FILE))
			{
				setFullPath(wnull.nextwind->path);
			}
		}
	}
}
