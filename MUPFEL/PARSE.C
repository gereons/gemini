/*
 * parse.c  -  parse command lines, execute commands
 * 07.02.91
 */

#include <ctype.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <tos.h>

#include "alias.h"
#include "attrib.h"
#include "batch.h"
#include "chario.h"
#include "check.h"
#include "commands.h"
#include "curdir.h"
#include "environ.h"
#include "exec.h"
#include "hash.h"
#include "mupfel.h"
#include "parse.h"
#include "redir.h"
#include "shellvar.h"
#include "toserr.h"

/* call checkcode() after internal commands? */
#define CHECK	0

SCCS(parse);

char cmdpath[CMDLINESIZE];
int icmd, iserrfile;

/*
 * findintern(char *cmd)
 * search cmd in list of internal commands
 * return index if found, -1 otherwise
 */
int findintern(const char *cmd)
{
	int i;
	
	for (i=0; i<interncount; ++i)
		if (!stricmp(interncmd[i].name,cmd))
			return i;
	return -1;
}

/*
 * allupper(char *str)
 * return TRUE if all letters in str are uppercase, FALSE otherwise
 */
int allupper(const char *str)
{
	int uppers=0, lowers=0;
	char c;
	
	while ((c=*str)!=0)
	{
		if (isalpha(c))
		{
			if (isupper(c))
				++uppers;
			else
				++lowers;
		}
		++str;
	}
	if (lowers>0 || uppers==0)
		return FALSE;
	else
		return TRUE;
}

int extinsuffix(const char *filename)
{
	char *dot = strrchr(filename,'.');
	char *found, ch;
	
	if (dot==NULL || suffenv==NULL)
		return FALSE;

	++dot;
	if ((found=strstr(suffenv,dot))==NULL)
		return FALSE;
		
	ch = *(found+strlen(dot));
	if (ch==';' || ch==',' || ch=='\0')
	{
		if (found>suffenv)
		{
			if (*(found-1)==';' || *(found-1)==',')
				return TRUE;
		}
		else
			return TRUE;
	}
	return FALSE;
}

/*
 * int canexec(char *cmd)
 * Purpose:
 *	Determine if <cmd> is an executable file
 * Returns:
 *	EXEC_NOTFOUND: Not executable
 *	EXEC_BINARY  : Binary executable
 *	EXEC_BATCH   : Batchfile
 */
exectype canexec(const char *cmd)
{
	DTA mydta, *olddta = Fgetdta();
	char *command = strlwr(strdup(cmd));
	exectype filetype;
	int exists;

	bioserr = 0;
	Fsetdta(&mydta);
	exists = !Fsfirst(command,FA_FILES);
	Fsetdta(olddta);

	filetype = EXEC_NOTFOUND;
	if (exists)
	{
		if (!strcmp(&command[strlen(command)-4],".mup"))
			filetype = EXEC_BATCH;
		else
			if (extinsuffix(command) || access(command,A_EXEC))
				filetype = EXEC_BINARY;
	}
	
	if (bioserr)
		ioerror(NULL,cmd,NULL,bioserr);
	free(command);
	return filetype;
}

/*
 * checkcmd(char *path, char *cmd, int setbatch, int *cost)
 * check if a file named by path and cmd exists with one of the
 * standard extensions for executables. setbatch is passed to
 * openbatch() if that is ever called. *cost is updated for
 * each iteration (used for hash table).
 */
static int checkcmd(char *path, char *cmd, int setbatch, int *cost)
{
	char *dot, *backsl, *suffix, *s;

	bioserr = 0;
 	if (suffenv!=NULL)
 		s = strdup(suffenv);
 	else
 		s = strdup("prg;app;gtp;tos;ttp;mup");

 	suffix = mstrtok(s,";,");
 	while (suffix!=NULL)
	{
		++(*cost);
		strcpy(cmdpath,path);
		if (*cmdpath)
			chrapp(cmdpath,'\\');
		strcat(cmdpath,cmd);
		dot = strrchr(cmd,'.');
		backsl = strrchr(cmd,'\\');
		/*
		 * if no dot or first char or no dot after last backslash
		 * append suffix
		 */
		if (dot==NULL || dot==cmd || (*cmd=='.' && dot==cmd+1) ||
			(backsl!=NULL && dot<backsl))
		{
			chrcat(cmdpath,'.');
			strcat(cmdpath,suffix);
		}
		dot = strrchr(cmdpath,'.') + 1;

		switch (canexec(cmdpath))
		{
			case EXEC_BATCH:
				free(s);
				return openbatch(cmdpath,setbatch);
			case EXEC_BINARY:
				free(s);
				return TRUE;
		}

		if (bioserr)
			break;
		suffix = mstrtok(NULL,";,");
	}
	free(s);
	return FALSE;
}

/*
 * locatecmd(char *cmd, int setbatch, int *cost)
 * try to find cmd in hash table or $PATH. setbatch and cost
 * are passed to checkcmd
 */
