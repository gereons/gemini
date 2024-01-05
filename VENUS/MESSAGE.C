/*
 * @(#)Gemini\Message.c
 * @(#)Stefan Eissing, 22. Mai 1991
 *
 * project: venus
 *
 * description: modul for handling messages
 *
 */

#include <string.h>
#include <aes.h>
#include <flydial\flydial.h>
#include <flydial\evntevnt.h>
#include <nls\nls.h>

#include "vs.h"
#include "vaproto.h"
#include "message.h"
#include "stand.h"
#include "myalloc.h"
#include "util.h"
#include "fileutil.h"
#include "venuserr.h"
#include "redraw.h"
#include "select.h"
#include "filedraw.h"
#include "window.h"
#include "menu.h"
#include "fileutil.h"
#include "scroll.h"
#include "dispatch.h"
#include "accwind.h"
#include "draglogi.h"
#if MERGED
#include "mvwindow.h"
#endif

store_sccs_id(venus);

#define NlsLocalSection "G.message"
enum NlsLocalText{
T_NOACC,		/*Das Accessory %s ist dem AES nicht bekannt!*/
};

/* externals
 */
extern word apid;
extern WindInfo wnull;

#define MESSLEN		512
				/* LÑnge des Buffers fÅr Nachrichten an Acc's */

/*internals
 */
typedef struct
{
	word accId;
	char name[9];
	word protoStatus;
	char *status;
} AccInfo;

static word accMax = 0;
static word accCount = 0;
static AccInfo *accArray = NULL;
static char message[MESSLEN];

static word getAccInfo(word accid)
{
	word i;
	
	for (i = 0; i < accCount; ++i)
	{
		if (accArray[i].accId == accid)
			return i;
	}
	return -1;
}

static word addAccInfo(word accid, word proto, const char *name)
{
	word entry = -1;
	
	entry = getAccInfo(accid);

	if (entry < 0)
	{
		if (accCount >= accMax)
		{
			/* Array vergrîûern */
			AccInfo *tmp;
		
			tmp = malloc((accMax + 6) * sizeof(AccInfo));
			if(!tmp)
				return -1;
				
			if (accArray)
			{
				memcpy(tmp, accArray, accMax * sizeof(AccInfo));
				free(accArray);
			}
			accMax += 6;
			accArray = tmp;
		}
		
		entry = accCount++;
		accArray[entry].status = NULL;
	}
		
	accArray[entry].accId = accid;
	if (name)
		strcpy(accArray[entry].name, name);
	else
		accArray[entry].name[0] = '\0';
	accArray[entry].protoStatus = proto;

	return entry;
}

static word storeAccInfoStatus(word entry, const char *line)
{
	char *cp;
	
	if (accArray[entry].status)
	{
		free(accArray[entry].status);
		accArray[entry].status = NULL;
	}
	if (!line)
		return TRUE;
		
	cp = malloc(strlen(line));
	if (!cp)
		return FALSE;
	strcpy(cp, line);
	accArray[entry].status = cp;
	return TRUE;
}

word WriteAccInfos(word fhandle, char *buffer)
{
	word i;
	
	for (i = 0; i < accCount; ++i)
	{
		if ((accArray[i].name[0]) && (accArray[i].protoStatus & 1)
			&& (accArray[i].status))
		{
			strcpy(buffer, "#V");
			strcat(buffer, accArray[i].name);
			strcat(buffer, accArray[i].status);
			if (!Fputs(fhandle, buffer))
				return FALSE;
		}
	}
	return TRUE;
}

void ExecAccInfo(const char *line)
{
	word accid, entry;
	char name[9];
	
	strncpy(name, &line[2], 8);
	name[8] = '\0';
	
	accid = appl_find(name);
	if (accid < 0)
		return;
	
	entry = addAccInfo(accid, 1, name);
	if (entry < 0)
		return;
	
	storeAccInfoStatus(entry, &line[10]);
}

