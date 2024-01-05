/*
 * gemsubs.h  -  definitions for gemsubs.c
 * 11.09.90
 */

#ifndef _M_GEMSUBS
#define _M_GEMSUBS

extern int GEMversion;
extern int _maxcol, _maxrow;
extern int _physcol, _physrow;
extern char _prgname[];
 
void initgem(void);			/* install application etc */
void exitgem(void);			/* terminate applicateion */
void mouseon(void);			/* turn mouse on */
void mouseoff(void);		/* turn mouse off */
int buttoncheck(void);		/* TRUE if left buttons is pressed */
void windupdate(int update);	/* tell AES to be quiet */
int shellwrite(int exec,int grafix,int atonce,char *cmd,char *cmdline);
int shellread(char *cmd, char *cmdline);
int windnew(void);
int shellfind(char *path);
void delaccwindows(void);
void gemgimmick(char *cmd,int grow);	/* desktop background, growing box */
void getcursor(int *xcur,int *ycur);	/* get cursor position */
void *savscr(void);
void rstscr(void *scrnbuf);
void savescreen(void);				/* save textscreen */
void restorescreen(void);			/* restore textscreen */

#endif