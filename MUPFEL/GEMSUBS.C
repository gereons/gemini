/*
 * gemsubs.c  -  gem aes & vdi routines
 * 05.01.91
 */

#include <aes.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <vdi.h> 

#include "alloc.h"
#include "chario.h"
#include "comm.h"
#include "gemsubs.h"
#include "mupfel.h"
#include "shellvar.h"
#include "stand.h"
#include "vt52.h"

#if MERGED
#include "flydial\flydial.h"
#define graf_mouse(x,y)	GrafMouse(x,y)
#define wind_update(x)	WindUpdate(x)
#endif

SCCS(gemsubs);

int _maxcol, _maxrow, _physcol, _physrow;
int GEMversion;
static char _prgname[256];

static int aes_wk;			/* handle for physical workstation */
static int my_vwk;			/* Mupfel's virtual workstation */
static int *mup_vwk = &my_vwk;/* address of workstation handle */
static int xpix, ypix;		/* x/y axis resolution */
static int wchar, hchar;		/* character width/height */
static int planes;			/* # of bitplanes on screen */
static int xcursor, ycursor;	/* saved cursor pos for screensave */

static void *screenbuf;
static size_t screensize;
static MFDB screenMFDB, bufferMFDB;
static int pxy[8];	/* for vro_cpyfm() */

/*
 * mouseon(void)
 * set mouse to busy bee, turn mouse on
 */
void mouseon(void)
{
	graf_mouse(HOURGLASS,NULL);
	graf_mouse(M_ON,NULL);
}

/*
 * mouseoff(void)
 * turn mouse off
 */
void mouseoff(void)
{
	graf_mouse(M_OFF,NULL);
}

void windupdate(int update)
{
	wind_update(update ? BEG_UPDATE : END_UPDATE);
}

/*
 * gemgimmick(void)
 * display desktop-like opening sequence
 */
void gemgimmick(char *cmd,int grow)
{
	extern void makeBarName(char *);
#if STANDALONE
	int pxy[4];
	int textx, texty;
#endif

	windupdate(BEG_UPDATE);
	/* force redraw of desktop background */
	wind_set(0,WF_NEWDESK,0,0,0,0);
	form_dial(FMD_FINISH,0,0,0,0,0,0,xpix,ypix);
	
#if STANDALONE
	/* white bar at top */
	pxy[0] = pxy[1]= 0;
	pxy[2]=xpix;
	pxy[3]=ypix;
	vs_clip(*mup_vwk,TRUE,pxy);

	vswr_mode(*mup_vwk,MD_REPLACE);
	vsf_interior(*mup_vwk,FIS_SOLID);
	vsl_type(*mup_vwk,1);				/* durchgezogene Linie */
	vsf_color(*mup_vwk,WHITE);
	pxy[0] = pxy[1] = 0;
	pxy[2] = xpix;
	pxy[3] = hchar+2;
	v_bar(*mup_vwk,pxy);
	/* command's name centered */
	textx = (xpix - (int)strlen(cmd)*wchar)/2;
	texty = hchar - 1;
	v_gtext(*mup_vwk,textx,texty,cmd);
	/* black line under bar */
	pxy[0] = 0;
	pxy[1] = pxy[3] = hchar+2;
	pxy[2] = xpix;
	v_pline(*mup_vwk,2,pxy);
	/* growing box if needed */
	if (grow)
		form_dial(FMD_GROW,xpix/2,ypix/2,1,1,0,0,xpix,ypix);
#endif
	windupdate(END_UPDATE);
	
#if MERGED
	makeBarName(cmd);
	(void)grow;
#endif
}

static void openvwork(void)
{
	int workin[11] = { 1,1,1,1,1,1,1,1,1,1,2 };
	int workout[57];
	int dummy;
#if MERGED
	extern int handle;	/* Steve's workstation handle */
#endif

	my_vwk = aes_wk = graf_handle(&wchar, &hchar, &dummy, &dummy);
	v_opnvwk(workin,&my_vwk,workout);

	if (my_vwk <= 0)
	{
		mprintf(GM_NOVDI "\n",CommInfo.PgmName);
		exit(2);
	}
	xpix = workout[0];	/* x-axis resolution */
	ypix = workout[1];	/* y-axis resolution */

	vq_extnd(my_vwk,1,workout);
	planes = workout[4];

	/* get screen size in charaters */
	vq_chcells(my_vwk,&_maxrow,&_maxcol);
	_physcol = --_maxcol;
	_physrow = --_maxrow;
#if MERGED
	v_clsvwk(my_vwk);
	my_vwk = -1;
	mup_vwk = &handle;
#endif
}

