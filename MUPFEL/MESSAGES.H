/*
 * messages.h  -  "Multilingual" support for Mupfel
 * 04.06.90
 */

#ifndef _M_MESSAGES
#define _M_MESSAGES

/* constants for the avaliable languages */
#define LNG_ENGLISH	1
#define LNG_GERMAN	2

/* define which language to use */
#define LANGUAGE	LNG_GERMAN

/* now, include the appropriate message file */
#if LANGUAGE == LNG_GERMAN
	#include "msg_ger.h"
#elif LANGUAGE == LNG_ENGLISH
	#include "msg_eng.h"
#endif

#if !defined(AL_CANTSET)
#error unknown LANGUAGE
#endif

#endif