static void sendAccStatus(word messbuff[8])
{
	word mbuff[8];
	word i;
	
	mbuff[0] = VA_SETSTATUS;
	mbuff[1] = apid;
	mbuff[2] = 0;
	
	for (i = 0; i < accCount; ++i)
	{
		if ((accArray[i].accId == messbuff[1]) 
			&& (accArray[i].protoStatus & 1))
		{
			*(char **)(mbuff+3) = accArray[i].status;
			appl_write(messbuff[1], 16, mbuff);
		}
	}
}

static void initAcc(word apid, word accid)
{
	word i,messbuff[8], entry;

	entry = getAccInfo(accid);
	if (entry < 0)
	{
		entry = addAccInfo(accid, 0, NULL);
		if (entry < 0)
			return;
	}
	
	messbuff[0] = ACC_ACC;
	messbuff[7] = accid;
	messbuff[1] = apid;
	messbuff[2] = 0;
	for (i = 0; i < accCount; i++)
	{
		if (accArray[i].accId != accid)
			appl_write(accArray[i].accId, 16, messbuff);
	}

	messbuff[0] = ACC_ID;
	messbuff[3] = 0x1200;  /* Version 1.2, Protokollstufe 0 */
	*((char **)(messbuff+4)) = PGMNAME ".APP";
	messbuff[6] = 0;      /* oder etwas anderes */
	messbuff[7] = 0;      /* oder etwas anderes */
	appl_write(accid, 16, messbuff);
}

static void sentProtoStatus(word messbuff[8])
{
	word buff[8];
	
	addAccInfo(messbuff[1], messbuff[3], *(char **)(messbuff+6));
	
	buff[0] = VA_PROTOSTATUS;
	buff[1] = apid;
	buff[2] = 0;
	buff[4] = buff[5] = 0;
#if MERGED
	buff[3] = 0x01FF;
	*(char **)(buff+6) = "GEMINI  ";
#else
	buff[3] = 0x01FB;
	*(char **)(buff+6) = "VENUS   ";
#endif
	appl_write(messbuff[1], 16, buff);
}

static void saveAccStatus(word messbuff[8])
{
	word entry;
	char *line;
	
	entry = getAccInfo(messbuff[1]);
	if (entry < 0)
	{
		venusDebug("AccStatus: Acc wurde nicht gefunden!");
		return;
	}
	
	line = *(char **)(messbuff+3);

	storeAccInfoStatus(entry, line);
}

static void sendFileFont(word messbuff[8])
{
	word buff[8];
	
	buff[0] = VA_FILEFONT;
	buff[1] = apid;
	buff[2] = 0;
	
	GetFileFont(&buff[3], &buff[4]);
	appl_write(messbuff[1], 16, buff);
}

#if MERGED
static void sendConsoleFont(word messbuff[8])
{
	word buff[8];
	
	buff[0] = VA_CONFONT;
	buff[1] = apid;
	buff[2] = 0;
	
	GetConsoleFont(&buff[3], &buff[4]);
	appl_write(messbuff[1], 16, buff);
}

static void openConsole(word messbuff[8])
{
	word buff[8];
	
	buff[0] = VA_CONSOLEOPEN;
	buff[1] = apid;
	buff[2] = 0;
	
	buff[3] = !doMupfel(NULL, 0);
	appl_write(messbuff[1], 16, buff);
}
#endif

static void askObject(word messbuff[8])
{
	word buff[8];
	char **cpp, *names;
	
	buff[0] = VA_OBJECT;
	buff[1] = apid;
	buff[2] = 0;
	
	cpp = (char **)&buff[3];
	*cpp = message;
	
	names = GetSelectedObjects();

	if (names)
	{
		strncpy(message, names, MESSLEN - 1);
		message[MESSLEN-1] = '\0';
		tmpfree(names);
		names = NULL;
	}
	else
		message[0] = '\0';
		
	appl_write(messbuff[1], 16, buff);
}

static void startProg(word messbuff[8])
{
	word buff[8];
	char name[MAX_FILENAME_LEN];
	char path[MAXLEN], *command, *cp;
	
	buff[0] = VA_PROGSTART;
	buff[1] = apid;
	buff[2] = 0;
	buff[3] = 0;		/* nicht gestartet ist default */
	
	cp = *((char **)&messbuff[3]);
	command = *((char **)&messbuff[5]);

	if (cp && (strlen(cp) < MAXLEN))
	{
		strcpy(path, cp);	
		if (path && getBaseName(name, path))
		{
			stripFileName(path);
			buff[3] = startFile(NULL, 0, TRUE, "", 
								path, name, command);
		}
	}
	appl_write(messbuff[1], 16, buff);
}

