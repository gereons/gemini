/*
	@(#)Mupfel/puninfo.c
	@(#)Julian F. Reschke, 7. Februar 1991
	
	get pointer to harddisk PUN_INFO structure, if available
*/
 
#include <tos.h>

#include "puninfo.h"

#define PUNID      0x0F
#define PUNRES     0x70
#define PUNUNKNOWN 0x80

/* get pointer to PUN_INFO-structure, if available */

PUN_INFO *PunThere (void)
{
	PUN_INFO *P;
	long oldstack;

	oldstack = Super(0L);
	P = *((PUN_INFO **)(0x516L));
	Super ((void *)oldstack);

	if (P)	/* berhaupt gesetzt? */
		if (P->P_cookie == 0x41484449L)	/* Cookie gesetzt? */
			if (P->P_cookptr == &(P->P_cookie))
				if (P->P_version >= 0x300)
					return P;					
	return 0L;
}
