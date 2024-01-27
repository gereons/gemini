/*
	@(#)mainargv.c
	@(#)Julian F. Reschke, 3. Juli 1990
	
	Startup fÅr Atari-ARGV
	Copyright (c) J. Reschke 1990
	
	Anwendung: im Hauptprogramm
	'main' durch 'argvmain' ersetzen
*/

#include <tos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern int argvmain (int, char **, char **);

int main (int argc, char *argv[], char *envp[])
{
	char **myargv;
	extern BASPAG *_BasPag;
	char *env;
	char *startpar;
	int count = 0;
	int i;

	/* Flag fÅr Verwendung von ARGV */
	if (_BasPag->p_cmdlin[0] != 127)
		return argvmain (argc, argv, envp);

	/* Zeiger auf Env-Var merken */
	env = getenv("ARGV");
	if (!env)
		return argvmain (argc, argv, envp);
		
	/* alle weiteren envp's lîschen */
	i = 0;
	while (strncmp (envp[i], "ARGV", 4)) i++;
	envp[i] = NULL;	

	/* alles, was dahinter kommt, abschneiden */	
	if (env[0] && env[-1])
	{
		*env++ = 0;			/* kill it */
		while (*env++);
	}
	
	/* Parameterstart */
	startpar = env;
	
	while (*env)
	{
		count++;
		while (*env++);
	}
	
	/* Speicher fÅr neuen Argument-Vektor */
	myargv = Malloc ((count+1)*sizeof (char *));
	env = startpar;
	
	count = 0;
	while (*env)
	{
		myargv[count++] = env;
		while (*env++);
	}
	myargv[count] = NULL;
	
	/* und ...argvmain() starten */
	count = argvmain (count, myargv, envp);	
	Mfree (myargv);
	return count;
}
