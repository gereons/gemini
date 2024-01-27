/*
 * @(#)nls/nlsdef.h
 * @(#)Stefan Eissing, 01. Januar 1991
 */

#ifndef __nlsdef__
#define __nlsdef__

#define NLS_SECTION_LEN		16

#define NLS_BIN_MAGIC		"Native Language Support (bin)"
#define NLS_TEXT_MAGIC		"Native Language Support (ascii)"

#define SECTION_MAGIC		"NlsLocalSection"
#define ENUM_MAGIC			"NlsLocalText"

/* Eine NLS-Bin-Datei ist wie folgt aufgebaut:
 * NLS_BIN_MAGIC
 * direkt gefolgt von der ersten BINSECTION
 * Alle weiter Sections k”nnen aus dieser ermittelt werden
 */
typedef struct sec
{
	char *SectionTitel;		/* Offset des Section-Strings im File */
	char *SectionStrings;	/* Offset des ersten Strings dieser
							 * Section im File die weiteren Strings 
							 * liegen direkt dahinter */
	long StringCount;		/* Anzahl der Strings */
	struct sec *NextSection;/* Offset der n„chsten Section im File
							 * 0L ist gleich Ende */	
} BINSECTION;

#endif