int locatecmd(char *cmd, int setbatch, int *cost)
{
	char *path, *p, *hash;
	
	if (setbatch)
 		isbatchcmd = FALSE;

	if (strchr(cmd,'\\')!=NULL || *cmd=='.' || cmd[1]==':')
		return checkcmd("",cmd,setbatch,cost);

	if ((hash=searchhash(cmd,TRUE))!=NULL)
	{
		if (access(hash,A_READ))
		{
			strcpy(cmdpath,hash);
			if (!stricmp(&cmdpath[strlen(cmdpath)-3],"MUP"))
				return openbatch(cmdpath,setbatch);
			else
				return TRUE;
		}
		else
		{
			deletehash(cmd);
			if (bioserror(bioserr))
				ioerror(NULL,cmdpath,NULL,bioserr);
		}
	}

	if ((p=getenv("PATH"))!=NULL)
		path = strdup(p);
	else
		path = strdup(".");

	p=strtok(path,";,");
	while (p!=NULL)
	{
		if (checkcmd(p,cmd,setbatch,cost))
		{
			free(path);
			return TRUE;
		}
		p=strtok(NULL,";,");
	}
	free(path);
	return FALSE;
}

/*
 * execintern(int i,int argc,char **argv)
 * execute internal command #i
 */
static int execintern(int i,int argc,char **argv)
{
	int unexpand, retcode;
	
	if (interncmd[i].needexp && (unexpand=anywild(argc,argv))>-1)
	{
		oserr = EFILNF;
		if (interncmd[i].unexp == NULL)
			eprintf("%s: " PA_NOFILE "\n",argv[0],argv[unexpand]);
		else
			eprintf("%s: %s\n",argv[0],interncmd[i].unexp);
		return 1;
	}
	else
	{
		icmd = i;
		iserrfile = !stricmp(interncmd[i].name,"errorfile");
		actcmd = interncmd[i].name;
		retcode = interncmd[i].func(argc,argv);
#if CHECK
		checkcode();	/* Let's see if we stomped ourselves... */
#endif
		return retcode;
	}
}

/*
 * execextern(int argc,char **argv,int cost)
 * execute external program
 */
static int execextern(int argc,char **argv,int cost)
{
	enterhash(argv[0],cmdpath,cost);
	if (!isbatchcmd)
		return execcmd(cmdpath,argc,argv);
	else
	{
		batchargs(argc,argv,TRUE);
		return 0;
	}
}

static int setenv(char *entry, char *eq)
{
	char *val;
	
	*eq = '\0';

	if (!envnameok(entry))
	{
		*eq = '=';
		return FALSE;
	}
		
	val = eq+1;
	if (*val == '\0')
		val = NULL;
	if (varindex(entry)!=-1)
		return setvar(entry,val);
	if (val!=NULL)
		*eq = '=';
	return putenv(entry);
}

static void batchredir(redirinfo *to, redirinfo *from)
{
	if (to->file == NULL && from->file != NULL)
	{
		memcpy(to,from,sizeof(redirinfo));
		to->file = strdup(from->file);
	}
}

/*
 * parse command line (argc/argv), do redirection, locate argv[0]
 * in internal list, hash table or $PATH, execute commands, free
 * allocated memory for argv
 */
int parse(ARGCV)
{
	int i;
	int cost = 0, retcode = 0;
	char *eq, olddrv;

	if (argc==1)
	{
		if (strlen(argv[0])==2 && argv[0][1]==':')
		{
			olddrv = getdrv();
			retcode = !chdrv(toupper(*argv[0]));
			if (retcode==1 || bioserr)
			{
				eprintf(PA_NODRV "\n",*argv[0]);
				chdrv(olddrv);
			}
			if (isupper(*argv[0]))
				chdir("\\");
			free(argv[0]);
			return retcode;
		}
		if ((eq=strchr(argv[0],'='))!=NULL)
		{
			if (!setenv(argv[0],eq))
			{
				eprintf(EV_ILLNAME "\n",argv[0]);
				retcode = 1;
			}
			free(argv[0]);
			return retcode;
		}
	}
	
	/* Redirect I/O channels */
	if (execbatch())
	{
		/* copy redir params if none given for this command */
		batchredir(&redirect.in,&curline->batch.in);
		batchredir(&redirect.out,&curline->batch.out);
		batchredir(&redirect.aux,&curline->batch.aux);
	}			
	if (!doredir(argv[0]))
	{
		/* didn't work. undo partial redirection if any */
		endredir(TRUE);
		for (i=0; i<argc; ++i)
			free(argv[i]);
		return 1;
	}
	
	if (argc>1 && !strcmp(argv[0],NOALIAS))
	{
		free(argv[0]);
		for (i=0; i<argc-1; ++i)
			argv[i] = argv[i+1];
		--argc;
	}
	
	if (allupper(argv[0]))
	{
		if (locatecmd(argv[0],TRUE,&cost))
			retcode = execextern(argc,argv,cost);
		else
		{		
			if ((i=findintern(argv[0]))!=-1)
				retcode = execintern(i,argc,argv);
			else
			{
				endredir(TRUE);
				oserr = EFILNF;
				eprintf(PA_NOTFOUND "\n",argv[0]);
			}
		}
	}
	else
	{	
		if ((i=findintern(argv[0]))!=-1)
			retcode = execintern(i,argc,argv);
		else /* no match found, try to execute external prg */
		{
			if (locatecmd(argv[0],TRUE,&cost))
				retcode = execextern(argc,argv,cost);
			else
			{
				endredir(TRUE);
				oserr = EFILNF;
				eprintf(PA_NOTFOUND "\n",argv[0]);
			}
		}
	}
	endredir(FALSE);
	/* free memory allocated for argv */
	for (i=0; i<argc; ++i)
		free(argv[i]);
	return retcode;
}
