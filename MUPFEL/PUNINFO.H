/*
	@(#)Mupfel/puninfo.c
	@(#)Julian F. Reschke, 7. Februar 1991
	
	get pointer to harddisk PUN_INFO structure, if available
*/
 
typedef struct
{
     int  puns;      /* Anzahl der physikalischen Laufwerke */
     char pun[16];   /* FÅr jedes logische Laufwerk das 
                        entsprechende physikalische Laufwerk */
     long part_start[16];     /* FÅr jede Partition der Startsektor */

     /* die folgenden Informationen folgen nur bei AHDI ab Version 3.01 und 
     dazu kompatiblen Harddisktreibern. Mit den beiden folgenden Werten zu 
     verifizieren! */

     long P_cookie;      /* Magic-Wert 0x41484449, zu deutsch 'AHDI' */
     long *P_cookptr;    /* Zeiger auf P_cookie */
     unsigned P_version; /* Treiberversion. Hier sollten Harddisktreiber 
                            0x0300 eintragen, wenn sie zu AHDI 3.01 
                            kompatibel sind */
     int  P_max_sector;  /* Maximale Sektorgrîûe im System */
} PUN_INFO;         

#define PUNID      0x0F
#define PUNRES     0x70
#define PUNUNKNOWN 0x80

/* get pointer to PUN_INFO-structure, if available */

PUN_INFO *PunThere (void);