static void openWind(word messbuff[8])
{
	word buff[8];
	char **pppath, **ppwild;
	char title[MAXLEN];
	
	buff[0] = VA_WINDOPEN;
	buff[1] = apid;
	buff[2] = 0;
	
	pppath = (char **)&messbuff[3];
	ppwild = (char **)&messbuff[5];
		
	if (*pppath && *ppwild)
	{
		if (!getBaseName(title, *pppath))
			strcpy(title, *pppath);
		else
			strcat(title, "\\");
			
		buff[3] = (openWindow(0, 0, 0, 0, 0, *pppath, *ppwild, title,
					"", WK_FILE) != NULL);
	}
	else
		buff[3] = 0;
	appl_write(messbuff[1], 16, buff);
}

static void accWindowOpened(word messbuff[8])
{
	InsAccWindow(messbuff[1], messbuff[3]);
}

static void accWindowClosed(word messbuff[8])
{
	DelAccWindow(messbuff[1], messbuff[3]);
}

/*
 * Falls Objekte auf ein Acc.-Fenster gezogen wurden, so
 * sende deren Namen an das Acc.; ansonsten gib FALSE zurÅck
 */
word PasteAccWindow(WindInfo *fromwp, word tohandle, word mx, word my)
{
	WindInfo *accwp;
	char *names;
	word buff[8];
	
	accwp = getwp(tohandle);
	
	if ((accwp == NULL) || (accwp->kind != WK_ACC))
		return FALSE;
		
	names = getDraggedNames(fromwp);
	if (names)
	{
		remchr(names, '\'');
		strncpy(message, names, MESSLEN);
		message[MESSLEN-1] = '\0';

		buff[0] = VA_DRAGACCWIND;
		buff[1] = apid;
		buff[2] = 0;
		buff[3] = tohandle;
		buff[4] = mx;
		buff[5] = my;
		*((char **)&buff[6]) = message;
		
		appl_write(accwp->owner, 16, buff);
		free(names);
		return TRUE;
	}

	return FALSE;
}

static void copyObjectsFromAcc (word messbuff[8])
{
	WindInfo *wp = &wnull;
	word buff[8];
	char *target = *((char **)&messbuff[4]);
	/* stevie */

	buff[0] = VA_COPY_COMPLETE;
	buff[1] = apid;
	buff[2] = 0;

	while (wp)
	{
		if (((wp->kind == WK_FILE) || (wp->kind == WK_DESK))
			&& (wp->selectAnz > 0))
			break;
		
		wp = wp->nextwind;
	}
	
	if (wp && (strlen (target) < MAXLEN-1))
	{
		char from_path[MAXLEN], to_path[MAXLEN];
		
		MarkDraggedObjects (wp);

		strcpy (from_path, wp->path);
		strcpy (to_path, target);
		buff[3] = PerformCopy (wp, from_path, to_path, 
			messbuff[3] & K_CTRL);
	
		UnMarkDraggedObjects (wp);
	}
	else
		buff[3] = FALSE;		/* nicht impl. -> FALSE */
	
	appl_write(messbuff[1], 16, buff);
}

