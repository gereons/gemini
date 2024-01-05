/*
	@(#)Mupfel/version.c
	@(#)Gereon Steffens & Julian F. Reschke, 27. April 1991

	pun_info now comes from puninfo.h
*/

#include <string.h>
#include <tos.h> 

#include "chario.h"
#include "date.h"
#include "gemsubs.h"
#include "getopt.h"
#include "mupfel.h"
#include "mversion.h"
#include "puninfo.h"
#include "sysvec.h"
#include "version.h"

SCCS(version);

static char *countryid[] =
{
	"USA", "FRG", "FRA", "UK",  "SPA", "ITA", "SWE", "SWF",
	"SWG", "TUR", "FIN", "NOR", "DEN", "SAU", "HOL", "???"
};

static uchar osverlo, osverhi;
static int oscountry;
static dosdate *osdate;
static int tosver;

void initsysvar(void)
{
	getsysvar(&osverlo,&osverhi,&osdate,&oscountry);
	if (oscountry >= DIM(countryid))
		oscountry = (int)DIM(countryid)-1;
	tosver = (osverhi<<8) | osverlo;
}

/*
 * return TOS version.
 */
int tosversion(void)
{
	return tosver;
}

int m_version(ARGCV)
{
	GETOPTINFO G;
	int c;
	static int mupfelversion, gemdosversion, tosversion, gemversion,
		driverversion;
	int sver, gemverlo, gemverhi;
	struct option long_option[] =
	{
		{ "all", FALSE, NULL, 0 },
		{ "mupfel", FALSE, &mupfelversion, TRUE },
		{ "gem", FALSE, &gemversion, TRUE },
		{ "gemdos", FALSE, &gemdosversion, TRUE },
		{ "tos", FALSE, &tosversion, TRUE },
		{ "harddisk", FALSE, &driverversion, TRUE },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;
	
	mupfelversion = gemdosversion = tosversion = 
		gemversion = driverversion = FALSE;
	
	optinit (&G);
	
	while ((c = getopt_long (&G, argc, argv, "amgdth", long_option,
		&opt_index)) != EOF)
		switch(c)
		{
			case 0:
				if (opt_index == 0)
					goto all;
				break;
			case 'm':
				mupfelversion = TRUE;
				break;
			case 'g':
				gemversion = TRUE;
				break;
			case 't':
				tosversion = TRUE;
				break;
			case 'd':
				gemdosversion = TRUE;
				break;
			case 'h':
				driverversion = TRUE;
				break;
			case 'a':
			all:
				mupfelversion = gemdosversion = tosversion =
					gemversion = driverversion = TRUE;
				break;
			default:
				return printusage(long_option);
		}

	if (argc==1)
		mupfelversion = TRUE;
	
	if (tosversion)
	{
		mprintf("TOS " VE_VERSION " %d.%02x [%02d.%02d.%02d] (%s)\n",
			osverhi,osverlo,
			osdate->s.day,osdate->s.month,osdate->s.year+80,
			countryid[oscountry]);
	}

	if (gemversion)
	{
		gemverhi = GEMversion / 256;
		gemverlo = GEMversion & 255;
		mprintf("GEM " VE_VERSION " %d.%0*d\n",
			gemverhi,
			gemverlo < 0x10 ? 2 : 1,
			gemverlo < 0x10 ? gemverlo : gemverlo>>4);
	}

	if (gemdosversion)
	{
		sver = Sversion();
		mprintf ("GEMDOS " VE_VERSION " %02x.%02x\n", sver & 0xff,
			sver/256);
	}

	if (mupfelversion)
		mprintf("Mupfel " VE_VERSION " %s [%s]\n",MUPFELVERSION,MUPFELDATE);

	if (driverversion)
	{
		PUN_INFO *pi = PunThere ();
	
		if (pi)
		{
			mprintf(VE_HARDDISK " ");
			if (pi->P_cookptr == &pi->P_cookie && pi->P_cookie == 0x41484449L) /* "AHDI" */
				mprintf("%d.%02d\n",pi->P_version/256,pi->P_version&255);
			else
				mprintf(VE_NOTACC "\n");
		}
		else
			mprintf(VE_NOHARD "\n");
	}
	
	return 0;
}
