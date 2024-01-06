/*
	@(#)ListMan/listman.h
	@(#)Julian F. Reschke, 24. MÑrz 1991
*/


#ifndef _LIST
#define _LIST

#include <aes.h>

#include <flydial\slid.h>

typedef struct listspec
{
	struct listspec *next;
	struct
	{
		unsigned selected : 1;
		unsigned reserved1 : 1;
		unsigned reserved2 : 1;
		unsigned reserved3 : 1;
		unsigned reserved4 : 1;
		unsigned reserved5 : 1;
		unsigned reserved6 : 1;
		unsigned reserved7 : 1;
		unsigned reserved8 : 1;
		unsigned reserved9 : 1;
		unsigned reserved10 : 1;
		unsigned reserved11: 1;
		unsigned reserved12: 1;
		unsigned reserved13: 1;
		unsigned reserved14: 1;
		unsigned reserved15: 1;
	} flags;
	void *entry;
} LISTSPEC;

#define LISTDRAWREDRAW 0x01

#define LISTSPECCACHE 16

typedef struct
{
	long index;
	LISTSPEC *l;
} SPECCACHE;


typedef struct
{
	/* Tree und index des Objektes, in dem alle EintrÑge liegen */
	OBJECT *boxtree;
	int boxindex;
	int boxbgindex;	/* Hintergrundbox der Liste (wg. Slider) */
	
	/* Handle des Fensters, in dem die Objekte dargestellt werden.
	   Dient zum Redraw. Wird kein Fenster verwendet, muû das 
	   Handle < 0 sein. */
	int		windhandle;

	/* Rechteck der Box, in der alle EintrÑge liegen, x und y sind
	   relativ zum Work-Bereich des Fensters; muû nur gesetzt werden,
	   wenn box == NULL */
	GRECT	area;
	
	/* AbstÑnde der einzelnen EintrÑge */
	int		hoffset,voffset;
	/* Grîûe der einzelnen EintrÑge */
	int		hsize,vsize;
	
	/* TRUE, wenn Slider entlang der Liste vertikal sein soll */
/*	int vertigo;
*/	
	/* Koordinaten der Sliderbox entlang der Liste. Sind davon
	   welche 0, so wird L->area fÅr Defaultwerte benutzt. */
	int lslidx;
	int lslidy;
	int lslidlen;
	
	/* maximale Breite eines Eintrags. Ist dieser Wert 0, so
	   wird kein Breitenslider benutzt */
	int		maxwidth;

	/* Schrittweite beim horizontalen Scrollen */
	int		hstep;

	/* horizontale Position, falls hor. Slider */
	int		hpos;

	
	/* Koordinaten der Sliderbox fÅr die "Breite" */
	int mslidx;
	int mslidy;
	int mslidlen;
	
	/* erster anzuzeigender Eintrag */
	long startindex;
	
	/* Kopf der Liste von EintrÑgen */
	LISTSPEC *list;
	
	/* Funktion zum Zeichnen eines Eintrags <entry> an den Koord.
	   <x> und <y> mit <offset> vom "linken" Rand beginnend.
	   <cliprect> muû berÅcksichtigt werden! */
	void 	(*drawfunc)(LISTSPEC *l, int x, int y, int offset, 
				GRECT *cliprect, int how);
				
	/* Art der erlaubten Selektionen:
		0: gar nicht
		1: nur einzelne Items
		2: beliebig
	*/
	int selectionservice;

	/* privater Teil */
	
	/* LÑnge der Liste == Anzahl aller EintrÑge */
	long	listlength;
	
	/* Anzahl der darstellbaren EintrÑge */
	int 	lines;
	
	/* Pointer auf die Slider */
	SLIDERSPEC *lslid;
	SLIDERSPEC *mslid;
} LISTINFO;

int ListInit (LISTINFO *L);
void ListDraw (LISTINFO *L);
long ListClick (LISTINFO *L, int clicks);
void ListExit(LISTINFO *L);
void ListVScroll(LISTINFO *L, long Lines);
void ListWindDraw (LISTINFO *L, GRECT *G, int fullredraw);
LISTSPEC *ListIndex2List (LISTSPEC *list, long index);

void ListStdInit (LISTINFO *L, OBJECT *tree, int box1, int box2, 
	void (*drawfunc)(LISTSPEC *l, int x, int y, int offset,
	GRECT *cliprect, int how), LISTSPEC *list, int maxwidth, long startindex,
	int selectionservice);
void ListScroll2Selection (LISTINFO *L);
void ListUpdateEntry (LISTINFO *L, long entry);
void ListInvertEntry (LISTINFO *L, long entry);

void ListPgDown (LISTINFO *L);
void ListPgUp (LISTINFO *L);
void ListLnRight (LISTINFO *L);
void ListPgRight (LISTINFO *L);
void ListLnLeft (LISTINFO *L);
void ListPgLeft (LISTINFO *L);
void ListVSlide (LISTINFO *L, int pos);
void ListHSlide (LISTINFO *L, int pos);

/* mit dieser Routine kann man ListMan sagen, daû die Liste nun an
   einer neuen Position steht. Der Returnwert ist FALSE, wenn
   ListMan keine VerÑnderung feststellen konnte; sonst TRUE */

int ListMoved (LISTINFO *L);

#endif