word HandleMessage(word messbuff[8], word kstate)
{
	WindInfo *wp;
	word ok = TRUE;
	
#if MERGED
	if ((wp = getMupfWp()) != NULL)
		moveMWindow(wp);	/* built new rectangle list */
#endif

	switch(messbuff[0])
	{
		case MN_SELECTED: 
			ok = doMenu(messbuff[3], messbuff[4], kstate);
			break;
		case WM_TOPPED:
		case WM_NEWTOP:
			DoTopWindow (messbuff[3]);
			break;
		case WM_REDRAW:
			redraw(getwp(messbuff[3]), &messbuff[4]);
			break;
		case WM_CLOSED:
			closeWindow(messbuff[3], FALSE);
			break;
		case WM_SIZED:
			sizeWindow(messbuff[3], &messbuff[4]);
			break;
		case WM_MOVED:
			moveWindow(messbuff[3],&messbuff[4]);
			break;
		case WM_FULLED:
			fullWindow(messbuff[3]);
			break;
		case WM_ARROWED:
			doArrowed(messbuff[3], messbuff[4]);
			break;
		case WM_HSLID:
			break;
		case WM_VSLID:
			doVslid(messbuff[3], messbuff[4]);
			break;

		/*
		 * xAcc-Protokoll der Stufe 0 von Konrad Hinsen
		 */
		case ACC_ID:
			initAcc(apid, messbuff[1]);
			break;
		/*
		 * Hier beginnt das Venus-Acc-Protokoll
		 */
		case AV_PROTOKOLL:
			sentProtoStatus(messbuff);
			break;
		case AV_GETSTATUS:
			sendAccStatus(messbuff);
			break;
		case AV_STATUS:
			saveAccStatus(messbuff);
			break;
		case AV_ASKFILEFONT:
			sendFileFont(messbuff);
			break;
		case AV_ASKOBJECT:
			askObject(messbuff);
			break;
		case AV_OPENWIND:
			openWind(messbuff);
			break;
		case AV_STARTPROG:
			startProg(messbuff);
			break;
		case AV_ACCWINDOPEN:
			accWindowOpened(messbuff);
			break;
		case AV_ACCWINDCLOSED:
			accWindowClosed (messbuff);
			break;
		case AV_COPY_DRAGGED:
			copyObjectsFromAcc (messbuff);
			break;
#if MERGED
		case AV_ASKCONFONT:
			sendConsoleFont(messbuff);
			break;
		case AV_OPENCONSOLE:
			openConsole(messbuff);
			break;
#endif
	}
	
	return ok;
}

void MessInit(void)
{
}

void MessExit(void)
{
	word i;
	
	if (accArray)
	{
		for (i = 0; i < accCount; ++i)
		{
			if (accArray[i].status)
				free(accArray[i].status);
		}
		free(accArray);
	}
	accArray = NULL;
	accMax = accCount = 0;
}

/*
 * word StartAcc(WindInfo *wp,word fobj,
 *					char *name,char *command)
 * try to start an installed accessory
 */
word StartAcc(WindInfo *wp,word fobj,char *name,char *command)
{
	word i,acc_id;
	word messbuff[8];
	char accname[MAX_FILENAME_LEN],*cp,**pm;
	size_t len;
	
	if((cp = strrchr(name,'\\')) != NULL)
		strcpy(accname,++cp);
	else
		strcpy(accname,name);
		
	if((cp = strchr(accname,'.')) != NULL)
		*cp = '\0';
	len = strlen(accname);
	for(i=1; i <= (8 - len); i++)
		strcat(accname," ");

	if((acc_id = appl_find(accname)) != -1)
	{							/* acc. installed */
		messbuff[0] = 0x4711;
		messbuff[1] = apid;
		messbuff[2] = 0;
		pm = (char **)&messbuff[3];
		*pm = message;

		if(command)
		{
			remchr(command, '\'');
			strncpy(message,command,MESSLEN);
			message[MESSLEN-1] = '\0';
		}
		else
			message[0] = '\0';
			
		deselectObjects(0);
		if (wp)
			objselect(wp,fobj);
		flushredraw();

		return appl_write(acc_id, 16, &messbuff);
	}
	else
	{
		venusErr(NlsStr(T_NOACC), name);
		return FALSE;
	}
}

void HandlePendingMessages (void)
{
	MEVENT E;
	word messbuff[8];
	word events;
	
	E.e_flags = (MU_MESAG|MU_TIMER);
	E.e_time = 0L;
	E.e_mepbuf = messbuff;
		
	for (;;)
	{
		WindUpdate (END_UPDATE);
		events = evnt_event(&E);
		WindUpdate (BEG_UPDATE);
		
		if (events & MU_MESAG)
		{
			HandleMessage (messbuff, E.e_kr);
			E.e_time = 200L;
		}
		else
			break;
		
	}
}