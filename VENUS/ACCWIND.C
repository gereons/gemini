/*
 * accwind.c
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: functions to manage accessorie windows
 *
 * last change: 14.01.1991
 */

#include <stdlib.h>
#include <string.h>

#include "vs.h"
#include "accwind.h"
#include "myalloc.h"
#include "venuserr.h"
#include "util.h"
#include "window.h"


store_sccs_id(accwind);


word InsAccWindow(word accid, word whandle)
{
	WindInfo *wp;

	wp = newwp();
	
	if (!wp)
		return FALSE;

	wp->handle = whandle;
	wp->kind = WK_ACC;
	wp->owner = accid;
	
	return TRUE;
}

word DelAccWindow(word accid, word whandle)
{
	WindInfo *wp;
	
	wp = getwp(whandle);
	
	if (!wp || (wp->owner != accid))
		return FALSE;
		
	freewp(wp->handle);
	return TRUE;
}
