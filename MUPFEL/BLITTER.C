/*
 * blitter.c  -  internal "blitmode" command
 * 26.08.90
 */
 
#include <tos.h>

#include "chario.h"
#include "mupfel.h"
#include "strsel.h"

SCCS(blitter);

static int showblit(void)
{
	int state = Blitmode(-1);
	int retcode;
	
	if (tstbit(state,2))
	{
		retcode = !tstbit(state,1);
		mprintf(BL_STATUS "\n",tstbit(state,1) ? BL_ON : BL_OFF);
	}
	else
	{
		mprintf(BL_NOBLIT "\n");
		retcode = 2;
	}
	return retcode;
}

static int setblit(int on)
{
	int state = Blitmode(-1);
	
	if (on)
		setbit(state,1);
	else
		clrbit(state,1);
	Blitmode(state);
	return 0;
}

int m_blitmode(ARGCV)
{
	int retcode;
	
	if (argc==1)
		return showblit();
	else
	{
		STRSELECT(argv[1])
		WHEN2("on","1")
			retcode = setblit(TRUE);
		WHEN2("off","0")
			retcode = setblit(FALSE);
		DEFAULT
			retcode = printusage(NULL);
		ENDSEL
	}
	return retcode;
}
