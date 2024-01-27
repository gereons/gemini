/*
 * @(#)Language/nlstest.c
 * @(#)Stefan Eissing, 29. Dezember 1990
*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "nls.h"

#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

#define NlsLocalSection "Gemini.Dialog"
enum NlsLocalText{

T_SHAREWARE1,	/*%s ist ein Shareware-Programm. Shareware 
bedeutet, da� Sie dieses Programm f�r nicht-kommerzielle 
Zwecke frei kopieren und testen d�rfen. Wenn Sie allerdings 
GEMINI/MUPFEL/VENUS regelm��ig benutzen, so haben Sie sich an das Konzept von 
Shareware zu halten, indem Sie den Autoren einen Obolus von 
50,- DM entrichten. Dies ist KEIN Public-Domain- oder Freeware-
Programm!*/

T_SHAREWARE2, /*Wenn Sie keine Raubkopie benutzen wollen oder 
an der Weiterentwicklung diese Programms interessiert sind, 
k�nnen Sie das Geld auf eins unserer Konten 
�berweisen:|
|Gereon Steffens,
|%sKto. xxxxxxxxxx, xxxxxxxxxxxxxxxxxxxxxxxx
|Stefan Eissing,
|%sKto. xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx|
|Danke.*/

T_FOLDER,	/*Der Pfad dieses Fensters ist zu lang, um neue 
Ordner anzulegen!*/

T_NONAME,	/*Sie sollten einen Namen angeben!*/

T_EXISTS,	/*Ein Objekt dieses Namens existiert schon!*/

T_INVALID,	/*Dies ist kein g�ltiger Name f�r eine Datei 
oder einen Ordner! Bitte w�hlen Sie einen anderen Namen.*/

T_NOSPACE,	/*Die Aufl�sung ist zu gering, um diesen Dialog 
darzustellen!*/

T_ESCTEST,  /* Dies ist Kommentaranfang \[ und -ende \]! und 
ein normaler Backslash-escape mit einem Kommentar \\\[ dran 
mit einer Hexzahl \x40 und einer Oktalen \7*/
};

int main(void)
{
	if (NlsInit("NLSTEST.NLS", Malloc, Mfree))
	{
		puts("NlsInit erfolgreich!");
		puts(NlsStr(T_SHAREWARE1));
		puts(NlsStr(T_SHAREWARE2));
		puts(NlsStr(T_ESCTEST));
		NlsExit();
		return 0;
	}
	return 1;
}
