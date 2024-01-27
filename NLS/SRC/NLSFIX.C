/*
 * @(#)Language/Nlsfix.c
 * @(#)Stefan Eissing, 14. Januar 1991
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "nlsdef.h"
#include "nlsfix.h"

#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

static int fixSection(BINSECTION *sec, char *start)
{
	if (sec->SectionTitel)
		sec->SectionTitel = start + (long)sec->SectionTitel;
	if (sec->StringCount)
		sec->SectionStrings = start + (long)sec->SectionStrings;
	if (sec->NextSection)
		sec->NextSection = (BINSECTION *)
								(start + (long)sec->NextSection);
	
	if ((sec->SectionTitel <= start)
		|| ((char *)sec->NextSection <= start)
		|| (sec->StringCount && (sec->SectionStrings <= start)))
	{
		return FALSE;
	}
	return TRUE;
}

BINSECTION *NlsFix(char *TextFile)
{
	size_t len;
	BINSECTION *firstSection, *Section;
	
	len = strlen(NLS_BIN_MAGIC);
	if (len & 1L)
		++len;
		
	if (strncmp(TextFile, NLS_BIN_MAGIC, len))
		return NULL;

	firstSection = (BINSECTION *)(TextFile + len);
	
	Section = firstSection;
	while (Section && fixSection(Section, TextFile))
	{
		Section = Section->NextSection;
	}
	
	return firstSection;
}