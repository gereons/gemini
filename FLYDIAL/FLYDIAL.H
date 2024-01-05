/*
	@(#)FlyDial/flydial.h
	@(#)Julian F. Reschke, 30. MÑrz 1991

	bitte aufmerksam den GEN-File durchlesen!
*/

#ifndef __FLYDIAL__
#define __FLYDIAL__

#if MSDOS
#	include "vdi.h"
#	include "aes.h"
#else
#	include <vdi.h>
#	include <aes.h>
#	include <tos.h>
#endif
#include <stddef.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define ALCENTER "\001"
#define ALRIGHT "\002"

extern int DialWk;	/* wird von allen meinen Routinen benutzt */

typedef void *(*DIALMALLOC) (size_t);
typedef void (*DIALFREE) (void *);

extern DIALMALLOC dialmalloc;
extern DIALFREE dialfree;

/*
	FlyDial Raster Manager
*/

typedef struct RDB
	{
	int		saved;

	char	filename[14];
	int		handle;
	int		fullSlices;
	int		lastSliceHeight;
	int		sliceHeight;
	int		scanlineSize;
	int		planes;
	}
RDB;

/* DOS-only: Initialize the raster manager and set the tempdir.*/
int RastInit(char *tempdir);

/* DOS-only: Terminate the raster manager */
void RastTerm(void);

/* DOS-only: write a part of the screen to disk */
int RastDiskSave(RDB *rdb, int x, int y, int w, int h);

/* DOS-only: retrieve a part of the screen from disk */
int RastDiskRestore(RDB *rdb, int x, int y, int w, int h);

/* Grî·e eines Bildschirmausschnitts in Bytes */
unsigned long RastSize (int w, int h, MFDB *TheBuf);

/* Bildschirmausschnitt in Buffer speichern */
void RastSave (int x, int y, int w, int h, int dx, int dy,
	MFDB *TheBuf);

/* Bildschirmausschnitt aus Buffer zurÅckholen */
void RastRestore (int x, int y, int w, int h, int sx, int sy, MFDB *TheBuf);

void RastBufCopy (int sx, int sy, int w, int h, int dx, int dy, MFDB *TheBuf );

/* Setze udsty auf eine so gepunktete Linie, daû bei einem grauen
   Desktophintergrund eine schwarze Linie erscheint (xy[]: Anfangs-
   und Endpunkt der Linie */
void RastSetDotStyle (int ha, int *xy);

/* Malt gepunktetes Rechteck */
void RastDotRect (int ha, int x, int y, int w, int h);

void RastDrawRect (int ha, int x, int y, int w, int h);
void RastTrans (void *saddr, int swb, int h, int handle);
void RastGetQSB(void **qsbAddr, int *qsbSize);


/*
	ersetzt form_alert (locker)

	Image: 		Pointer auf Bitimage
	String:		Text, UNFORMATIERT. Senkrechter Strich fÅr 'harten'
				Umbruch erlaubt.
	Default:	Nummer des Default-Buttons (zero-based)
	Buttons:	Button-Namen, getrennt durch '|', Shortcuts durch
				vorangestelltes '[' gekennzeichnet.
*/

int DialAlert (BITBLK *Image, const char *String, int Default,
	const char *Buttons);



/*
	Unterschied zu DialAlert: Icon ist animiert

	Image:		Zeiger auf Liste von BITBLKS, NULL-terminiert
	Durations:	Zeiger auf Liste von Warteintervallen (ms)
*/

int DialAnimAlert (BITBLK **Image, int *Durations, char *String,
	int Default, const char *Buttons);

typedef struct
{
	OBJECT 	*Tree;
	MFDB 	Buffer;
	int 	x, y, w, h;
	int 	offset;
	RDB		rdb;
} DIALINFO;



/*
	Zum Anfang des Dialogs aufrufen

	Return: 0: Hintergrund konnte NICHT gebuffert werden!

	Hinweis: nach DialStart darf man NICHT mehr mit form_- oder
	DialCenter die Position verÑndern!!!!!
*/
int DialStart (OBJECT *TheTree, DIALINFO *D);
int DialExStart (OBJECT *TheTree, DIALINFO *D, int use_qsb);

/*
	Bildschirmplatz wieder freigeben
*/
void DialEnd (DIALINFO *D);


