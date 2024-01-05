/*
 * strsel.h  -  switch-like constructs for strings
 * 26/08/88
 */

#ifndef _M_STRSEL
#define _M_STRSEL

#include <string.h>

#define STRSELECT(a) { char *_s; _s=a; if (a!=_s) { 

#define WHEN(a)	} else if (!strcmp(_s,a)) {

#define WHEN2(a,b)	} else if (!strcmp(_s,a) || !strcmp(_s,b)) {

#define WHEN3(a,b,c) } else if (!strcmp(_s,a) || !strcmp(_s,b) || !strcmp(_s,c)) {

#define DEFAULT	} else {

#define ENDSEL		} }

#endif