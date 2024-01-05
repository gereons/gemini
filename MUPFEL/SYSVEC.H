/*
 * sysvec.h - definitions for sysvec.c
 * 25.01.91
 */

#ifndef _M_SYSVEC
#define _M_SYSVEC

#include <tos.h>
#include "date.h"

extern int conwindow;
 
void shellinit(void);
void shellexit(void);
void criticinit(void);
void criticexit(void);
void etvterminit(void);
void etvtermexit(void);
int getnflops(void);
ulong gethz200(void);
int pgmcrash(void);
void clearcrashflag(void);
void getsysvar(unsigned char *osverlo, unsigned char *osverhi,
			 dosdate **osdate,int *oscountry);
BASPAG *getactpd(void);
void setactpd(BASPAG *bp);
void coninit(void);
void conexit(void);
void *getcookiejar(void);
void initcookie(void);
void removecookie(void);
void cursinit(void);
void cursexit(void);
int getclick(void);
void keyclick(int on);
void setLAxy(int x, int y);
void resetLAxy(void);

#endif
