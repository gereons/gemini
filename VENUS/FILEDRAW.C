/*
 * @(#) Gemini\FileDraw.c
 * @(#) Stefan Eissing, 08. M„rz 1991
 *
 * description: functions to redraw text in file windows
 *
 */

#include <vdi.h>
#include <stdlib.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\fontsel.h>
#include <nls\nls.h>


#include "vs.h"
#include "showbox.rh"
#include "stdbox.rh"

#include "redraw.h"
#include "util.h"
#include "venuserr.h"
#include "filedraw.h"
#include "fileutil.h"

/* external
 */
extern word handle,hchar;
extern word deskx,desky,deskw,deskh;
extern ShowInfo show;
extern DeskInfo NewDesk;
extern OBJECT *pshowbox,*pstdbox;
extern FONTWORK filework;

/* internal texts
 */
#define NlsLocalSection "G.filedraw"
enum NlsLocalText{
T_TEST,			/* D ist ein Ordner!*/
T_WRONGFONT,	/*Der gewnschte Font fr die Dateifenster konnte 
nicht eingestellt werden! Ist vielleicht kein GDOS vorhanden?*/
};

/*
 * internals
 */
word ff_wchar, ff_hchar;
static word fontSize;
static word font_id;
static word char_height;
static word char_width;
static char dir_char;


void GetFileFont(word *id, word *points)
{
	*id = font_id;
	*points = fontSize;
}

word cdecl drawFileText(PARMBLK *pb)
{
	if (pb->pb_prevstate == pb->pb_currstate)
	{
		char *text = (char *)pb->pb_parm;
		word pxy[4];

		pxy[0] = pb->pb_xc;
		pxy[1] = pb->pb_yc;
		pxy[2] = pb->pb_xc + pb->pb_wc - 1;
		pxy[3] = pb->pb_yc + pb->pb_hc - 1;
		vs_clip(handle, TRUE, pxy);
		vswr_mode(handle, MD_TRANS);
	
		v_gtext(handle, pb->pb_x, pb->pb_y, text);
		
/*		if (pb->pb_currstate & SELECTED)
		{
			vsf_color(handle,BLACK);
			vsf_interior(handle,FIS_SOLID);
			pxy[0] = pb->pb_x;
			pxy[1] = pb->pb_y;
			pxy[2] = pb->pb_x + pb->pb_w - 1;
			pxy[3] = pb->pb_y + pb->pb_h - 1;
	
			vr_recfl(handle,pxy);
		}
*/	}
	return pb->pb_currstate;
}


static void optDraw(WindInfo *wp,word r1[4])
{
	word r2[4],i,x,y,pxy[4];
	OBJECT *tree = wp->tree;
	FileInfo *pf;
	
	r2[0] = deskx;
	r2[1] = desky;
	r2[2] = deskx + deskw;
	r2[3] = deskx + deskh;
	
	if (!rc_intersect(r2, r1))
		return;
		
	pxy[0] = r1[0];
	pxy[1] = r1[1];
	pxy[2] = r1[0] + r1[2] - 1;
	pxy[3] = r1[1] + r1[3] - 1;
	vswr_mode(handle,MD_REPLACE);
	vsf_color(handle,WHITE);
	vs_clip(handle,FALSE,pxy);
	vr_recfl(handle,pxy);
	vs_clip(handle,TRUE,pxy);

	vsf_color(handle,BLACK);
	
	for (i=tree[0].ob_head; i > 0; i=tree[i].ob_next)
	{
		pf = getfinfo(&tree[i]);
		if (!pf)
			continue;
		x = r2[0] = tree[0].ob_x + tree[i].ob_x;
		y = r2[1] = tree[0].ob_y + tree[i].ob_y;
		r2[2] = tree[i].ob_width;
		r2[3] = tree[i].ob_height;
		if (rc_intersect(r1,r2))
		{
			if ((r2[2] >= tree[i].ob_width) 
				&& (r2[3] >= tree[i].ob_height))
			{
				vs_clip(handle,FALSE,pxy);
				v_gtext(handle,x,y,pf->o.t.text);
				vs_clip(handle,TRUE,pxy);
			}
			else
				v_gtext(handle,x,y,pf->o.t.text);
	
			if (tree[i].ob_state & SELECTED)
			{
				r2[2] += r2[0] - 1;
				r2[3] += r2[1] - 1;
				vswr_mode(handle,MD_XOR);
				vr_recfl(handle,r2);
				vswr_mode(handle,MD_REPLACE);
			}
			
		}
	}
}

