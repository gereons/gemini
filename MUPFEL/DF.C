/*
	@(#)Mupfel/df.c
	@(#)Julian F. Reschke, 16. Mai 1991
	
	internal command df

	no global vars, no mallocs
*/
 
#include <ctype.h>
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "chario.h"
#include "curdir.h"
#include "df.h"
#include "environ.h"
#include "getopt.h"
#include "label.h"
#include "mupfel.h"
#include "puninfo.h"
#include "shellvar.h"

#define MAXDRIVES 32

SCCS (df);

/* get pointer to character array containing all drive letters */

char
*drivemap (void)
{
	char *d;
	static char drvmap[MAXDRIVES+1];

	/* Check for DRIVEMAP in environment */	

	if ((d = getenv("DRIVEMAP")) != NULL)
	{
		strncpy (drvmap, d, MAXDRIVES);
		drvmap[MAXDRIVES] = '\0';
		return strupr (drvmap);
	}

	/* Nothing found, use all connected drives */

	return getvar ("drivelist");
}
