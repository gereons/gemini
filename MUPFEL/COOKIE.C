/*
 * cookie.c  -  Cookie Jar management
 * 04.06.90
 *
 * 19.11.90 jr: _FPU cookie eingebaut
 */

#include <string.h>

#include "chario.h"
#include "cookie.h"
#include "mupfel.h"
#include "strsel.h"
#include "sysvec.h"

static int first;

int legalcookie(void *cj)
{
	return cj!=NULL && ((size_t)cj%2)==0 && (long)cj>0;
}

static char *vdoval(long val)
{
	static char *vdotxt[] =
	{
		"ST", "STE", "TT"
	};

	val >>= 16;	
	if (val>=0 && val<DIM(vdotxt))
		return vdotxt[val];
	else
		return "???";
}

static void fpuval(long val)
{
	int comma = 0, hword, bit0, bit12, bit3;
	
	hword = *((int *)&val);
	bit0 = hword & 0x01;
	bit12 = hword & 0x06;
	bit3 = hword & 0x08;
	
	if (bit0)
		mprintf("SFP004"), ++comma;

	if (bit12)
	{	
		if (bit12 == 2)
			mprintf("%s68881/68882",comma ? ", " : "");
		else if (bit12 == 4)
			mprintf("%s68881",comma ? ", " : "");
		else if (bit12 == 6)
			mprintf("%s68882",comma ? ", " : "");

		++comma;
	}

	if (bit3)
		mprintf("%s68040",comma ? ", " : "");
}

static char *mchval(long val)
{
	static char *mchtxt[] =
	{
		"520/1040/Mega ST", "STE", "TT"
	};

	val >>= 16;
	if (val >= 0 && val < DIM(mchtxt))
		return mchtxt[val];
	else
		return "???";
}

static void sndval(long val)
{
	int comma = 0;
	
	if (val & 01)
		mprintf("GI/Yamaha"), ++comma;
	if (val & 02)
		mprintf("%sDMA",comma ? ", " : "");
}

static void showcookie(long *cj)
{
	char cookie[] = "____";
	long cookieval;
	
	strncpy(cookie,(char *)cj,4);
	cookieval = *(cj+1);
	if (first)
	{
		mprintf(CO_HEADER "\n");
		first = FALSE;
	}
	mprintf("%s    %08lx",cookie,cookieval);
	STRSELECT(cookie)
	WHEN("_CPU")
		mprintf(" (%ld)\n",68000L+cookieval);
	WHEN("_VDO")
		mprintf(" (%s)\n",vdoval(cookieval));
	WHEN("_FPU")
		mprintf(" (");
		fpuval(cookieval);
		mprintf(")\n");
	WHEN("_SND")
		mprintf(" (");
		sndval(cookieval);
		mprintf(")\n");
	WHEN("_MCH")
		mprintf(" (%s)\n",mchval(cookieval));
	DEFAULT
		crlf();
	ENDSEL
}
 
int m_cookie(void)
{
	long *cookiejar;
	
	cookiejar = getcookiejar();
	
	if (!legalcookie(cookiejar))
	{
		eprintf("cookie: " CO_NOCOOKIE "\n");
		return 1;
	}

	first = TRUE;	
	while (*cookiejar != 0)
	{
		showcookie(cookiejar);
		cookiejar += 2;
	}
	return 0;
}
