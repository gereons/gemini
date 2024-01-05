/*
 * @(#) Gemini\order.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: order icons on the desktop
 */

#include <stdlib.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "divbox.rh"

#include "order.h"
#include "venuserr.h"
#include "gemtrees.h"
#include "util.h"
#include "redraw.h"

/* externals
 */
extern DeskInfo NewDesk;
extern WindInfo wnull;
extern ShowInfo show;
extern word deskx, desky, deskw, deskh;
extern OBJECT *pdivbox;

/* internal texts
 */
#define NlsLocalSection "Gmni.order"
enum NlsLocalText{
T_SKIP,		/*Der gewÑhlte Abstand von %d in X- und %d in Y-Richtung
 ist zu groû fÅr diese Auflîsung!*/
};
				

/* internals
 */
static word wclip, hclip;


word ggT(word x, word y)
{
	word z;
	if (x < y)
	{
		z = x;
		x = y;
		y = z;
	}
	z = x % y;
	while (z)
	{
		x = y;
		y = z;
		z = x % y;
	}
	return y;
}

static void orderIcon(OBJECT *tree, word index)
{
	word times;

	if ((tree[index].ob_type&0xFF) != G_ICON)
		return;
		
	if (NewDesk.selectAnz && !isSelected(tree, index))
		return;
	
	redrawObj(&wnull,index);

	times = (tree[index].ob_x + wclip/2) / wclip;
	tree[index].ob_x = times * wclip;
	times = (tree[index].ob_y + hclip/2) / hclip;
	tree[index].ob_y = times * hclip;

	deskAlign(&tree[index]);
	redrawObj(&wnull,index);
	flushredraw();
}

word doDivOptions(void)
{
	word retcode,changed,ok;
	DIALINFO d;
	char *xskip, *yskip;

	xskip = pdivbox[ORDERX].ob_spec.tedinfo->te_ptext;
	yskip = pdivbox[ORDERY].ob_spec.tedinfo->te_ptext;
	sprintf(xskip,"%2d", NewDesk.snapx);
	sprintf(yskip,"%2d", NewDesk.snapy);
	
	setSelected(pdivbox,MOVEFREE,!show.aligned);
	setSelected(pdivbox,SHOWHIDD,NewDesk.showHidden);

	if (GotBlitter())
	{
		setDisabled(pdivbox, DIVBLIT, FALSE);
		setSelected(pdivbox, DIVBLIT, SetBlitter(-1));
	}
	else
		setDisabled(pdivbox, DIVBLIT, TRUE);
	
	DialCenter(pdivbox);
	DialStart(pdivbox,&d);
	DialDraw(&d);

	do
	{
		changed = FALSE;
		ok = TRUE;
		retcode = DialDo(&d,0) & 0x7FFF;
		setSelected(pdivbox,retcode,FALSE);
	
		if (retcode == DIVOK)
		{	
			word tmpx, tmpy, hidden, align;
			
			hidden = isSelected(pdivbox,SHOWHIDD);
			align = !isSelected(pdivbox,MOVEFREE);

			changed = (hidden != NewDesk.showHidden);

			NewDesk.showHidden = hidden;
			show.aligned = align;

			tmpx = atoi(xskip);
			tmpy = atoi(yskip);
			if ((tmpx > deskw) || (tmpy > deskh))
			{
				ok = FALSE;
				venusErr(NlsStr(T_SKIP), tmpx, tmpy);
			}
			else
			{
				NewDesk.snapx = tmpx;
				NewDesk.snapy = tmpy;
			}
			if (!isDisabled(pdivbox, DIVBLIT))
			{
				SetBlitter(isSelected(pdivbox, DIVBLIT)? 1 : 0);
			}
		}
		fulldraw(pdivbox,retcode);
	} while (!ok);

	DialEnd(&d);

	return changed;
}

static void setSnap(OBJECT *tree, word index)
{
	if ((tree[index].ob_type&0xFF) != G_ICON)
		return;
		
	if (NewDesk.selectAnz && !isSelected(tree, index))
		return;
	
	if (wclip)
		wclip = ggT(wclip, tree[index].ob_width);
	else
		wclip = tree[index].ob_width;

	if (hclip)
		hclip = ggT(hclip, tree[index].ob_height);
	else
		hclip = tree[index].ob_height;
}

static void setSnapSize(void)
{
	wclip = hclip = 0;
	walkGemTree(NewDesk.tree, 0, setSnap);
}


void orderDeskIcons(void)
{
	setSnapSize();
	
	wclip += NewDesk.snapx;
	hclip += NewDesk.snapy;

	walkGemTree(NewDesk.tree, 0, orderIcon);
}
