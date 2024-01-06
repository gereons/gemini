/*
	@(#)ListMan/slid.h
	@(#)Julian F. Reschke, 27. September 1990
*/

#ifndef _SLID
#define _SLID

typedef struct
{
	int hori;
	int accel;
	long scale;
	int size;
	long position;
	int lastdraw;
	int offset;
	OBJECT ob[5];
	int drawn;
} SLIDERSPEC;

/* malt den Slider <s> im Rechteck xc, yc,... */
void SlidDraw (SLIDERSPEC *s, int xc, int yc, int wc, int hc, 
		int redraw);

void SlidDrCompleted (SLIDERSPEC *s);

/* Setzt die LÑnge der Box in Slider <s> auf <size> */
void SlidSlidSize (SLIDERSPEC *s, int size);

/* Setzt die Skalierung des Sliders <s> auf <scale> */
void SlidScale (SLIDERSPEC *s, long scale);

/* Setzt die Position des Sliders <s> auf <pos> */
void SlidPos (SLIDERSPEC *s, long pos);

/* Legt einen neuen Slider an und liefert einen Pointer auf
   diesen zurÅck. <hori> gibt an, ob es sich um einen horizontalen
   oder vertikalen Slider handelt. <Slider> kann auch NULL sein.
   accel gibt den Faktor fÅrs zeilenweise scrollen an (also
   normalerweise 1) */
SLIDERSPEC *SlidCreate (SLIDERSPEC *Slider, int hori, int accel);

/* Gibt den Speicherplatz fÅr Slider <s> wieder frei. <s> darf
   danach nicht mehr benutzt werden. */
void SlidDelete (SLIDERSPEC *s);

/* Setzt die Koordinaten des Sliders <s>. Sollte direkt nach
   SlidCreate aufgerufen werden. */
void SlidExtents (SLIDERSPEC *s, int x, int y, int len);


/* In: Mauskoordinate, Out: neue Position. Bei einem Mausclick
   auûerhalb des Sliders wird -1 zurÅckgegeben. */
long SlidClick (SLIDERSPEC *s, int x, int y, int clicks, int realtime);

long SlidAdjustSlider (SLIDERSPEC *s, int x, int y);

/* Deselektiert die Pfeile, falls notwendig */
void SlidDeselect (SLIDERSPEC *s);

#endif
