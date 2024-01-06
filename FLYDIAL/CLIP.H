/*
	@(#)FlyDial/clip.h
	@(#)Julian F. Reschke, 11. Oktober 1990
*/

#include <string.h>

/*
	FindClipFile

	R�ckgabewert:	== 0: Fehler
					!= 0: Erfolg
	Extension:		Namenserweiterung der gesuchten
					Datei
	Filename:		Zeiger auf Char-Array, in das der
                    vollst�ndige Name eingetragen wird
*/

int ClipFindFile (const char *Extension, char *Filename);

/*
	ClearClip

	L�scht alle SCRAP-Dateien
*/

int ClipClear (char *not);
