/*
 * comm.h
 *
 * Kommunikation zwischen Mupfel und Venus
 *
 * gs & se
 *
 * last change: 09.02.91
 *
 * ATTENTION: CHANGING THIS FILE REQUIRES RECOMPLING MUPFEL AND VENUS!
 */

#ifndef _M_COMM
#define _M_COMM

#include <stddef.h>

typedef enum
{
	execPrg,		/* um von Venus/Mupfel ein Programm auszufÅhren */
	neverMind,	/* neverMind, um normal fortzufahren */
	overlay,		/* es soll ein Overlay ausgefÅrt werden */
	windOpen,		/* console fenster geîffnet */
	windClose,	/* console fenster geschlossen */
	feedKey,		/* eine Taste gedrÅckt */
} CommType;

typedef enum
{
	CP, MV, RM, LABEL, INIT, FORMAT
} MupfelFunction;

struct CommInfo
{
	int isGemini;		/* TRUE for Gemini */
	char *PgmName;		/* "Gemini" or "Mupfel" */
	char *PgmPath;		/* complete Path to Gemini/Mupfel */
	size_t dirty;
	CommType cmd;
	union
	{
		const char *cmdspec;		/* fÅr execPrg	*/
		long key;					/* fÅr feedKey */
	} cmdArgs;
	union
	{
		void (*fmt_progress)(int permille);
	} fun;
	char *errmsg;
	char **fkeys;
	void (*getMupfelWD)(char *,size_t);
};

extern struct CommInfo CommInfo;

int MupfelCommand(MupfelFunction f, int argc, char **argv);

#endif