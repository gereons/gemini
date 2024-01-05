/*
 * exarg.c
 * Beispielimplementation fÅr Atari's EXARG-Verfahren
 * 12.11.89
 * Geschrieben mit Turbo-C v1.1
 */
 
#include <stdio.h>
#include <string.h>
#include <tos.h>

extern BASPAG *_BasPag;

void showargs(int argc, char **argv)
{
	int i;

	printf("argc = %d\n",argc);	
	for (i=0; i<argc; ++i)
		printf("argv[%d] = %s\n",i,argv[i]);
}

int main(int argc, char **argv)
{
	char *environ = _BasPag->p_env;
	char *argenv = NULL;
	
	if (_BasPag->p_cmdlin[0] != (char)127)
	{
		/* EXARG not used */
		printf("args passed normally\n");
	}
	else
	{
		/* 
		 * EXARG used. set argc and argv[] to the args
		 */
		printf("using EXARG\n");

		while (*environ)
		{
			if (strncmp(environ,"ARGV=",5))
				environ += strlen(environ)+1;
			else
			{
				/* found ARGV= at environ. skip the value */
				argenv = environ + strlen(environ) + 1;
				/* 
				 * set the 'A' to '\0', so that child processes
				 * won't see the args
				 */
				*environ = '\0';
			}
		}
		
		if (argenv == NULL)
			printf("oops! ARGV not in environment!\n");
		else
			printf("args found at address %p\n",argenv);

		argc = 0;
		do
		{
			argv[argc++] = argenv;
			argenv += strlen(argenv)+1;
		} while (*argenv != '\0');
	}

	/* display the arguments */
	showargs(argc,argv);
	return 0;
}
