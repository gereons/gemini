/*
 * @(#) Gemini\draglogi.h
 * @(#) Stefan Eissing, 11. April 1991
 *
 * description: Header File for draglogi.c
 *
 */

#ifndef __draglogi__
#define __draglogi__

#include "stand.h"

void doDragLogic(WindInfo *fwp,WindInfo *twp,word toobj,word kstate,
					word fromx,word fromy,word tox,word toy);

int PerformCopy (WindInfo *from, char *from_path, char *to_path, 
		int move);
		
#if MERGED
void PasteString(const char *line);
#endif

#endif