/*
 * geminit(void) 
 * init application, open virtual workstation
 */
void initgem(void)
{
	char dummy[128];
	
	_GemParBlk.global[0] = 0;
	if (appl_init()<0)
	{
		mprintf(GM_NOAES "\n",CommInfo.PgmName);
		exit(2);
	}
	
	if (_GemParBlk.global[0] == 0)
	{
		mprintf(GM_NOAUTO "\n",CommInfo.PgmName);
		exit(0);
	}
	
#if MERGED
	graf_mouse(HOURGLASS,NULL);
#endif
	GEMversion = _GemParBlk.global[0];

	shel_read(_prgname,dummy);

	if (_prgname[1] != ':')
	{
		strcpy(dummy,_prgname);		/* save name */
		_prgname[0] = Dgetdrv() + 'A';
		_prgname[1] = ':';
		Dgetpath(&_prgname[2],0);
		chrapp(_prgname,'\\');
		strcat(_prgname,dummy);
	}

	CommInfo.PgmPath = _prgname;

	/* open virtual workstation */	
	openvwork();	

	/* prepare MFDB's for screensave */
	screensize = ((long)(xpix+1) * (long)(ypix+1) * (long)planes)/8L;

	pxy[0] = pxy[1] = pxy[4] = pxy[5] = 0;
	pxy[2] = pxy[6] = xpix;
	pxy[3] = pxy[7] = ypix;
	/* fill source MFDB */
	screenMFDB.fd_addr = NULL;
	/* fill destination MFDB */
	bufferMFDB.fd_w = xpix+1;
	bufferMFDB.fd_h = ypix+1;
	bufferMFDB.fd_wdwidth = (xpix+1)/16;
	bufferMFDB.fd_stand = 0;
	bufferMFDB.fd_nplanes = planes;
	
	screenbuf = NULL;
#if STANDALONE
	mouseoff();
	windupdate(TRUE);
#endif
}

/*
 * exitgem(void)
 * exit application, close virtual workstation
 */
void exitgem(void)
{
	cursoroff();
#if STANDALONE
	mouseon();
	windupdate(FALSE);
#endif
	if (my_vwk > 0)
		v_clsvwk(my_vwk);
	appl_exit();
}

int shellread(char *cmd, char *cmdline)
{
	return shel_read(cmd,cmdline);
}

int shellwrite(int exec,int grafix,int atonce,char *cmd,char *cmdline)
{
	return shel_write(exec,grafix,atonce,cmd,cmdline);
}

int windnew(void)
{
	if (GEMversion==0x104 || (GEMversion>=0x130 && GEMversion!=0x210))
	{
		wind_new();
		return TRUE;
	}
	else
		return FALSE;
}

int shellfind(char *path)
{
	return shel_find(path);
}

void delaccwindows(void)
{
	int window;
	
	while (wind_get(0,WF_TOP,&window)>0 && window>0)
		wind_delete(window);
}

/*
 * getcursor(int *xcur, int *ycur)
 * save current cursor position in *xcur and *ycur
 */
void getcursor(int *xcur, int *ycur)
{
#if STANDALONE
	vq_curaddress(*mup_vwk,ycur,xcur);
	--(*xcur);
	--(*ycur);
#else
	extern void TM_GetCursor(int *x, int *y);
	
	TM_GetCursor(xcur,ycur);
#endif
}

static void *round256(void *x)
{
	return (void *)((size_t)x+256L & (size_t)(~255L));
}

void *savscr(void)
{
	void *scrnbuf;
	
	scrnbuf = Malloc(screensize + 1024L);
	if (scrnbuf != NULL)
	{
		bufferMFDB.fd_addr = round256(scrnbuf);
		vro_cpyfm(*mup_vwk,3,pxy,&screenMFDB,&bufferMFDB);
	}
	return scrnbuf;
}

void rstscr(void *scrnbuf)
{
	if (scrnbuf != NULL)
	{
		bufferMFDB.fd_addr = round256(scrnbuf);
		vro_cpyfm(*mup_vwk,3,pxy,&bufferMFDB,&screenMFDB);
		Mfree(scrnbuf);
	}
}

/*
 * savescreen(void)
 * if internal var screensave has a value, save screen contents
 * to dynamically allocated memory
 */
void savescreen(void)
{
	if (getvar("screensave")==NULL)
	{
		screenbuf=NULL;
		return;
	}
	screenbuf = savscr();
	getcursor(&xcursor,&ycursor);
}

/*
 * restorescreen(void)
 * if screen was saved, restore it
 */
void restorescreen(void)
{
	rstscr(screenbuf);
	screenbuf=NULL;
	vt52cm(xcursor,ycursor);
}
