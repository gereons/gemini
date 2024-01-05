/*
 * @(#) Gemini\applmana.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: functions to manage start of applications
 */

#include <stdlib.h>
#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "applmana.h"
#include "appledit.h"
#include "util.h"
#include "fileutil.h"
#include "myalloc.h"
#include "venuserr.h"
#include "wildcard.h"
#include "select.h"
#include "stand.h"
#include "files.h"

store_sccs_id(applmana);

/* externals
 */
extern DeskInfo NewDesk;

/* internal texts
 */
#define NlsLocalSection "G.applmana.c"
enum NlsLocalText{
T_MORE1,		/*Was wollen Sie mit %s machen?*/
T_MORE2,		/*An[sehen|[Drucken|[Abbruch*/
};


/*
 * void applDialog(void)
 * make dialog for application installation
 */
void applDialog(void)
{
	WindInfo *wp;
	FileInfo *pf;
	IconInfo *pii;
	char fname[MAX_FILENAME_LEN], tmp[MAXLEN];
	char *applname, *path, *label;
	word index, mode;
	
	applname = path = label = NULL;
	mode = 0;
	
	if(getOnlySelected(&wp, &index))
	{
		switch (wp->kind)
		{
			case WK_FILE:
				pf = GetSelectedFileInfo(wp);
				if (!(pf->attrib & FA_FOLDER)
					&& isExecutable(pf->fullname))
				{
					applname = pf->fullname;
					path = wp->path;
					label = wp->label;
				}
				break;
			case WK_DESK:
				pii = getIconInfo(&wp->tree[index]);
				if (pii && (pii->type == DI_PROGRAM)
					&& getBaseName(fname, pii->path)
					&& isExecutable(fname))
				{
					applname = fname;
					label = pii->label;
					strcpy(tmp, pii->path);
					stripFileName(tmp);
					path = tmp;
				}
				break;
			default:
				break;
		}
	}

	if (applname && !getStartMode(applname, &mode))
		applname = NULL;
	
	EditApplList(applname, path, label, mode);
}

/*
 * make recursiv Configuration-Infos for applicationrules
 */
static ConfInfo *rekursivApplConf (ConfInfo *aktconf, ApplInfo *pai,
					char *buffer)
{
	
	if (!pai)
		return aktconf;

	aktconf = rekursivApplConf (aktconf, pai->nextappl, buffer);
	
	aktconf->nextconf = malloc (sizeof (ConfInfo));

	if (aktconf->nextconf)
	{
		ConfInfo *prevconf;
		char *wcard;

		prevconf = aktconf;
		aktconf = aktconf->nextconf;
		aktconf->nextconf = NULL;
		
		if (strlen (pai->wildcard))
			wcard = pai->wildcard;
		else
			wcard = " ";
			
		sprintf (buffer, "#A@%d@%s@%s@%s@%s",
				pai->startmode, pai->path, pai->name,
				wcard, pai->label);
				
		aktconf->line = malloc (strlen (buffer) + 1);
		
		if(aktconf->line)
			strcpy (aktconf->line, buffer);
		else
		{					/* malloc failed restore aktconf */
			free (aktconf);
			aktconf = prevconf;
			aktconf->nextconf = NULL;
		}
	
		pai = pai->nextappl;
	}
	
	return aktconf;
}

/*
 * ConfInfo *makeApplConf(ConfInfo *aktconf)
 * make Configuration-Infos for List of applicationrules
 */
ConfInfo *makeApplConf(ConfInfo *aktconf, char *buffer)
{
	return rekursivApplConf (aktconf, applList, buffer);
}

/*
 * void addApplRule(char *line)
 * scan line for applicationrule and add it to List
 */
void addApplRule(const char *line)
{
	ApplInfo ai;
	char *cp,*tp;
	
	cp = tmpmalloc(strlen(line) + 1);
	if(cp)
	{
		strcpy(cp, line);
		strtok(cp, "@\n");
		if((tp = strtok(NULL, "@\n")) != NULL)
		{
			ai.startmode = atoi(tp);
			if((tp = strtok(NULL, "@\n")) != NULL)
			{
				strcpy(ai.path, tp);
				if((tp = strtok(NULL, "@\n")) != NULL)
				{
					strcpy(ai.name, tp);
					if((tp = strtok(NULL, "@\n")) != NULL)
						strcpy(ai.wildcard, tp);
					if(!tp || !strcmp(ai.wildcard, " "))
						ai.wildcard[0] = '\0';

					if((tp = strtok(NULL, "@\n")) != NULL)
						strcpy(ai.label, tp);
					else
						ai.label[0] = '\0';
			
					insertApplInfo(&applList, NULL, &ai);
				}
			}
		}
		tmpfree(cp);
	}
}

