/*
 * @(#) Gemini\vs.h
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: Header File for the new Desktop
 *
 */

#include <stdio.h>
#include <aes.h>

#define store_sccs_id(a) \
	/* char a##_id[] = "@(#) "__FILE__" "__DATE__ */

/*
 * Macros und Defines
 */

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#ifndef NULL
#define NULL ((void *)0L)
#endif

#ifndef max
#define max(a,b)	((a)>(b))?(a):(b)
#define min(a,b)	((a)<=(b))?(a):(b)
#endif

#ifndef DIM
#define DIM(x)		(sizeof(x) / sizeof(x[0]))
#endif


/*
 * Typen
 */

typedef unsigned char byte;			/* 8 Bit unsigned */
typedef unsigned char uchar;		/* 8 Bit unsigned */

typedef int word;					/* 16 Bit signed */
typedef unsigned int uword;			/* 16 Bit unsigned */

typedef unsigned int uint;

typedef unsigned long ulong;		/* 32 Bit unsigned */

/*
 * define the language to be used in Gemini
 */
#define GERMAN		1
#define ENGLISH		(!GERMAN)

#define WILDLEN     30      /* maximal length for wildcards */
#define MAXLEN		128		/* (maximal) string length */
#define DRAG		0x4000	/* zu draggendes Icon, Marker im Typ */

#ifndef MAX_FILENAME_LEN

#define MAX_FILENAME_LEN		15

#endif

/* file attributs
 */
#define FA_RDONLY       0x01
#define FA_HIDDEN       0x02
#define FA_SYSTEM       0x04
#define FA_LABEL        0x08
#define FA_FOLDER       0x10
#define FA_ARCHIV       0x20
#define FA_ALL		(FA_HIDDEN|FA_SYSTEM|FA_LABEL|FA_FOLDER)
#define FA_NOLABEL	(FA_HIDDEN|FA_SYSTEM|FA_FOLDER)

#define CONF_LEN	256		/* LÑnge einer Zeile fÅr Config-Info */

#define W_OPENED		TRUE
#define W_CLOSED		FALSE

/* udate-flags for WindInfo
*/
#define SHOWTYPE		0x0001
#define WINDNAME		0x0002
#define WINDINFO		0x0004
#define HOSLIDER		0x0008
#define VESLIDER		0x0010
#define SLIDERRESET		0x0020
#define POSITION		0x0040
#define NOWINDDRAW		0x0080
#define MUPFELTOO		0x0100
#define KILLREDRAW		0x1000

#define NEWWINDOW		(SHOWTYPE|WINDNAME|WINDINFO|VESLIDER)

/* Icontypes (fÅr Desktophintergrund)
 */
#define DI_FILESYSTEM	0x0001
#define DI_TRASHCAN		0x0002
#define DI_SHREDDER		0x0003
#define DI_SCRAPDIR		0x0004
#define DI_FOLDER		0x0005
#define DI_PROGRAM		0x0006

/* startmodes */
#define TOS_START		0x1000	/* start as tos application */
#define GEM_START		0x0001	/* make gem background */
#define TTP_START		0x0002	/* always get commandline */
#define WCLOSE_START	0x0004	/* close windows when starting */
#define WAIT_KEY		0x0008	/* wait for key after termination */
#define OVL_START		0x0010  /* starten als Overlay */

/* kind of window */
#define WK_FILE			0x0001	/* normales window mit Dateien */
#define WK_MUPFEL       0x0002  /* Mupfel-Window */
#define WK_DESK			0x0003  /* Desktophintergrund */
#define WK_ACC			0x0004	/* Fenster gehîrt Accessory */

typedef struct iconInfo
{
	char magic[5];				/* Iconmagic "IcoN" */
	word type;					/* Typ des Icons (s.o.) */
	word defnumber;				/* Nummer des default Icons */
	word altnumber;				/* Nummer eines altern. Icons */
	word shortcut;				/* shortcut fÅr das Icon */
	char truecolor;				/* gewÅnschte Iconfarbe */
	ICONBLK ib;					/* Iconblock des Icons */
	char iconname[MAX_FILENAME_LEN];	/* String im Icon */
	char path[MAXLEN];			/* default Path */
	char label[MAX_FILENAME_LEN];	/* Label of drive it came from */
} IconInfo;

typedef struct displayRule
{
	struct displayRule *prevrule;		/* doppelt */
	struct displayRule *nextrule;		/* verkettete Liste */
	struct displayRule *nextHash;		/* nÑchste gehashte rule */
	word wasHashed;						/* wurde gehasht */
	char wildcard[WILDLEN];				/* Wildcard fÅr Filename */
	word isFolder;						/* Flag for Folders only */
	word iconNr;						/* Nummer des Icons */
	char truecolor;						/* gewÅnschte Farbe */
	char color;							/* reale Farbe des Icons */
} DisplayRule;

