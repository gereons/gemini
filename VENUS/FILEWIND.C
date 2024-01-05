/*
 * @(#) Gemini\filewind.c
 * @(#) Stefan Eissing, 08. Juni 1991
 *
 * description: functions to manage accessorie windows
 */

#include "vs.h"

#include "filewind.h"
#include "wildcard.h"

void FileWindowSpecials (WindInfo *wp, int mx, int my)
{
	int half;
	
	if (my >= wp->worky || my < wp->windy)
		return;

	(void)mx;
	half = wp->windy + ((wp->worky - wp->windy) / 2);

	if (my < half)
	{
		/* Hier sind wir _wahrscheinlich_ in der Titelzeile des
		 * Fensters. Ganz sicher kann man da nicht sein, aber wenn
		 * nicht, geschieht auch nichts schlimmes...
		 */
		
	}
	else
		doWildcard (wp);
}

