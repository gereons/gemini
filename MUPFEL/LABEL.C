/*
 * label.c  -  internal "label" function
 * 01.10.90
 */
 
#include <ctype.h>
#include <string.h>
#include <tos.h>

#include "attrib.h"
#include "chario.h"
#include "comm.h"
#include "curdir.h"
#include "handle.h"
#include "label.h"
#include "mupfel.h"
#include "toserr.h"

SCCS(label);

char *getlabel(char drv)
{
	static DTA dta;
	DTA *olddta = Fgetdta();
	char label[7];
	int fstat;
	
	Fsetdta(&dta);
	strcpy(label,"x:\\*.*");
	*label=drv;
	fstat = Fsfirst(label,FA_LABEL);
	Fsetdta(olddta);
	
	if (!fstat)
		return dta.d_fname;
	else
		return NULL;
}

static int showlabel(char drv)
{
	char *l;
	
	if (!legaldrive(drv))
	{
		eprintf("label: " LB_NODRV "\n",drv);
		return FALSE;
	}
	if ((l=getlabel(drv))!=NULL)
		mprintf(LB_VOLNAME "\n",drv,l);
	else
		mprintf(LB_NOLBL "\n",drv);
	return TRUE;
}
		
int setlabel(char drv, char *newlabel, const char *cmd)
{
	char *l, label[16];
	int hnd, deletelabel;
	
	deletelabel = (*newlabel == '-');

	strcpy(label,"x:\\xxxxxxxx.xxx");
	*label = drv;

	if (!legaldrive(drv))
	{
		oserr = EDRIVE;
		eprintf("%s: " LB_NODRV "\n",cmd,drv);
		return FALSE;
	}
	if (!deletelabel && strlen(newlabel)>14)
	{
		oserr = EACCDN;
		eprintf("%s: " LB_ILLLBL "\n",cmd);
		return FALSE;
	}
	
	if (!deletelabel)
	{
		strcpy(&label[3],newlabel);
		
		if (access(label,A_EXIST) || isdir(label))
		{
			oserr = EACCDN;
			eprintf("%s: " LB_EXISTS "\n",cmd,label);
			return FALSE;
		}
	}
	
	if ((l=getlabel(drv))!=NULL)
	{
		strcpy(&label[3],l);

		if ((hnd=Fcreate(label,0))<MINHND)
		{
			oserr = EACCDN;
			eprintf("%s: " LB_RMLBL "\n",cmd);
			return FALSE;
		}
		Fclose(hnd);
		Fdelete(label);
	}
	if (!deletelabel)
	{
		strcpy(&label[3],newlabel);
		if ((hnd=Fcreate(label,FA_LABEL))<MINHND)
		{
			oserr = EACCDN;
			eprintf("%s: " LB_CANTSET "\n",cmd,drv);
			return FALSE;
		}
		else
			Fclose(hnd);
	}
	CommInfo.dirty |= drvbit(label);
	return TRUE;
}	

int m_label(ARGCV)
{
	int l, retcode = 1;
	
	switch (argc)
	{
		case 1:
			retcode = showlabel(getdrv());
			break;
		case 2:
			l=(int)strlen(argv[1]);
			if (l==1 || (l==2 && argv[1][1]==':'))
				retcode = showlabel(toupper(argv[1][0]));
			else
			{
				eprintf("label: " LB_ILLLBL " %s\n",argv[1]);
				return printusage(NULL);
			}
			break;
		case 3:
			retcode = setlabel(toupper(argv[1][0]),argv[2],"label");
			break;
		default:
			return printusage(NULL);
	}
	return !retcode;
}
