/*
 * message.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: modul for handling messages
 *
 * last change: 15.02.1991
 */

#ifndef __message__
#define __message__

void MessInit(void);	/* sollten vor bzw. nach appl_init, appl_exit
						 * aufgerufen werden */
void MessExit(void);

word HandleMessage(word messbuff[8], word kstate);

word StartAcc(WindInfo *wp,word fobj,char *name,char *command);

word PasteAccWindow(WindInfo *fromwp, word tohandle, word mx, word my);

word WriteAccInfos(word fhandle, char *buffer);
void ExecAccInfo(const char *line);

void HandlePendingMessages (void);

#endif