/* Dialogbox Åber den Bildschirm bewegen */

int DialMove (DIALINFO *D, int sx, int sy, int sw, int sh);


/*
	équivalent zu form_do

	aktiviert die Move-Routine bei Anklicken eines Objekt mit
	extended object type 17 und TOUCHEXIT-Status
	liefert in StartOb auch das AKTUELLE Edit-Obj ZURöCK!!!
*/
int DialDo (DIALINFO *D, int *StartOb);

void DialDraw (DIALINFO *D);

void DialCenter (OBJECT *D);

/*
	Initialisiert bzw. deinitialisiert die Dial-Routinen
	malloc() und free() sind jetzt konfigurierbar, zB
	DialInit (malloc, free);
*/
int DialInit (void *, void *);


void DialExit (void);




/*
	Ersetzen jeweils die gleichnamigen
*/

int FormButton (OBJECT *tree, int obj, int clicks, int *nextobj);
int FormKeybd (OBJECT *tree, int edit_obj, int next_obj, int kr,
	int ks, int *onext_obj, int *okr);
int FormXDo (OBJECT *tree, int *startfld);
int FormDo (OBJECT *tree, int startfld);


/*
	Installiert einen neuen Keyboard-Handler in FormDo
*/

typedef int (*FORMKEYFUNC) (OBJECT *, int, int, int, int, int *, int *);
void FormSetFormKeybd (FORMKEYFUNC fun);
FORMKEYFUNC FormGetFormKeybd (void);

typedef int (*VALFUN)(OBJECT *tree, int ob, int *chr, int *shift,
	int idx);
void FormSetValidator (char *valchars, VALFUN *funs);


extern int HandStdWorkIn[];
extern int HandAES, HandXSize, HandYSize, HandBXSize, HandBYSize;
int HandFast (void);
int HandYText (void);
void HandScreenSize (int *x, int *y, int *w, int *h);
void HandInit (void);
void HandClip (int x, int y, int w, int h, int flag);


#define G_ANIMIMAGE 42

typedef struct
{
	BITBLK **Images;	/* Liste der Bitblocks, durch Nullpointer
						abgeschlossen */
	int *Durations;
	int Current;
} ANIMBITBLK;


/*
	Alle Routinen sollten call-kompatibel zu den Vorbildern sein

	Wichtigster Unterschied:

	Extended object type 18: spezielle Buttons

	- durch vorangestelltes '[' wird Shortcut gekennzeichnet.
	  Beispiel: "[Abbruch" ergibt Text "Abbruch" und kann auch
	  mit ALT-A aktiviert werden

	- handelt es sich bei dem Button um einen Exit-Button, dann
	  wird die Hîhe um 2 Pixel vergrîûert, um Platz fÅr die
	  Unterstreichung zu lassen

	- bei anderen Button-Typen wird der Text NEBEN einen kleinen
	  Knopf gesetzt, der -- je nach 'Radio Button' oder nicht --
	  verschieden aussieht


	Extended object type 17: Dialog-Mover

	FÅr ein 'Eselsohr' wie in den Alertboxen bitte folgendes
	Objekt benutzen:

	- I-BOX, Extended type 17, TOUCHEXIT, OUTLINE, CROSSED

	
	Extended object type 19: UNDERLINE

	Das Objekt wird mit einer horizontalen Linie unterstrichen
	(10.6.1989)

	Extended object type 20: TITLELINE

	Speziell fÅr beschriftete Rahmen. Man nehme einen normalen
	Button (mit Outline). Bei TITLELINE-Objekten wird der
	Text dann nicht in der Mitte zentriert, sondern direkt
	(type-over) Åber dem oberen Rand ausgegeben. Im Beispiel-RSC
	ansehen!

	Extended object type 21: HELP-Taste
	
	Objekt kann auch durch DrÅcken der HELP-Taste betÑtigt
	werden. Zum Design: mîglichst wie in SCSI-Tool, also:
	
	Text, Outlined, Shadowed, Font klein, zentriert, 'HELP'

*/ 

#if __MSDOS__
VOID ObjcMyButton(VOID);
VOID ObjcAnimImage(VOID);
#else
int cdecl ObjcMyButton (PARMBLK *p);
int cdecl ObjcAnimImage (PARMBLK *p);
#endif