typedef struct
{
	OBJECT *tree;					/* pointer to tree-array */
	word maxobj;					/* max array entries */
	word objanz;					/* current entries */
	word selectAnz;					/* no. of selected Items */
	word scrapNr;					/* no. of scrapicon */
	word trashNr;					/* no. of trashicon */
	word emptyPaper;				/* empty Paperbasket on exit */
	word waitKey;					/* wait for key after TOS Apps */
	word ovlStart;					/* start everything as overlay */
	word silentCopy;				/* Cp/Mv without Dialog */
	word silentRemove;				/* Rm without Dialog */
	word replaceExisting;			/* Replace existing Files */
	word showHidden;				/* show hidden files */
	word askQuit;					/* ask when quitting */
	word saveState;					/* save state on exiting */
	word snapx, snapy;				/* snap von Icons beim AufrÑumen*/
} DeskInfo;

typedef struct
{
	word aligned;					/* Windows character aligned */
	word normicon;					/* norm or small icons */
	word fsize;						/* Size of File */
	word fdate;						/* Date of File */
	word ftime;						/* Time of File */
	word showtext;					/* checked menuentry for show */
	word sortentry;					/* checked menuentry for sort */
	word m_cols;					/* Werte fÅrs Mupfel Window */
	word m_rows;
	word m_inv;
	word m_font;
	word m_fsize;
	word m_wx;
	word m_wy;
}ShowInfo;

typedef struct confInfo						/* Configuration List */
{
	struct confInfo *nextconf;
	char *line;
} ConfInfo;

#define TEXTLEN		48
struct texttype
{
	USERBLK	ublk;
	char text[TEXTLEN];
};

typedef union fileUnion
{
	ICONBLK i;						/* enthÑlt Icon Inforationen */
	struct texttype t;				/* String fÅr Textanzeige */
} FileUnion;

typedef struct fileInfo
{
	char magic[5];					/* filemagic "FilE" */
	char fullname[13];				/* Full Name from TOS */
	char name[9];					/* name of File */
	struct
	{
		noObject : 1;				/* ob die FileUnion gefÅllt ist */
		selected : 1;				/* file ist selektiert */
		dragged  : 1;				/* wurde verschoben */
	} flags;
	char ext[4];					/* extension */
	char attrib;					/* fileattribut */
	long size;						/* length of file */
	uword date;
	uword time;
	uword number;					/* laufende Nummer fÅr unsort */
	FileUnion o;
} FileInfo;


#define FILEBUCKETSIZE	40

typedef struct fileBucket
{
	struct fileBucket *nextbucket;
	word usedcount;
	FileInfo finfo[FILEBUCKETSIZE];
} FileBucket;

typedef struct windInfo
{
	struct windInfo *nextwind;		/* Pointer zum nÑchsten Window */
	word kind;						/* Art des Windows */
	word owner;						/* ApId. des Besitzers */
	word update;					/* Flags fÅr updates */
	word handle;					/* Kennung des Windows */
	word type;						/* Windowtyp */
	word windx,windy,windw,windh;	/* totale Grenzen des Windows */
	word workx,worky,workw,workh;	/* Grenzen der Workarea */
	word oldx,oldy,oldw,oldh;		/* fÅr Fullbox */
	OBJECT *tree;					/* Pointer auf Hintergrund */
	word objanz;					/* Anzahl der Objekte im Baum */
	word selectAnz;					/* Anzahl der selektierten Objekte */
	long selectSize;				/* Gesamtgrîûe der selektierten Objekte */
	word maxlines;					/* maximale Anzahl von Zeilen */
	word xanz,yanz;					/* Anzahl der Spalten/Zeilen */
	word xdist;						/* x-Distanz zwischen 2 Objekten
									 * y-Distanz ist immer 1 */
	word xskip,yskip;				/* Wieviel Objekte bei der Anzeige
									 * links/oben unsichtbar sind */
	OBJECT *stdobj;					/* Addresse des verwendeten 
										Standerdobjekts */
	word obw;						/* Breite eines Objekts */
	word obh;						/* Hîhe eines Objekts */
	word sorttype;					/* wie sortiert wurde */
	word extratitle;				/* TRUE -> Titelstring ist gÅltig */
	char path[MAXLEN];				/* Pfad des Windows */
	char wildcard[WILDLEN];			/* Wildcard fÅr Files */
	char label[MAX_FILENAME_LEN];	/* label of drive, files came
									   from */
	char *aesname;					/* titel fÅr das AES */
	char title[MAXLEN];				/* Titelzeile */
	char info[MAXLEN];				/* Infozeile */
	FileBucket *files;				/* Liste der Files */
	word fileanz;					/* Anzahl der Files */
	long dirsize;					/* Gesamtgrîûe des Directories */
	word vslpos;					/* Position vertikaler Slider */
	word vslsize;					/* dessen Grîûe */
	word hslpos;					/* Position horizontaler Slider */
	word hslsize;					/* dito */
} WindInfo;
