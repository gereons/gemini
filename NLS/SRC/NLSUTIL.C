/*
 * @(#)Language/Nlsutil.c
 * @(#)Stefan Eissing, 29. Dezember 1990
*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "nlsdef.h"
#include "nls.h"
#include "nlsfix.h"

#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

/*
 * Interne, lokale Variablen
 */
/* LangInit schon erfolgreich aufgerufen? */
static int Initialisiert = FALSE;

/* Pointer fÅr malloc/free-Funktionen */
static void *(*NlsMalloc)(long);
static void (*NlsFree)(void *ptr);

/* Pointer auf den Inhalt der geladenen Datei */
static char *TextBuffer;

/* Zeiger auf die erste Section */
static BINSECTION *FirstSection;

int NlsInit(const char *TextFileName, 
			void *MallocFunction,
			void *FreeFunction)
{
	int fhandle;
	long fsize;
	
	if (Initialisiert)
		return 0;
	
	NlsMalloc = (void *(*)(long))MallocFunction;
	NlsFree =  (void(*)(void *))FreeFunction;

	fhandle = Fopen(TextFileName, 0);
	if (fhandle < 0)
	{
		return 0;
	}
	
	fsize = Fseek(0L, fhandle, 2);
	Fseek(0L, fhandle, 0);
	
	TextBuffer = NlsMalloc(fsize);
	if (!TextBuffer)
	{
		Fclose(fhandle);
		return 0;
	}
	
	if (Fread(fhandle, fsize, TextBuffer) != fsize)
	{
		NlsFree(TextBuffer);
		Fclose(fhandle);
		return 0;
	}
	Fclose(fhandle);
	
	FirstSection = NlsFix(TextBuffer);
	if (FirstSection == NULL)
	{
		NlsFree(TextBuffer);
		return 0;
	}
	
	Initialisiert = TRUE;
	return 1;
}

void NlsExit(void)
{
	if (!Initialisiert)
		return;
	
	NlsFree(TextBuffer);
	Initialisiert = FALSE;
}

const char *NlsGetStr(const char *Section, int Number)
{
	BINSECTION *sec, *nextsec;
	int found;
	
	if (!Initialisiert)
		return NULL;
		
	nextsec = FirstSection;
	do
	{
		sec = nextsec;
		found = !strcmp(Section, sec->SectionTitel);
		nextsec = sec->NextSection;
	}
	while (!found && nextsec);
	
	if (found)
	{
		if (sec->StringCount > Number)
		{
			char *str = sec->SectionStrings;
			
			/* öberspringe Number Strings */
			for (; Number; --Number)
			{
				while (*str)
					++str;
				++str;
			}
			return str;
		}
	}
	return "String missing!";
}
