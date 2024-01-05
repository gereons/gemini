/*
 * mkargcv.c - build argc/argv from command line
 * 07.12.90
 */

#include <ctype.h>
#include <stdio.h> 
#include <string.h>
#include <tos.h>

#include "alias.h"
#include "alloc.h"
#include "batch.h"
#include "chario.h"
#include "cmdline.h"
#include "curdir.h"
#include "environ.h"
#include "expand.h"
#include "mkargcv.h"
#include "mupfel.h"
#include "redir.h"
#include "shellvar.h"

SCCS(mkargcv);

static char *clp;
static int argc;
static int prefixdir;
static char **myargv;

static int whitespace(char c)
{
	return (c==' ') || (c=='\t');
}

static void skipwhite(void)
{
	while (whitespace(*clp))
		++clp;
}

static int doalias(char *alias)
{
	int retcode = lastchr(alias)!=' ';
	
	strins(clp,alias,0);
	strins(clp," ",strlen(alias));
	
	return retcode;
}

int argcpy(char **argv,char *str)
{
	char *token;
	int offs;
	
	if (prefixdir)
	{
		token = malloc(strlen(str)+strlen(getdir())+4);
		*token = '\0';
		sprintf(token,"%c:%s",getdrv(),getdir());
		strlwr(token);
		offs = *str=='\\';
		if (!offs)
			chrapp(token,'\\');
		strcat(token,&str[offs]);
		argv[argc++] = token;
	}
	else
		argv[argc++] = strdup(str);

	if (argc==MAXARGC)
	{
		eprintf("%s: " MA_TOOLONG "\n",argv[0]);
		return FALSE;
	}
	else
	{
		if (prefixdir)	/* Got offset 1 instead of 0 */
			--str;
		*str='\0';
		return TRUE;
	}
}

static int error(char **argv)
{
	int i;
	
	for (i=0; i<argc; ++i)
		free(argv[i]);
	return -1;
}

static int legalchar(char ch)
{
	return isalnum(ch) || strchr("+_#?*",ch);
}

static int envvar(char *argstr)
{
	char *ep, varname[40], endchar='\0';
	
	*varname='\0';
	++clp;
	if (*clp=='{')
	{
		++clp;
		endchar='}';
	}
		
	while (*clp!='\0' && *clp!=';' && (legalchar(*clp) || *clp==endchar))
	{
		if (*clp!=endchar)
			chrcat(varname,*clp++);
		else
		{
			++clp;
			break;
		}
	}
	
	if (execbatch && !strcmp(varname,"*"))
	{
		int i;
		char s[300];
		
		for (i=1; i<curline->batch.argc; ++i)
		{
			strcpy(s,curline->batch.argv[i]);
			if (!argcpy(myargv,s))
				return error(myargv);
		}
		return FALSE;
	}
	if (strlen(varname)>0 && (ep=getenv(varname))!=NULL)
	{
		strcat(argstr,ep);
		return TRUE;
	}
	else
		return FALSE;
}

static int quote(char *argstr,char delim)
{
	++clp;
	while (*clp!='\0')
	{
		if (*clp==delim)
		{
			++clp;
			return TRUE;
		}
		if (*clp=='$' && delim=='"')
			envvar(argstr);
		else
			chrcat(argstr,*clp++);
	}
	return *clp!='\0';
}

static void doslashconv(char *str)
{
	char *sl;

	if (getvar("slashconv") != NULL)
	{
		
		while ((sl=strchr(str,'/'))!=NULL)
			*sl = '\\';
	}
}

static void doredirect(char *argstr)
{
	++clp;
	skipwhite();
	while (*clp!='\0' && *clp!=';' && !whitespace(*clp))
		chrcat(argstr,*clp++);
	skipwhite();
}

static int redirtoken(char *argstr,int auxout)
{
	int isout;
	
	isout = *clp=='>';
	if (!strncmp(clp,">>",2))
	{
		if (auxout)
			redirect.aux.append = TRUE;
		else
			redirect.out.append = TRUE;	
		++clp;
	}
	else
	{
		if (auxout)
			redirect.aux.append = FALSE;
		else
			redirect.out.append = FALSE;
	}
	if (*(clp+1)=='!')
	{
		if (auxout)
			redirect.aux.clobber=TRUE;
		else
			redirect.out.clobber=TRUE;
		++clp;
	}
	else
	{
		if (auxout)
			redirect.aux.clobber=FALSE;
		else
			redirect.out.clobber=FALSE;
	}
	doredirect(argstr);
	if (argstr[0] == '$')
	{
		char *oldclp = clp;
		char tmpstr[CMDLINESIZE];
		
		clp = argstr;
		*tmpstr = '\0';
		if (envvar(tmpstr))
		{
			while (*clp!='\0' && *clp!=';' && !whitespace(*clp))
				chrcat(tmpstr,*clp++);
			strcpy(argstr,tmpstr);
			clp = oldclp;
		}
		else
		{
			clp = oldclp;
			eprintf(MA_ILLREDIR "\n");
			return FALSE;
		}
	}
	if (*argstr == '\0')
	{
		eprintf(MA_EOF "\n");
		return FALSE;
	}
	doslashconv(argstr);
	if (isout)
		newstr(auxout ? &redirect.aux.file : &redirect.out.file,argstr);
	else
		newstr(&redirect.in.file,argstr);
	*argstr='\0';
	auxout = FALSE;
	return TRUE;
}

