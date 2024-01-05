/*
 * whereis.c  -  internal "whereis" command
 * 05.01.91
 */
 
#include "alias.h"
#include "chario.h"
#include "hash.h"
#include "mupfel.h"
#include "parse.h"

SCCS(whereis);

static int whereis(char *cmd)
{
	int dummy, retcode;
	char *hash, *alias;
	
	retcode = 0;
	if ((hash=searchhash(cmd,FALSE))!=NULL)
	{
		mprintf(WH_HASHED "\n",cmd,hash);
		return 0;
	}
	if (allupper(cmd))
	{
		if (locatecmd(cmd,FALSE,&dummy))
			mprintf(WH_IS "\n",cmd,cmdpath);
		else
		if ((alias=searchalias(cmd))!=NULL)
			mprintf(WH_ALIAS "\n",cmd,alias);
		else
		if (findintern(cmd)!=-1)
			mprintf(WH_BUILTIN "\n",cmd);
		else
		{
			mprintf(WH_NOTFOUND "\n",cmd);
			retcode = 1;
		}
	}
	else
	{
		if ((alias=searchalias(cmd))!=NULL)
			mprintf(WH_ALIAS "\n",cmd,alias);
		else
		if (findintern(cmd)!=-1)
			mprintf(WH_BUILTIN "\n",cmd);
		else
		if (locatecmd(cmd,FALSE,&dummy))
			mprintf(WH_IS "\n",cmd,cmdpath);
		else
		{
			mprintf(WH_NOTFOUND "\n",cmd);
			retcode = 1;
		}
	}
	return retcode;
}

/*
 * m_whereis(ARGCV)
 * locate argv[1] in internal command table, hash table, $PATH or
 * alias list
 */
int m_whereis(ARGCV)
{
	int i;
	
	if (argc==1)
		return printusage(NULL);
	else
		for (i=1; i<argc; ++i)
			whereis(argv[i]);		
	return 0;
}
