/*
	@(#)MausTausch/fontsel.h
	@(#)Julian F. Reschke, 10. August 1990
*/


#ifndef __fontsel__
#define __fontsel__

#include <stdlib.h>
#include <vdi.h>

#include <flydial\listman.h>

typedef struct
{
	LISTINFO L;			/* Listinfo f�r die Liste der Fontnamen */
	LISTINFO P;			/*    "              "       Fontgr��en */
	OBJECT *tree;		/* Dialogbox							*/
	int fontbox;		/* Index der Box f�r Namen				*/
	int fontbgbox;		/* Index der Hintergrundbox f�r Namen	*/
	int pointbox;		/*               f�r Gr��en				*/
	int pointbgbox;		/*               f�r Gr��en				*/
	int showbox;		/* Index der Box f�r Teststring			*/
	FONTWORK *fw;		/* Pointer auf FONTWORK-Struktur 		*/
	char *test;			/* Teststring f�r Ausgabe				*/
} FONTSELINFO;


int FontSelInit (FONTSELINFO *F, FONTWORK *fw, OBJECT *tree,
				int fontbox, int fontbgbox, int pointbox,
				int pointbgbox, int showbox, char *teststring,
				int proportional, int *actfont, int *actsize);

void FontSelDraw (FONTSELINFO *F, int font, int size);

void FontShowFont (FONTWORK *F, OBJECT *tree, int frame, int font,
	int size, char *teststring);

int FontClFont (FONTSELINFO *F, int clicks, int *font, int *size);
int FontClSize (FONTSELINFO *F, int clicks, int *font, int *size);

void FontSelExit (FONTSELINFO *F);
void FontSelectSize (LISTINFO *P, int *size, int redraw);

#endif