void optFileDraw(WindInfo *wp,word r1[4])
{
	word r2[4];
	
	vsf_interior(handle,FIS_SOLID);

	GrafMouse(M_OFF,NULL);
	wind_get(wp->handle,WF_FIRSTXYWH,
				&r2[0],&r2[1],&r2[2],&r2[3]);
	while (r2[2]+r2[3])
	{
		if (rc_intersect(r1,r2))
		{
			optDraw(wp,r2);
		}
		wind_get(wp->handle,WF_NEXTXYWH,
					&r2[0],&r2[1],&r2[2],&r2[3]);
	}
	GrafMouse(M_ON,NULL);
	setBigClip(handle);
}

static void calcNewFontWindows(void)
{
	ff_wchar = char_width;
	ff_hchar = char_height;
	pstdbox[FISTRING].ob_height = ff_hchar;
	allFileChanged(TRUE);
}

word SetFont(FONTWORK *fw, word *id, word *points,
					word *width, word *height)
{
	word distances[5];
	word effects[3];
	word cw,ch;
	word dummy, desiredId;
	
	FontLoad(fw);

	desiredId = *id;
	*id = vst_font(fw->handle, *id);
	vqt_fontinfo(fw->handle, &dummy, &dummy,
					distances, width, effects);
	*points = vst_point(fw->handle, *points, &cw, &ch, width, height);
	
	return desiredId == *id;
}

void setAESFont(FONTWORK *fw)
{
	word i;
	word size;
	
	font_id = 1;
	size = 10;
	
	for(i=0; i<11; i++)
	{
		fontSize = size;
		SetFont(fw, &font_id, &fontSize, &char_width, &char_height);

		if (char_height < hchar)
			++size;
		else if (char_height > hchar)
			--size;
		else			/* it fit's */
			break;
	}
}

static void deinst_font(void)
{
	FontUnLoad(&filework);
}

/*
 * word doShowOptions(void)
 * make dialog for showoptions (options for display of files
 * in windows)
 */
word doShowOptions(void)
{
	DIALINFO d;
	FONTSELINFO F;
	uchar *pdir;
	word id,size,retcode;
	word changed,length,date,time;
	word clicks, draw, item;
	char *test = (char *)NlsStr(T_TEST);

	pdir = (uchar *)pshowbox[DIRCHAR].ob_spec.tedinfo->te_ptext;
	test[1] = pdir[0] = dir_char;
	

	setSelected(pshowbox,SOPTSIZE,show.fsize);
	setSelected(pshowbox,SOPTDATE,show.fdate);
	setSelected(pshowbox,SOPTTIME,show.ftime);

	
	DialCenter(pshowbox);

	id = font_id;
	size = fontSize;
	GrafMouse(HOURGLASS, NULL);
	FontGetList(&filework, TRUE, FALSE);
	FontSelInit(&F, &filework, pshowbox, SHOWFB, SHOWFBG, SHOWPB,
		SHOWPBG, SHOWFONT, (char *)NlsStr(T_TEST), 0, &id, &size);
	GrafMouse(ARROW, NULL);

	DialStart(pshowbox,&d);
	DialDraw(&d);
	FontSelDraw(&F, id, size);

	do
	{
		draw = TRUE;
		item = -1;
		
		retcode = DialDo(&d, 0);
		
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;
		
		if (retcode == SHOWFB || retcode == SHOWFBG)
			item = FontClFont (&F, clicks, &id, &size);

		else if (retcode == SHOWPB || retcode == SHOWPBG)
			item = FontClSize (&F, clicks, &id, &size);

		else if (retcode == DIRPREV)
		{
			if (*pdir > 1)
			{
				test[1] = --pdir[0];
				fulldraw (pshowbox, DIRCHAR);
				FontShowFont (F.fw, pshowbox, SHOWFONT,
							 id, size, test);
			}
		}
		else if (retcode == DIRNEXT)
		{
			if (*pdir < 255)
			{
				test[1] = ++pdir[0];
				fulldraw (pshowbox, DIRCHAR);
				FontShowFont (F.fw, pshowbox, SHOWFONT,
							 id, size, test);
			}
		}
		if ((item >= 0) && (clicks == 2))
		{
			retcode = SHOWOK;
			draw = FALSE;
		}

	} while ((retcode != SHOWCANC) && (retcode != SHOWOK));
	
	if (draw)
	{
		setSelected(pshowbox,retcode,FALSE);
		fulldraw(pshowbox,retcode);
	}
	
	if (retcode == SHOWOK)
	{
		length = isSelected(pshowbox,SOPTSIZE);
		date = isSelected(pshowbox,SOPTDATE);
		time = isSelected(pshowbox,SOPTTIME);
		
		changed = ((show.showtext)
					&&((length != show.fsize)
					||(date != show.fdate)
					||(time != show.ftime)
					||(pdir[0] != dir_char)));

		show.fsize = length;
		show.fdate = date;
		show.ftime = time;
		
		dir_char = *pdir;
		pstdbox[FISTRING].ob_spec.free_string[0] = dir_char;

		if ((changed) || (id != font_id) || (size != fontSize))
			changed = TRUE;
		
		if (changed)
		{
			font_id = id;
			fontSize = size;
		}
		
		SetFont(&filework, &font_id, &fontSize,
				&char_width, &char_height);

		FontSelExit(&F);
		DialEnd(&d);

		if (changed)
			calcNewFontWindows();
		return changed;
	}
	else
	{
		/* all was cancelled */
		SetFont(&filework, &font_id, &fontSize,
					&char_width, &char_height);
		changed = FALSE;
	}

	FontSelExit(&F);
	DialEnd(&d);
	return changed;
}