static void initredir(redirinfo *r)
{
	r->file = NULL;
	r->append = r->clobber = FALSE;
}

/* build argv array from cmdline, return argc */
int mkargcv(char *cmdline,char **argv)
{
	char delim, argstr[CMDLINESIZE];
	char *alias;
	int l, envok=TRUE, quoteok=TRUE, auxout = FALSE;
	int didalias = FALSE;

	myargv = argv;
	unusedalias();			/* set all aliases to "unused" */
	initredir(&redirect.in);
	initredir(&redirect.out);
	initredir(&redirect.aux);

	clp=cmdline;

restart:	/* restart here when argv[0] is an alias */
	while (whitespace(*clp) || *clp==';')
		++clp;
	l=(int)strlen(clp);
	if (l==0 || *clp=='#')
		return 0;
	while (whitespace(clp[l-1]))
	{
		clp[l-1]='\0';
		--l;
	}
	if (l==0)
		return 0;

	argc=0;
	*argstr = '\0';

	while (*clp!='\0' && *clp!=';')
	{
		switch (*clp)
		{
			case '2':
				if (*(clp+1)!='>')
				{
					chrcat(argstr,*clp++);
					break;
				}
				else
				{
					++clp;
					auxout=TRUE;
				}
			case '>':
			case '<':
				if (*argstr!='\0')
				{
					chrcat(argstr,*clp++);
					break;
				}
				if (!redirtoken(argstr,auxout))
					return error(argv);
				break;
			case '$':
				envok=envvar(argstr);
				if (envok == -1)
					return -1;
				break;
			case ' ':
			case '\t':
				skipwhite();
				if (*clp == '\0' || *clp==';')
					break;
				if (envok)
				{
					doslashconv(argstr);
					prefixdir = (*argstr == '~');
					if (wildcard(argstr))
					{
						if (!expand(argv,&argstr[prefixdir]))
							return error(argv);
						*argstr='\0';
					}
					else
					{
						if (!argcpy(argv,&argstr[prefixdir]))
							return error(argv);
					}
				}
				envok=TRUE;
				break;
			case '@':
				++clp;
				if (*clp!='\0')
					chrcat(argstr,*clp);
				++clp;
				break;
			case '"':
			case '\'':
				delim = *clp;
				quoteok = quote(argstr,delim);
				if (quoteok && (whitespace(*clp) || *clp=='\0' || *clp==';'))
				{
					prefixdir = FALSE;
					if (!argcpy(argv,argstr))
						return error(argv);
					else
					{
						skipwhite();
						break;
					}
				}
				break;
			case '#':
				if (*argstr == '\0')
				{
					*clp = '\0';
					break;
				}
			default:
				chrcat(argstr,*clp++);
				break;
		}
		/* check argv[0] for alias */
		if (!didalias && argc==1)
		{
			alias = isalias(argv[0]);
			if (alias == (char *)-1)
			{
				eprintf(MA_ALOOP "\n");
				return error(argv);
			}
			if (alias != NULL)
			{
				didalias = doalias(alias);
				free(argv[0]);
				argc=0;
				goto restart;
			}
		}
	}

	if (*clp==';')
	{
		/* line continues, save rest */
		*clp = '\0';
		pushline(clp+1);
	}
	
	if (quoteok)
	{
		if (envok)
		{
			doslashconv(argstr);
			prefixdir = (*argstr == '~');
			if (wildcard(argstr))
			{
				if (!expand(argv,&argstr[prefixdir]))
					return error(argv);
			}
			else
			{
				if (*argstr!='\0')
				{
					if (!argcpy(argv,&argstr[prefixdir]))
						return error(argv);
				}
			}
		}
		argv[argc]=NULL;
		/* check argv[0] for alias */
		if (!didalias && argc==1)
		{
		 	alias = isalias(argv[0]);
		 	if (alias == (char *)-1)
		 	{
		 		eprintf(MA_ALOOP "\n");
		 		return error(argv);
		 	}
		 	if (alias != NULL)
		 	{
				didalias = doalias(alias);
				free(argv[0]);
				argc=0;
				goto restart;
			}
		}
		return argc;
	}
	else
		eprintf(MA_QUOTE "\n",delim);
	return error(argv);
}