/*
 * void freeApplRules(void)
 * Liste der applicationrules freigeben
 */
void freeApplRules(void)
{
	FreeApplList(&applList);
}


static word askDefaultAppl(const char *file, char *program,
							 char *label, word *mode)
{
	char tmp[1024];
	
	strcpy(label,"");
	*mode = TTP_START;
	
	sprintf(tmp, NlsStr(T_MORE1), file);
	switch (DialAlert(ImSqQuestionMark(), tmp, 0, NlsStr(T_MORE2)))
	{
		case 0:
			strcpy(program,"more");
			break;
		case 1:
			strcpy(program,"print");
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

/*
 * word getApplForData(const char *name,char *program,char *label)
 * get program responsible for data-file "name"
 * return if an application was found
 */
word getApplForData(const char *name,char *program,
					char *label, word *mode)
{
	ApplInfo *pai;

	*mode = 0;	
	pai = applList;
	while(pai)
	{
		if(strlen(pai->wildcard) && filterFile(pai->wildcard,name))
		{
			strcpy(program,pai->path);
			addFileName(program,pai->name);
			strcpy(label,pai->label);
			*mode = pai->startmode;
			return TRUE;
		}
		pai = pai->nextappl;
	}
	return askDefaultAppl(name, program, label, mode);
}

static word isGemExtender(const char *ext)
{
	static char defGem[] = {"PRG;APP"};
	char *pSuffix,*cp,*mySuffix;
	size_t len;

	pSuffix = getenv("GEMSUFFIX");
	if(pSuffix)
	{
		len = strlen(pSuffix) + 1;
	}
	else
	{
		len = strlen(defGem) + 1;
		pSuffix = defGem;
	}
	cp = tmpmalloc(len);
	if(cp)
	{
		strcpy(cp,pSuffix);
		strupr(cp);
		mySuffix = cp;
		
		if((cp = strtok(mySuffix,";,")) != NULL)
		{
			do
			{
				if(!strcmp(cp,ext))
				{
					tmpfree(mySuffix);
					return TRUE;
				}
					
			} while((cp = strtok(NULL,";,")) != NULL);

		}
		tmpfree(mySuffix);
	}
	return FALSE;		/* malloc failed or fallen through */
}

/*
 * word getStartMode(const char *name, word *mode)
 * get startmode for a program
 * return if something was found
 */
word getStartMode(const char *name, word *mode)
{
	ApplInfo *pai;
	char *cp;
	
	pai = applList;
	while(pai)
	{
		if(!strcmp(name, pai->name))
		{
			*mode = pai->startmode;
			return TRUE;
		}
		else
			pai = pai->nextappl;
	}
	cp = strrchr(name, '.');
	if(cp == NULL)
		return FALSE;
		
	cp++;
	if(isGemExtender(cp))
		*mode = (GEM_START|WCLOSE_START);
	else if(!strcmp(cp,"TTP"))
		*mode = TTP_START;
	else if(!strcmp(cp,"TOS"))
		*mode = TOS_START;
	else if(!strcmp(cp,"ACC"))
		*mode = (GEM_START);
	else
#if MERGED
		*mode = (TOS_START);
#else
		*mode = (TOS_START|WCLOSE_START);
#endif

	if (NewDesk.waitKey
		&& ((*mode & TOS_START) || (*mode & TTP_START)))
	{
		*mode |= WAIT_KEY;
	}
	
	if (NewDesk.ovlStart)
	{
		*mode |= OVL_START;
	}
	return TRUE;
}

void ApRenamed(const char *newname,
				const char *oldname, const char *path)
{
	ApplInfo *pai;
	
	pai = applList;
	while(pai)
	{
		if((!strcmp(oldname,pai->name)) && (!strcmp(path, pai->path)))
		{
			strcpy(pai->name, newname);
			return;
		}
		pai = pai->nextappl;
	}
}

void ApNewLabel(char drive, const char *oldlabel, const char *newlabel)
{
	ApplInfo *pai;
	
	pai = applList;
	while(pai)
	{
		if ((drive == pai->path[0])
			&& (!strcmp(oldlabel, pai->label)))
		{
			strcpy(pai->label, newlabel);
		}
		pai = pai->nextappl;
	}
}

word removeApplInfo(const char *name)
{
	return deleteApplInfo(&applList, name);
}