void initFileFonts(void)
{
	font_id = 1;
	fontSize = 10;
	SetFont(&filework, &font_id, &fontSize,
				&char_width, &char_height);
	ff_wchar = char_width;
	ff_hchar = char_height;
	pstdbox[FISTRING].ob_height = ff_hchar;
	dir_char = pstdbox[FISTRING].ob_spec.free_string[0];
}

void exitFileFonts(void)
{
	deinst_font();
}

word writeFontInfo(word fhandle, char *buffer)
{
	sprintf(buffer, "#F@%d@%d@%d@%d@%d",
			font_id, fontSize, dir_char,
			NewDesk.snapx, NewDesk.snapy);
	return Fputs(fhandle, buffer);
}

void execFontInfo(char *line)
{
	char *cp;
	
	strtok(line,"@\n");		/* skip first */
	
	if((cp = strtok(NULL,"@\n")) != NULL)
	{
		font_id = atoi(cp);
		if((cp = strtok(NULL,"@\n")) != NULL)
		{
			fontSize = atoi(cp);
			if((cp = strtok(NULL,"@\n")) != NULL)
			{
				dir_char = (char)atoi(cp);
				if((cp = strtok(NULL,"@\n")) != NULL)
				{
					NewDesk.snapx = (char)atoi(cp);
					if((cp = strtok(NULL,"@\n")) != NULL)
					{
						NewDesk.snapy = (char)atoi(cp);
					}
				}
			}
		}
	}
	if (font_id < 1)
		font_id = 1;
		
	if ((font_id == 1)	&& (fontSize == 10))
	{
		/* system font, try to scale */
		setAESFont(&filework);
	}
	else
	{
		if (!SetFont(&filework, &font_id, &fontSize,
					&char_width, &char_height))
		{
			venusErr(NlsStr(T_WRONGFONT));
		}
	}
	ff_wchar = char_width;
	ff_hchar = char_height;
	pstdbox[FISTRING].ob_height = ff_hchar;
	pstdbox[FISTRING].ob_spec.free_string[0] = dir_char;
}

void switchFont(void)
{
	static word isSystemFont = FALSE;
	static word id, size;
	
	if (isSystemFont)
	{
		font_id = id;
		fontSize = size;
		SetFont(&filework, &font_id, &fontSize,
					&char_width, &char_height);
		ff_wchar = char_width;
		ff_hchar = char_height;
		isSystemFont = FALSE;
	}
	else
	{
		id = font_id;
		size = fontSize;
		setAESFont(&filework);
		isSystemFont = TRUE;
	}
}