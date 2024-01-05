/*
 * GbootXXX.c  -  delay XXX seconds before starting Gemini
 * If name is MbootXXX, run Mupfel instead
 * Install this program when auto-starting Gemini together with
 * MultiDesk or Harlekin
 * 20.10.90
 */
 
#include <aes.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>

void ourexit(int retcode)
{
	/*
	 * Note: can't simply terminate, because ACCs may have made Malloc 
	 * calls. To avoid crashes, keep our basebage resident.
	 */
	Ptermres(sizeof(BASPAG), retcode);
}

void main(void)
{
	long delay;
	char *dot, *which, *bs;
	char cmd[128], tail[128], path[128];
	COMMAND notail = { 0, "" };

	if (Kbshift(-1)!=0)
		ourexit(0);
		
	if (appl_init() < 0)
		ourexit(1);

	shel_read(cmd,tail);		/* get our name */
	if (strchr(cmd,'\\')==(char *)0)
	{
		strcpy(path,"X:\\");
		path[0] = Dgetdrv() + 'A';
		Dgetpath(&path[3],0);
		if (path[strlen(path)-1] != '\\')
			strcat(path,"\\");
		strcat(path,cmd);
	}
	else
		strcpy(path,cmd);

	which = strrchr(path,'\\');	/* last backslash */
	++which;					/* bump to first letter of filename */
	
	dot = strrchr(path,'.');		/* last dot */
	*dot = '\0';				/* terminate string there */

	delay = (long)atoi(dot-3);	/* 3 digits before dot */
	if (delay <= 0)			/* minimum delay is 1 second */
		delay = 1;
	delay *= 1000UL;

	bs = strrchr(path,'\\');
	bs[1] = '\0';
	strcat(path, *which == 'M' ? "MUPFEL.APP" : "GEMINI.APP");
	evnt_timer((int)(delay & 0xFFFF), (int)(delay >> 16));

	if (Kbshift(-1)==0)
		shel_write(1,1,1,path,(char *)&notail);

	appl_exit();

	ourexit(0);
}
