/*
 * timer.c  -  internal "timer" command
 * 22.09.90
 */

#include <stdlib.h>
#include <string.h>
#include <tos.h>

#include "chario.h"
#include "getopt.h"
#include "mupfel.h"
#include "sysvec.h"
 
#define TIMERTICK	200
#define SECSPERMINUTE	60
#define SECSPERHOUR		3600	/* adjust these to your planet */

SCCS(timer);

static ulong timer;

void timerinit(void)
{
	timer = gethz200();
}

int m_timer(ARGCV)
{
	GETOPTINFO G;
	long hours, minutes, seconds, tenths, hundreds;
	ldiv_t t;
	static int startflag = FALSE;
	struct option long_option[] =
	{
		{ "start", FALSE, &startflag, TRUE },
		{ NULL,0,0,0 },
	};
	int opt_index = 0,c;

	startflag = FALSE;	
	optinit (&G);
	
	while ((c = getopt_long (&G, argc, argv, "s", long_option,
		&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				break;
			case 's':
				startflag = TRUE;
				break;
			default:
				return printusage(long_option);
		}
	
	if (startflag)
		timer = gethz200();
	else
	{
		t = ldiv(gethz200()-timer,TIMERTICK);
		seconds = t.quot;
		
		tenths = t.rem / 20;
		hundreds = (t.rem - (tenths*20)) / 2;
		
		hours = seconds / SECSPERHOUR;
		minutes = (seconds - (hours*SECSPERHOUR)) / SECSPERMINUTE;
		seconds -= (minutes*SECSPERMINUTE) + (hours*SECSPERHOUR);

		mprintf(TI_TIME "%ld:%ld:%ld.%ld%ld\n",hours,minutes,seconds,tenths,hundreds);
	}
	return 0;
}
