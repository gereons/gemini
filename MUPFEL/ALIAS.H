/*
 * alias.h - definitions for alias.c
 * 05.01.91
 */

#ifndef _M_ALIAS
#define _M_ALIAS

#include "imv.h"

#define NOALIAS		"noalias"

void aliasinit(void);
void aliasexit(void);
char *isalias(char *name);
char *searchalias(char *name);
void *aliasaddress(void);
void unusedalias(void);
void inheritalias(IMV *imv);

#endif