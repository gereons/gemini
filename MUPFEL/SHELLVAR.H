/*
 * shellvar.h - definitions for shellvar.c
 * 26.11.89
 */

#ifndef _M_SHELLVAR
#define _M_SHELLVAR

#include "imv.h"

void freevar(void);
char *getvar(const char *name);
char *getvarstr(const char *name);
int getvarint(const char *name);
int setvar(const char *name, const char *value);
int varindex(const char *name);
void initdrivelist(void);
void *shvaraddress(void);
void inheritvars(IMV *imv);

#endif