int ObjcChange (OBJECT *tree, int obj, int resvd, int cx, int cy,
				int cw, int ch, int newstate, int redraw);
void ObjcXywh (OBJECT *tree, int obj, GRECT *p);
void ObjcToggle (OBJECT *tree, int obj);
int ObjcGParent (OBJECT *tree, int obj);
void ObjcDsel (OBJECT *tree, int obj);
void ObjcSel (OBJECT *tree, int obj);
int ObjcDraw (OBJECT *tree, int startob, int depth, int xclip,
	int yclip, int wclip, int hclip);
int ObjcTreeInit (OBJECT *tree);
int ObjcRemoveTree (OBJECT *tree);
int ObjcOffset (OBJECT *tree, int oby, int *x, int *y);
OBSPEC *ObjcGetObspec (OBJECT *tree, int index);

/*
	Dier vertikalen Koordinaten der Objekte im Baum werden mit
	a/b multipliziert. 1.5-zeiliger Zeilenabstand also mittels
	a=3, b=2. Ansehen!
*/
void ObjcVStretch (OBJECT *tree, int ob, int a, int b);



#define FLYDIALMAGIC	'FLYD'

typedef struct
{
	long	me_magic;		/* == FLYDIALMAGIC */
	OBJECT	*me_sub;
	int		me_obnum;
	char	*me_title;
	USERBLK	me_ublk;
} MENUSPEC;

void PoppInit (void);		/* tut das Offensichtliche */
void PoppExit (void);		/* ebenso */
void PoppResult (OBJECT **Tree, int *obj);
void PoppChain (OBJECT *ParentTree, int ParentOb,
				OBJECT *SubTree, int ChildOb, MENUSPEC *TMe);
				/* entweder muû ein Zeiger (auf eine zu fuellende
				MENUSPEC-Struktur Åbergeben werden, oder NULL
				(dann wird der benoetigte Speicher gemalloct */
void PoppUp (OBJECT *Tree, int x, int y, OBJECT **ResTree, int *resob);


/* ersetzt gestrichelte Linie in MenÅs durch durchgezogene */
void MenuSet2ThinLine (OBJECT *tree, int ob);

/* macht die MenÅs im MenÅbaum ein Zeichen schmaler (wg. Kuma-Resource)
   (wenn tune != FALSE.) und Ñndert die gestrichelten Linien mit 
   MenuSet2ThinLine; patcht Breite der MenÅleiste */
void MenuTune (OBJECT *tree, int tune);

/* Grîûe eines Bildschirmausschnitts in Bytes */
unsigned long RastSize (int w, int h, MFDB *TheBuf);

/* Bildschirmausschnitt in Buffer speichern */
void RastSave (int x, int y, int w, int h, int dx, int dy,
	MFDB *TheBuf);

/* Bildschirmausschnitt aus Buffer zurÅckholen */
void RastRestore (int x, int y, int w, int h, int sx, int sy, MFDB *TheBuf);

void RastBufCopy (int sx, int sy, int w, int h, int dx, int dy, MFDB *TheBuf );

/* Setze udsty auf eine so gepunktete Linie, daû bei einem grauen
   Desktophintergrund eine schwarze Linie erscheint (xy[]: Anfangs-
   und Endpunkt der Linie */
void RastSetDotStyle (int ha, int *xy);

/* Malt gepunktetes Rechteck */
void RastDotRect (int ha, int x, int y, int w, int h);

void RastDrawRect (int ha, int x, int y, int w, int h);
void RastTrans (void *saddr, int swb, int h, int handle);


void RectAES2VDI (int x, int y, int w, int h, int *xy);
void RectGRECT2VDI (GRECT *g, int *xy);

int RectInter (int x1,int y1,int w1,int h1,int x2,int y2,int w2,int h2,
		  int *x3,int *y3,int *w3,int *h3);

int RectOnScreen (int x, int y, int w, int h);
int RectInside (int x, int y, int w, int h, int x2, int y2);
int RectGInter (GRECT *a, GRECT *b, GRECT *c);
void RectClipWithScreen (GRECT *g);

extern BITBLK *ImQuestionMark (void), *ImHand (void), *ImInfo (void);
extern BITBLK *ImFinger (void), *ImBomb (void);
extern BITBLK *ImPrinter (void), *ImDisk (void), *ImDrive (void);
extern BITBLK *ImExclamation (void), *ImSignQuestion (void);
extern BITBLK *ImSignStop (void), *ImSqExclamation (void);
extern BITBLK *ImSqQuestionMark (void);


