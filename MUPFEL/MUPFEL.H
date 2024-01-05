/*
 * mupfel.h  -  prototypes, typedefs, #defines etc.
 * 05.11.90
 */

#ifndef _M_MUPFEL
#define _M_MUPFEL

/* language definitions */

#include "messages.h"
#include "getopt.h"

/* global definitions & macros */

typedef	unsigned long	size_t;
typedef	unsigned char	uchar;
typedef	unsigned int	uint;
typedef	unsigned long	ulong;

#define MAXARGC	512		/* entries in argv array */
#define CMDLINESIZE	300

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#ifndef NULL
#define NULL	((void *)0L)
#endif

#ifndef EOF
#define EOF	(-1)
#endif

#define ARGCV		int argc,char **argv
#define DIM(x)		(sizeof(x)/sizeof(x[0]))
/* #define SCCS(x)	char _##x##_id[] = "@(#)"__FILE__", compiled "__DATE__ */
#define SCCS(x)

/* Function codes for Fdatime() and Fattrib() */
#define GETDATE	0
#define SETDATE	1
#define GETATTRIB	0
#define SETATTRIB	1

/* bit operations */
#define tstbit(a,b)	((a&b)==b)
#define setbit(a,b)	(a|=b)
#define clrbit(a,b)	(a&=~b)

/* 
 * global vars from mupfel.c 
 */
extern int bioserr, oserr, shellcmd, sysret, linecont, gotovenus;
extern comefromvenus;
extern const char *actcmd;

/* 
 * prototypes from misc.c
 */
int access(const char *path,int mode);
void chrcat(char *str,char c);
int strpos(const char *str,char c);
int strrpos(const char *str,char c);
int isdir(const char *dir);
int isdrive(const char *drv);
int validfile(const char *name);
char lastchr(const char *str);
char *strfnm(char *path);		/* get file name part */
char *strdnm(const char *path);	/* get dir name part */
char *strdup(const char *str);
char *newstr(char **dest, const char *src);
int isatty(int handle);
void chrapp(char *str,char ch);
char *itos(int i);
char *mstrtok(char *str, const char *delim);
int wildcard(const char *str);
int anywild(int count, char **array);
char charselect(const char *allowed);
char *strins(char *dest, const char *ins, size_t where);
int ioerror(const char *cmd,const char *str,void *dta,int errcode);
int isdevice(const char *file);
size_t drvbit(const char *file);
void print_longoption(char *options, struct option *long_option);
int printusage(struct option *long_option);
/* codes for access() */
#define A_READ		1
#define A_WRITE	2
#define A_RDWR		3
#define A_EXEC		4
#define A_EXIST	5
#define A_RDONLY	6

/*
 * prototypes from mupfel.c 
 */
void lateinit(void);			/* late inits, called by batch.c */
void copyright(void);			/* copyright message */
char *mupfel(const char *cmd);	/* main control loop */
void storevenuscmd(const char *cmd, int argc, const char *argv[]);

#endif