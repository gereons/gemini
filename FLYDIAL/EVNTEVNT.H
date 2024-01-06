/*
	@(#)FlyDial/evntevnt.h
	@(#)Julian F. Reschke, 30. Juni 1990
*/

#ifndef evnt_event

#include <aes.h>

typedef struct
{
	unsigned int e_flags, e_bclk, e_bmsk, e_bst, e_m1flags;
	GRECT e_m1;
	unsigned int e_m2flags;
	GRECT e_m2;
	int *e_mepbuf;
	unsigned long e_time;
	int  e_mx, e_my;
	unsigned int e_mb, e_ks, e_kr, e_br, e_m3flags;
	GRECT e_m3;
	int  e_xtra0;
	int  *e_smepbuf;
	unsigned long e_xtra1, e_xtra2;
} MEVENT;

int evnt_event (MEVENT *);

#endif
