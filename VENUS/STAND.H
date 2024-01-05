/*
 * stand.h  -  standalone / merged version definitions
 *
 * last change: 07.05.1990
 */

#ifndef __stand__
#define __stand__

#define MERGED		1
#define STANDALONE	(!MERGED)

#include "..\mupfel\comm.h"

#if MERGED

static void m_mupfel(void);

extern struct CommInfo CommInfo;

#define PGMNAME		"GEMINI"

#else
#define PGMNAME		"VENUS"
#endif

#define TMPINFONAME		PGMNAME ".TMP"
#define FINFONAME		PGMNAME ".INF"
#define MAINRSC		PGMNAME ".RSC"
#define ICONRSC		PGMNAME "IC.RSC"	
#define AUTOEXEC		PGMNAME ".MUP"
#define DEFAULTDEVICE	PGMNAME "-Drive"

#define RUNNER			"RUNNER"

#define MSGFILE			PGMNAME ".MSG"

#endif