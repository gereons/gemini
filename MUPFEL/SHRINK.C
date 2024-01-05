/*
 * shrink.c  -  internal "shrink" command
 * 26.08.90
 */

#include <ctype.h>
#include <string.h>
#include <tos.h>

#include "chario.h"
#include "getopt.h"
#include "mupfel.h"

SCCS(shrink);

static void *shrinkaddr = NULL;	/* Malloc() return */
static size_t shrinksize = 0L;	/* Memory shrinked by this much */
static size_t freemem;

/* called by m_exit() */
void shrinkexit(void)
{
	if (shrinkaddr!=NULL)
		Mfree(shrinkaddr);
}

static int doMfree(void *addr)
{
	if (Mfree(addr)!=0)
	{
		eprintf("shrink: " SK_MFREE "\n");
		return 1;
	}
	return 0;
}

/*
 * static int doshrink(size_t by)
 * Shrink memory by the amount "by". If memory is already shrinked,
 * add "by" to the current shrink value.
 */
static int doshrink(size_t by)
{
	int retcode = 0;
	
	/* Already shrinked? */
	if (shrinkaddr != NULL)
	{
		/* Free memory */
		doMfree(shrinkaddr);
		/* new size */
		shrinksize += by;
	}
	else
		/* first call */
		shrinksize = by;

	/* Shrink memory */
	shrinkaddr = Malloc(shrinksize);
	if (shrinkaddr == NULL)
	{
		eprintf("shrink: " SK_CNTALLOC "\n",shrinksize);
		shrinksize = 0L;
		retcode = 1;
	}
	return retcode;
}

static int doshrinkby(size_t val)
{
	int retcode;

	/* Value out of range? */
	if (val >= freemem)
	{
		eprintf("shrink: " SK_NOSHRINK "\n");
		retcode = 1;
	}
	else
		retcode = doshrink(val);
	return retcode;
}

static int doshrinkto(size_t val)
{
	/* Already shrinked? */
	if (shrinkaddr != NULL)
	{
		/* Free memory */
		doMfree(shrinkaddr);
		shrinkaddr = NULL;
		shrinksize = 0L;
	}

	/* Calculate total free memory */
	freemem = (size_t)Malloc(-1);
	if (val >= freemem)
	{
		/* He wants to keep free more than he's got, the fool */
		eprintf("shrink: " SK_BUYRAM "\n");
		return 1;
	}
	/* Shrink so that val bytes keep useable */
	return doshrink(freemem-val);
}

static int doshrinkfree(void)
{
	int retcode = 0;
	
	/* Did he shrink? */
	if (shrinkaddr == NULL)
	{
		mprintf(SK_NOTSHRNK);
		crlf();
	}
	else
	{
		/* Free memory */
		retcode = doMfree(shrinkaddr);
		shrinkaddr = NULL;
		shrinksize = 0L;
	}
	return retcode;
}

static int doshrinkinfo(void)
{
	if (shrinkaddr == NULL)
		mprintf(SK_NOTSHRNK);
	else
		mprintf(SK_SHRINKBY,
			shrinksize,shrinksize/1024L);
	mprintf(SK_FREE "\n",
		freemem,freemem/1024L);
	return 0;
}

/*
 * static size_t memval(char *arg)
 * Returns the numeric value of arg. arg may be octal (leading 0),
 * decimal or hexadecimal (leading 0x or 0X), and it may be trailed
 * by "m" or "k" to indicate K-Byte (1024) or M-Byte (1024*1024)
 * units.
 */
static size_t memval(char *arg)
{
	size_t val, mult = 1L;
	char factor, *endptr;
	
	factor = toupper(lastchr(arg));
	if (!isdigit(factor))
		/* Check for valid size indicator */
		switch (factor)
		{
			case 'K':
				mult = 1024L;
				break;
			case 'M':
				mult = 1048576L;	/* 1024*1024 */
				break;
			default:
				eprintf("shrink: " SK_ILLSIZE "\n");
				return 0;
		}
	/* Was suffixed by "m" or "k", delete that */
	if (mult != 1L)
		arg[strlen(arg)-1] = '\0';
		
	val = strtoul(arg,&endptr,0);
	if (val==0L || *endptr!='\0')
	{
		/* It was 0 or something wasn't convertible */
		eprintf("shrink: " SK_BADARG "\n");
		val = 0L;
	}
	return val * mult;
}

int m_shrink(ARGCV)
{
	GETOPTINFO G;
	int c;
	int shrinkto, shrinkby;
	static int shrinkfree, shrinkinfo;
	size_t shrinkmem;
	char *shrinkarg;
	struct option long_option[] =
	{
		{ "by", TRUE, NULL, 0 },
		{ "to", TRUE, NULL, 0 },
		{ "free", FALSE, &shrinkfree, TRUE },
		{ "info", FALSE, &shrinkinfo, TRUE },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;

	shrinkto = shrinkby = shrinkfree = shrinkinfo = FALSE;	

	if (argc==1)
		shrinkinfo = TRUE;
		
	optinit (&G);
	
	while ((c = getopt_long (&G, argc, argv, "b:t:fi", long_option,
		&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				if (G.optarg)
					switch(opt_index)
					{
						case 0: goto shrinkby;
						case 1: goto shrinkto;
					}
				break;
			case 'b':
			shrinkby:
				shrinkby = TRUE;
				shrinkarg = G.optarg;
				break;
			case 't':
			shrinkto:
				shrinkto = TRUE;
				shrinkarg = G.optarg;
				break;
			case 'f':
				shrinkfree = TRUE;
				break;
			case 'i':
				shrinkinfo = TRUE;
				break;
			default:
				return printusage(long_option);
		}
	if (shrinkby + shrinkto + shrinkfree + shrinkinfo > 1)
	{
		eprintf("shrink: " SK_ARGUSE "\n");
		return 1;
	}
	freemem = (size_t)Malloc(-1);
	if (shrinkby || shrinkto)
	{
		shrinkmem = memval(shrinkarg);
		if (shrinkmem == 0L)
			return 1;
		if (shrinkby)
			return doshrinkby(shrinkmem);
		else
			return doshrinkto(shrinkmem);
	}
	else
		if (shrinkfree)
			return doshrinkfree();
		else
			return doshrinkinfo();
}
