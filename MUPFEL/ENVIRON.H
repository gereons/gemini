/*
 * environ.h - definitions for environ.c
 * 24.01.90
 */

#ifndef _M_ENVIRON
#define _M_ENVIRON

char *getenv(const char *name);	/* get env var */
int putenv(char *var);			/* set / delete env var */
int envnameok(char *s);			/* check legality of name */
char *makeenv(int exarg,int argc,char **argv,char *cmdpath);
							/* build env for Pexec */
void envinit(char **envp);		/* init environment */
void envexit(void);

extern char *suffenv;

#endif