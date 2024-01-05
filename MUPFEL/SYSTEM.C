/*
 * system.c  -  Mupfel-Kommando via _shell_p ausfÅhren
 * 17.11.89
 */
 
#include <tos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * MUPFEL_ONLY:
 * 	0 wenn's auch mit anderen Shells gehen soll
 * 	1 fÅr Checks ob MUPFEL bzw GEMINI wirklich da ist
 */
#define MUPFEL_ONLY 1

#define _SHELL_P ((long *)0x4f6L)

#if MUPFEL_ONLY
#define SHELL_OK	(do_sys!=0 && (!strncmp(xbra_id,"XBRAMUPF",8) || !strncmp(xbra_id,"XBRAGMNI",8)))
#else
#define SHELL_OK	(do_sys!=0)
#endif

/*
 * int system(const char *cmd)
 * FÅhrt ein Kommando Åber die in _shell_p installierte Shell aus.
 * Ohne Shell gibt's -1 als Returnwert, ansonsten
 * den Returncode des ausgefÅhrten Kommandos.
 * Die Mupfel-interne Routine erwartet den Pointer auf die Kommando-
 * zeile auf dem Stack und gibt den Returncode des ausgefÅhrten
 * Kommandos in Register D0.W zurÅck.
 */
int system(const char *cmd)
{
	/* Parameter auf dem Stack Åbergeben! */
	int cdecl (*do_sys)(const char *cmd);
	char *xbra_id;
	long oldssp;

	oldssp = Super(0L);
	do_sys = (void (*))*_SHELL_P;
	Super((void *)oldssp);
	xbra_id = (char *)((long)do_sys - 12);
	
	if (cmd==NULL)
		return SHELL_OK;

	if (SHELL_OK)
		return do_sys(cmd);
	else
		return -1;
}

/*
 * Testprogramm fÅr system().
 * Aus allen Argumenten wird wieder ein einziges zusammengebastelt,
 * das dann per system() an Mupfel Åbergeben wird.
 */
int main(int argc,char **argv)
{
	int i, ex_code;
	char str[256];
	
	printf("Shell status: %d\n",system(NULL));
	*str = '\0';
	for (i=1; i<argc; ++i)
	{
		strcat(str,argv[i]);
		strcat(str," ");
	}
	printf("system(%s)\n",str);
	ex_code = system(str);
	if (ex_code == -1)
		printf("Mupfel nicht da?!?\n");
	else
		printf("Returncode %d\n",ex_code);
	return 0;
}
