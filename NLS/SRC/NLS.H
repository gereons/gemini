/*
 * @(#)nls/nlsutil.h
 * @(#)Stefan Eissing, 19. Januar 1991
*/

#ifndef __nls_h__
#define __nls_h__

/*
 * TextFileName:  	Name der Datei, in der die Textdaten stehen. 
 *					Dieser sollte mit Fopen zu îffnen sein. Im Falle
 *                  von GEM-Programmen, also ermittelten Pfad mit
 *                  Åbergeben!
 * MallocFuntcion:	Funktion, die zum allozieren von Speicherplatz 
 *					benutzt wird
 * FreeFunction:	Funktion, die zum freigeben von Speicherplatz
 *					benutzt wird
 *
 * Ergebnis:		== 0 Laden war nicht erfolgreich
 *					!= 0 erfolgreich initialisiert
 *
 * Beschreibung:	Muû vor der Benutzung einer anderen Funktion
 *					aufgerufen werden.
 */
int NlsInit(const char *TextFileName, 
			void *MallocFunction,
			void *FreeFunction);

/*
 * Ergebnis:		keins
 *
 * Beschreibung:	Muû am Ende des Programmes, falls NlsInit()
 *					erfolgreich war, aufgerufen werden
 */
void NlsExit(void);

/*
 * Section:			String, der die Sektion (Gruppe) von Texten
 *					eindeutig beschreibt (z.B. Dateiname von Projekt
 *                  und Sourcefile). KEINE Steuerzeichen erlaubt!
 * Number:			Nummer des Strings in dieser Sektion
 *
 * Ergebnis:		Zeiger auf den Åber Section/Number spezifizierten
 *                  String oder NULL, falls dieser nicht gefunden
 *                  wurde
 *
 * Beschreibung:	Liefert einen Zeiger auf den mit Section/Number
 *                  spezifizierten String. Dazu muû zuvor mit
 *					LangInit die Text-Datei geladen werden.
 */
const char *NlsGetStr(const char *Section, int Number);

/*
 * Falls in einem Sourcefile mit
 * #define NlsLocalSection "GeminiIcondrag"
 * NlsLocalSection definiert wurde, kann statt LangGetStr auch
 * folgendes Makro benutzt werden.
 */
#define NlsStr(a)	NlsGetStr(NlsLocalSection, (a))


#endif