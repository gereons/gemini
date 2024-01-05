/*
 * @(#) Gemini\appledit.h
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: Header File for appledit.c
 */

#ifndef __appledit__
#define __appledit__

struct applInfo
{
	struct applInfo *nextappl;	/* pointer zur nÑchsten Struktur */
	char path[MAXLEN];			/* Pfad der Applikation */
	char name[MAX_FILENAME_LEN];	/* Name der Applikation */
	char label[MAX_FILENAME_LEN];	/* Label des Laufwerks */
	char wildcard[WILDLEN];		/* zustÑndig fÅr z.B. *.c */
	word startmode;				/* Modus fÅr Programmstart */
};
typedef struct applInfo ApplInfo;

extern ApplInfo *applList;


ApplInfo *getApplInfo(ApplInfo *List, const char *name);

word insertApplInfo(ApplInfo **List, ApplInfo *prev, ApplInfo *ai);
word deleteApplInfo(ApplInfo **List, const char *name);
void FreeApplList(ApplInfo **list);

void EditApplList(const char *applname, const char *path,
					const char *label, word defstartmode);

#endif