void WindUpdate (int mode);
void WindRestoreControl (void);

void GrafMouse (int num, MFORM *form);
void GrafGetForm (int *num, MFORM *form);


/*
	rel: 1: relativ zur Mausposition
	cob: Objekt, das unter der Maus erscheinen soll (oder -1)
	mustbuffer: 1: bei Speichermangel abbrechen

    wenn mustbuffer gesetzt ist und der Speicher nicht reicht,
    erhÑlt man in resob -2 zurÅck

*/
void JazzUp (OBJECT *Tree, int x, int y, int rel, int cob,
	int mustbuffer, OBJECT **ResTree, int *resob);

/*
Die Idee ist folgende: man hat einen Button, den Cycle-Button und das
Poppup-MenÅ. Das MenÅ enthÑlt mehrere EintrÑge vom gleichen Typ wie
der Button. Der ob_spec im Button -- also die aktuelle Einstellung --
ist IDENTISCH mit einem der ob_specs im MenÅ. Beim Aufruf gibt man
einen Zeiger auf den ob_spec des Buttons mit. JazzSelect sucht den
Eintrag im MenÅ, setzt einen Haken davor (also zwei Zeichen Leerraum
davor) und zentriert das MenÅ entsprechend. NACH JazzSelect zeigt der
Zeiger auf den ob_spec des zuletzt im MenÅ selektierten Objekts. Dieser
ob_spec wird nun in den Button eingetragen un der Button wieder
neu gemalt. docycle gibt an, ob ein PoppUp erscheinen soll (0) oder
nur `weitergeblÑttert' werden soll (-2). Dieser Modus wird auch
benutzt, wenn das Poppup-MenÅ nicht aufgerufen werden konnte (Speicher-
platz).
*/

/*
	Box, ob: Tree und Objekt des betr. Knopfes
	Poppup: Tree mit dem Popup-MenÅ
	docheck: 1: aktuellen Wert mit Haken versehen
	docycle: 0: Popup, -1: einen Wert zurÅck, 1: einen Wert vor
					-2: cyclen
	obs: neuer obspec
	returns: sel. Objekt bzw. -1
*/

int JazzSelect (OBJECT *Box, int ob, OBJECT *Poppup, int docheck,
	int docycle, long *obs);


typedef struct
{
	int	id;
	struct {
	unsigned isprop : 1;
	unsigned isfsm : 1;
	} flags;
	char name[33];
} FONTINFO;

typedef struct
{
	int handle;		/* vdi-handle auf dem gearbeitet wird */
	int loaded;		/* Fonts wurden geladen */
	int sysfonts;	/* Anzahl der Systemfonts */
	int addfonts;	/* Anzahl der Fonts, die geladen wurden */
	FONTINFO *list;	/* Liste von FONTINFO-Strukturen */
} FONTWORK;

/* Bei FontLoad muû das handle gesetzt werden und beim ersten
 * Aufruf muû loaded FALSE(0) sein. Weiterhin muû in sysfonts die
 * Anzahl der Fonts stehen, die man beim ôffnen der Workstation
 * in work_out bekommen hat.
 */
void FontLoad (FONTWORK *fwork);

/*
 * Baut die Liste list in fwork auf, falls noch nicht besetzt.
 * Bei RÅckgabe von FALSE hat das Ganze nicht geklappt.
 */ 
int  FontGetList (FONTWORK *fwork, int test_prop, int test_fsm);
/* Liste ist readonly */

/* Deinstalliert die Fonts auf der Workstation handle und gibt,
 * falls list != NULL ist die Liste wieder frei
 */
void FontUnLoad (FONTWORK *fwork);

/* Fontgrîûe unter BerÅcksichtigung von FSMGDOS setzen */
int FontSetPoint (FONTWORK *F, int handle, int id, int point,
	int *cw, int *ch, int *lw, int *lh);

/* Feststellen, ob FSM-Font */
int FontIsFSM (FONTWORK *F, int id);

int vst_arbpt (int handle, int point, int *char_width, 
	int *char_height, int *cell_width, int *cell_height);
void vqt_devinfo (int handle, int devnum, int *devexists, char *devstr);



#endif
