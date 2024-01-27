/*
 * @(#)nls/nlsfix.h
 * @(#)Stefan Eissing, 01. Januar 1991
 */

#ifndef __nlsfix__
#define __nlsfix__

#include "nlsdef.h"

/*
 * TextFile:		Pointer auf die in den Speicher geladene
 *					NLS-Datei
 *
 * Ergebnis:		Zeiger auf die erste Section oder NULL, wenn
 *                  ein Fehler aufgetreten ist.
 *
 * Beschreibung:    Setzt die Zeiger in der geladenen NLS-Datei
 */
BINSECTION *NlsFix(char *TextFile);

#endif