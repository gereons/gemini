/*
 * @(#) Gemini\greeting.c
 * @(#) Stefan Eissing, 07. April 1991
 *
 * description: functions to greet user
 */

#include <aes.h>
#include <flydial\flydial.h>

#include "vs.h"
#include "greeting.h"
#include "stand.h"

#define BYTE	char
#define WORD	word
#define LONG	long
#include "hello.h"

static int whandle;

static void blinzeln(word start)
{
	word objnr,obx,oby;
	
	rs_object[COMPBLIN].ob_flags &= ~HIDETREE;
	if (start)
		objnr = COMPBLIN;
	else
		objnr = COMPNORM;
	
	objc_offset(rs_object,objnr,&obx,&oby);
	objc_draw(rs_object,objnr,1,obx,oby,
		rs_object[objnr].ob_width,
		rs_object[objnr].ob_height);
}

static void mapIcon(word nr)
{
	long iconnr,masknr,datanr,strnr;
	
	iconnr = (long) rs_object[nr].ob_spec.free_string;
	rs_object[nr].ob_spec.iconblk = &rs_iconblk[iconnr];

	masknr = (long)rs_iconblk[iconnr].ib_pmask;
	rs_iconblk[iconnr].ib_pmask = rs_imdope[masknr].image;

	datanr = (long)rs_iconblk[iconnr].ib_pdata;
	rs_iconblk[iconnr].ib_pdata = rs_imdope[datanr].image;

	strnr = (long)rs_iconblk[iconnr].ib_ptext;
	rs_iconblk[iconnr].ib_ptext = rs_strings[strnr];

	rsrc_obfix(rs_object,nr);
}

static void mapString(word nr)
{
	long strnr;
	
	strnr = (long) rs_object[nr].ob_spec.free_string;
	rs_object[nr].ob_spec.free_string = rs_strings[strnr];

	rsrc_obfix(rs_object,nr);
}

static void mapBox(word nr)
{
	rsrc_obfix(rs_object,nr);
}

static int displayIcon(void)
{
	word obx,oby,obw,obh;
	
	mapBox(0);
	mapIcon(COMPNORM);
	mapIcon(COMPBLIN);
	mapString(TEXT);
	
	rs_object[COMPBLIN].ob_flags |= HIDETREE;
	form_center(rs_object,&obx,&oby,&obw,&obh);
	
	whandle = wind_create (0, obx, oby, obw, obh);
	if (whandle <= 0)
		return FALSE;
		
	if (wind_open (whandle, obx, oby, obw, obh))
	{
		objc_draw(rs_object,0,1,obx,oby,obw,obh);
		return TRUE;
	}
	else
	{
		wind_delete (whandle);
		return FALSE;
	}
}

static void hideIcon (void)
{
	wind_close (whandle);
	wind_delete (whandle);
}

void greetings(void)
{
	static word Times = 0;

	WindUpdate(BEG_UPDATE);
	switch (Times)
	{
		case 0:
			if (displayIcon ())
				++Times;
			else
				Times = 99;
			break;
		case 1:
			blinzeln (TRUE);
			++Times;
			break;
		case 2:
			blinzeln (FALSE);
			++Times;
			break;
		case 3:
			hideIcon ();
			++Times;
			break;
	}
	WindUpdate(END_UPDATE);
	return;
}