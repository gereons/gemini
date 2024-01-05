/*
 * check.c  -  security checks
 * 05.11.90
 */

#include <tos.h>
 
#include "alert.h"
#include "check.h"
#include "comm.h"
#include "mupfel.h"

static size_t check;
static size_t *chkend, *chkstart;
extern BASPAG *_BasPag;		/* defined in TCSTART.O */

SCCS(check);

void reboot(void)
{
	extern long doreboot();
	
	Supexec(doreboot);
	Pterm(-1);
}

/* 
 * calculate checksum over text segment
 */
static size_t checksum(void)
{
	size_t chksum = 0L;
	size_t *p = chkstart;

	while (p < chkend)
		chksum += *p++;

	return chksum;
}

void checkcode(void)
{
	if (check != checksum())
		if (alert(STOP_ICON,1,2,CK_DAMAGE,CK_OK,CK_REBOOT,CommInfo.PgmName)==2)
			reboot();
}

void initcheck(void)
{
	chkstart = _BasPag->p_tbase;
	chkend = (size_t *)((char *)chkstart + _BasPag->p_tlen - 3);
	check = checksum();
}
