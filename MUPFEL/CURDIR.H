/*
 * curdir.h - definitions for curdir.c
 * 15.05.90
 */

#ifndef _M_CURDIR
#define _M_CURDIR
 
int setcurdir(void);			/* ask GEMDOS for current dir */
void curdirinit(void);			/* init current dir */
void direxit(void);				/* de-init pushdir stack */
int legaldrive(char drv);		/* check legal drives */
int chdrv(char drv);			/* change current drive */
int chdir(const char *dir);		/* change current dir */
void savedir(void);				/* remember current dir */
void restoredir(void);			/* cd back to saved dir */
char getdrv(void);				/* get current drive */
char *getdir(void);				/* get current dir */
void setdrv(char drv);
void setdir(char *dir);
char *normalize(const char *dir);	/* make absolute path from dir */

#endif