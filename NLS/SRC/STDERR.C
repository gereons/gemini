/*
	@(#)Tools/stderr.c
	@(#)Julian F. Reschke, 7. Juli 1990
	
	Setze stderr auf vernuenftigen Kanal
*/

#include <ext.h>
#include <stdlib.h>
#include <stdio.h>
#include <tos.h>

void patchstderr (void)
{
	if (stderr->Handle == 2) return;	/* schon passiert? */
	
	stderr->Handle = 2; 				/* umsetzen! */
	if (getenv ("STDERR")) return;		/* lassen, wie es ist? */

	if (isatty (2)) Fforce (2, -1);		/* umbiegen, falls kein tty */
}
