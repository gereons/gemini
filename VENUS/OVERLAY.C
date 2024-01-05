/*
 * @(#) Gemini\overlay.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: functions to start overlays
 */

#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <nls\nls.h>

#include "vs.h"
#include "overlay.h"
#include "infofile.h"
#include "stand.h"
#include "fileutil.h"
#include "venusErr.h"
#include "util.h"

/* externals
 */
extern DeskInfo NewDesk;
extern char bootpath[];

/* internal texts
 */
#define NlsLocalSection		"Gmni.overlay"
enum NlsLocalText{
T_RUNNER,	/*Ein Start von %s ist nicht m”glich, da %s
 nicht gefunden werden konnte!*/
T_SHWRITE,	/*Das AES weigert sich, %s zu starten!*/
};


void setOverlay(char *fname, char *command, word gemstart)
{
	char runner[MAXLEN];
	COMMAND ovlCommand;
	
	strcpy(runner, bootpath);
	stripFileName(runner);
	addFileName(runner, RUNNER ".APP");

	if (!fileExists(runner))
	{
		venusErr(NlsStr (T_RUNNER), fname, runner);
	}
	else
	{
#if MERGED
		strcpy(ovlCommand.command_tail, "G ");
#else
		strcpy(ovlCommand.command_tail, "V ");
#endif
		strcat(ovlCommand.command_tail, gemstart? "G " : "T ");

		strncat(ovlCommand.command_tail, fname, 122);
		if (command)
		{
			remchr(command, '\'');
			
			strncat(ovlCommand.command_tail, " ",
				 125 - strlen(ovlCommand.command_tail));
			strncat(ovlCommand.command_tail,
				command, 125 - strlen(ovlCommand.command_tail));
		}
		
		ovlCommand.length = strlen(ovlCommand.command_tail);
 		if (!shel_write(1,1,1,runner,(char *)&ovlCommand))
 		{
 			venusErr(NlsStr (T_SHWRITE), fname);
 		}
 		else
 			CommInfo.cmd = overlay;
 	}
	
}

word doOverlay(const char *tmpinfo)
{
	if (CommInfo.cmd != overlay)
		return FALSE;
	
	if (!NewDesk.saveState)
	{
		makeConfInfo();
		writeInfoDatei(tmpinfo, FALSE);
	}

	return TRUE;
}
