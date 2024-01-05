/*
 * xarg.c
 * 10/10/88
 * Beispielimplementation (in Turbo C) eines Programms, das per
 * xArg Argumente annimmt, inklusive Legalit„tschecks.
 */
 
#include <tos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	char		xarg_magic[4];	/* "xArg" == 0x78417267L			*/
	int		xargc;		/* Wie argc in main()			*/
	char		**xargv;		/* Wie argv in main()			*/
	char		*xiovector;	/* Unbenutzt					*/
	BASPAG	*xparent;		/* Zeigt auf die Basepage des Auf- */
						/* rufers. Typdeklaration in tos.h */
} XARG;

main(void)
{
	extern BASPAG *_BasPag;	/* definiert in TCSTART.O */
	XARG	*xarg;
	char *xenv;
	unsigned long x;
	int i;
	
	if ((xenv=getenv("xArg"))!=NULL)
	{
		x = strtoul(xenv,NULL,16);
		printf("xArg structure at %08lX\n",x);
		if ((x!=0) && (x%2==0))
		{
			xarg = (XARG *)x;
			if (!strncmp(xarg->xarg_magic,"xArg",4))
			{
				if (xarg->xparent == _BasPag->p_parent)
				{
					/* alles ok. Argumente verarbeiten */
					for (i=0; i<xarg->xargc; ++i)
						printf("%d: %s\n",i,xarg->xargv[i]);
				}
				else
					printf("xArg parent != my parent\n");
			}
			else
				printf("xArg magic number not found\n");
		}
		else
			printf("illegal xArg address (0 or odd)\n");
	}
	else
		printf("xArg not in environment\n");
	return 0;
}
