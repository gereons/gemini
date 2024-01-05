/*
 * @(#) Gemini\scut.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: handles dialog for Shortcuts
 * 				in venus
 */

#include <string.h>
#include <flydial\flydial.h>

#include "shortcut.rh"

#include "vs.h"
#include "scut.h"
#include "util.h"
#include "redraw.h"
#include "select.h"
#include "dispatch.h"


/* externals
 */
extern OBJECT *pshortcut;
extern WindInfo wnull;


word Object2SCut(word index)
{
	switch (index)
	{
		case SHORTNO:
			return SCUT_NO;
		case SHORT0:
			return SCUT_0;
		case SHORT1:
			return SCUT_1;
		case SHORT2:
			return SCUT_2;
		case SHORT3:
			return SCUT_3;
		case SHORT4:
			return SCUT_4;
		case SHORT5:
			return SCUT_5;
		case SHORT6:
			return SCUT_6;
		case SHORT7:
			return SCUT_7;
		case SHORT8:
			return SCUT_8;
		case SHORT9:
			return SCUT_9;
		default:
			return -1;
	}
}

word SCut2Object(word shortcut)
{
	switch (shortcut)
	{
		case SCUT_0:
			return SHORT0;
		case SCUT_1:
			return SHORT1;
		case SCUT_2:
			return SHORT2;
		case SCUT_3:
			return SHORT3;
		case SCUT_4:
			return SHORT4;
		case SCUT_5:
			return SHORT5;
		case SCUT_6:
			return SHORT6;
		case SCUT_7:
			return SHORT7;
		case SCUT_8:
			return SHORT8;
		case SCUT_9:
			return SHORT9;
		default:
			return SHORTNO;
	}
}

OBSPEC SCut2Obspec(word shortcut)
{
	word index;
	
	index = SCut2Object(shortcut);
	return pshortcut[index].ob_spec;
}

word SCutSelect(OBJECT *tree, word index, 
				word newshort, word origshort, word circle)
{
	word retcode, origindex;
	word restore = FALSE;
	OBSPEC obspec;

	obspec = tree[index].ob_spec;

	origindex = SCut2Object(origshort);
	if (isDisabled(pshortcut, origindex))
	{
		setDisabled(pshortcut, origindex, FALSE);
		restore = TRUE;
	}
	
	retcode = JazzSelect (tree, index, pshortcut,
					 TRUE, circle? -2:0, (long *)&obspec);
	
	if (restore)
		setDisabled(pshortcut, origindex, TRUE);
	
	if (retcode > -1)
	{
		return Object2SCut(retcode);
	}
	else
		return newshort;
}

word DoSCut(word kstate, word kreturn)
{
	word shortcut, i;
	char c;
	
	if (!(kstate & K_ALT))
		return FALSE;
	
	c = kreturn & 0xFF;
	if (c && (strchr("0123456789", c) == NULL))
		return FALSE;
	
	switch (((uword)kreturn) >> 8)
	{
		case 112:			/* Alt-0 */
		case 129:
			shortcut = SCUT_0;
			break;
		case 109:			/* Alt-1 */
		case 120:
			shortcut = SCUT_1;
			break;
		case 110:			/* Alt-2 */
		case 121:
			shortcut = SCUT_2;
			break;
		case 111:			/* Alt-3 */
		case 122:
			shortcut = SCUT_3;
			break;
		case 106:			/* Alt-4 */
		case 123:
			shortcut = SCUT_4;
			break;
		case 107:			/* Alt-5 */
		case 124:
			shortcut = SCUT_5;
			break;
		case 108:			/* Alt-6 */
		case 125:
			shortcut = SCUT_6;
			break;
		case 103:			/* Alt-7 */
		case 126:
			shortcut = SCUT_7;
			break;
		case 104:			/* Alt-8 */
		case 127:
			shortcut = SCUT_8;
			break;
		case 105:			/* Alt-9 */
		case 128:
			shortcut = SCUT_9;
			break;
		default:
			return FALSE;
	}
	
	for (i = wnull.tree[0].ob_head; i > 0; i = wnull.tree[i].ob_next)
	{
		IconInfo *pii;
		
		if (wnull.tree[i].ob_type != G_ICON)
			continue;
		
		pii = getIconInfo(&wnull.tree[i]);
		if (!pii)
			continue;
		
		if (pii->shortcut == shortcut)
		{
			static word lastcut = SCUT_NO;
			static long lasttime = 0L;
			word selected, done = FALSE;
			long now;
			
			now = GetHz200();
			selected = isSelected(wnull.tree, i);
			
			if (selected && (shortcut == lastcut))
			{
				long maxtime, difftime;
				word clickspeed;
				
				clickspeed = evnt_dclick(3, 0);
				maxtime = (5 - clickspeed) * 40;
				difftime = now - lasttime;
				if (difftime < 0)
					difftime = -difftime;
									
				if (difftime <= maxtime)	/* Doppelklick */
				{
					done = TRUE;
					simDclick(kstate & ~K_ALT);
				}
			}
			
			if (!done)
			{
				if (kstate & (K_LSHIFT|K_RSHIFT))
				{
					if (selected)
						objdeselect(&wnull, i);
					else
						objselect(&wnull, i);
				}
				else
				{
					deselectObjects(0);
					objselect(&wnull, i);		/* selektieren */
				}
				flushredraw();
			}
			
			lasttime = now;
			lastcut = shortcut;
			
			break;
		}
	}
	
	return TRUE;
}

word SCutInstall(word shortcut)
{
	word index;
	
	if (shortcut == SCUT_NO)
		return TRUE;
		
	index = SCut2Object(shortcut);
	if ((index >= 0) && !isDisabled(pshortcut, index))
	{
		setDisabled(pshortcut, index, TRUE);
		return TRUE;
	}
	return FALSE;
}

word SCutRemove(word shortcut)
{
	word index;
	
	if (shortcut == SCUT_NO)
		return TRUE;
		
	index = SCut2Object(shortcut);
	if ((index >= 0) && isDisabled(pshortcut, index))
	{
		setDisabled(pshortcut, index, FALSE);
		return TRUE;
	}
	return FALSE;
	
}

