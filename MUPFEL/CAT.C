/*
	@(#)Mupfel/cat.c
	@(#)Julian F. Reschke, 29. Januar 1991
*/

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "chario.h"
#include "getopt.h"
#include "handle.h"
#include "keys.h"
#include "mupfel.h"
#include "scan.h"

SCCS (cat);


/* Structure containing all information relevant to cat */

typedef struct
{
	uchar 	*input_buffer;		/* Pointer to input buffer */
	ulong	input_len;			/* its length */
	ulong	pending;			/* waiting characters */
	uchar 	*output_buffer;		/* Pointer to output buffer */
	ulong	output_len;			/* its length */
	int		tty_in, tty_out;	/* isatty (0), isatty (1) */
	int		last_char;
	ulong	line_cnt;
	uchar	small_inbuf[512];	/* if Malloc() fails */
	uchar	small_outbuf[512];

	/* the different options */

	int		silent;				/* be silent about non-existing files */
	int		insert_lf;			/* insert lf after cr */
	int		insert_cr;			/* *NIX to TOS */
	int		remove_cr;			/* TOS to *NIX */
	int		nonprinting;		/* show nonprinting */
	int		showtabs;			/* show \t as ^I */
	int		showmeta;			/* show ascii codes > 128 */
	int		showends;			/* show \n as $	*/
	int		number;				/* number lines */
} CATINFO;


/* Functions to free IO buffers */

static void free_buffers (CATINFO *C)
{
	if (C->input_buffer != C->small_inbuf) Mfree (C->input_buffer);
	if (C->output_buffer != C->small_outbuf) Mfree (C->output_buffer);
}


/* Functions to allocate memory for IO buffers */

static void alloc_buffers (CATINFO *C)
{
	long free_amount = (long)Malloc (-1L);
	long sizes;
	
	sizes = free_amount / 2;		/* need two buffers */
	if (sizes > 16384L) sizes = 16384L;
	if (sizes < 1024L) sizes = 1024L;
	
	C->input_len = C->output_len = sizes;
	C->input_buffer = Malloc (C->input_len);
	C->output_buffer = Malloc (C->output_len);

	if (!C->input_buffer)
	{
		C->input_buffer = C->small_inbuf;
		C->input_len = sizeof (C->small_inbuf);
	}
	
	if (!C->output_buffer)
	{
		C->output_buffer = C->small_outbuf;
		C->output_len = sizeof (C->small_outbuf);
	}
}


/* convert character according to the conversion options */

static int l_conv (CATINFO *C, int ch, char *to)
{
	char *recall = to;
	
	if ((ch == 13) && C->remove_cr) return 0;

	if (ch == 13)
		if (C->showends) *to++ = '$';

	if (ch == 10)
		if (C->insert_cr)
		{
			*to++ = 13; 
		}
		
	if ((ch >= 128) && C->showmeta)
	{
		*to++ = 'M';
		*to++ = '-';
		ch -= 128;
	}

	if ((ch > 0) && (ch < 32) && C->nonprinting)
	{
		if ((ch != 13) && (ch != 10))
			if (C->showtabs || (ch != '\t'))
			{
				*to++ = '^';
				ch += 64;
			}
	}
	
	if ((ch == 127) && C->nonprinting)
	{
		*to++ = '^';
		ch = '?';
	}

	*to++ = ch;
	
	if ((ch == 13) && (C->insert_lf)) *to++ = 10;
		
	return (int)(to - recall);	
}



/* do conversion on buffers, return number of converted chars */

static long convert (CATINFO *C, long amount)
{
	long index = 0L;
	long out_cnt;
	uchar *in, *out;
	char c_buf[6];
	
	out_cnt = 0L;
	in = C->input_buffer;	
	out = C->output_buffer;	
	
	/* insert special case for no conversion */
	
	while (index < amount)
	{
		int len;

		if (C->number && (C->last_char == 10))
		{
			if (out_cnt + 7 > C->output_len)
				break;
			else
			{
				int len;
				
				len = sprintf ((char *)out, "%7ld\t", ++C->line_cnt);

				out += len;
				out_cnt += len;
			}
		}

		len = l_conv (C, in[index], c_buf);
	
		if (out_cnt + len > C->output_len)
			break;

		memcpy (out, c_buf, len);
		out += len;
		out_cnt += len;
		
		C->last_char = len ? c_buf[len-1] : 0;
		
		index++;
	}

	C->pending = (amount - index);
	memcpy (C->input_buffer, &C->input_buffer[index], C->pending);

	return out_cnt;
}



/* read characters from file_handle */

static long read_in (CATINFO *C, const char *file_name, int file_handle)
{
	long retcode;

	if ((!file_handle) && (C->tty_in))
	{
		long ch = inchar();

		if ((int)ch == 26 || (int)(ch>>16) == UNDO)
			return 0;	/* EOF */

		C->input_buffer[0] = (char)ch;
		return 1;
	}
	
	retcode = Fread (file_handle, C->input_len - C->pending,
		&C->input_buffer[C->pending]);
	
	if (retcode < 0L)	/* GEMDOS error? */
	{
		eprintf ("cat: " CA_CANTREAD "\n", file_name);
		return 0;
	}
	else
	{
		long result = retcode + C->pending;
		C->pending = 0;
		return result;
	}
}

/* write characters to stdout */

static int write_out (CATINFO *C, long amount)
{
	long retcode;

	if (C->tty_out)
	{
		long i;
		
		for (i = 0L; i < amount; i++)
		{
			canout (C->output_buffer[i]);
			if (intr()) return FALSE;
		}
		
		return TRUE;
	}
	
	retcode = Fwrite (1, amount, C->output_buffer);
	
	if (retcode < 0L)
	{
		eprintf ("cat: " CA_CANTWRT "\n");
		return FALSE;
	}
	
	return (retcode == amount);
}


/* do cat from file to stdout, allow empty filename for stdin */

static int cat_file (CATINFO *C, const char *file_name)
{
	int file_handle;
	long n_read;
	int no_conversion = FALSE;
	
	no_conversion = (!C->insert_lf && !C->insert_cr && 
		!C->remove_cr && !C->nonprinting && !C->number);	

	C->pending = 0L;
	C->last_char = 10;		/* line delimiter */
	
	if (*file_name == 0)
		file_handle = 0;
	else
		file_handle = Fopen (file_name, 0);
		
	if (file_handle < MINHND)
	{
		if (!C->silent)
		{
			eprintf ("cat: " CA_CANTOPEN "\n", file_name);
			return 2;	/* fatal */
		}
		else
			return 1;
	}

	write_out (C, 0L);	/* for the first line number, if needed */	

	while (TRUE)
	{
		long converted;
	
		n_read = read_in (C, file_name, file_handle);

		if (!n_read) break;
	
		if (no_conversion)
			memcpy (C->output_buffer, C->input_buffer, 
				converted = n_read);
		else
			converted = convert (C, n_read);

		if (!write_out (C, converted)) break;
	}
	
	if (file_handle) Fclose (file_handle);
	return 0;
}



/* main function for cat */

int m_cat (ARGCV)
{
	GETOPTINFO G;
	CATINFO C;
	
	struct option long_options[] =
	{
		{"show-ends",	0, NULL, 'e'},
		{"insert-cr",	0, NULL, 'i'},
		{"insert-lf",	0, NULL, 'l'},
		{"show-meta",	0, NULL, 'm'},
		{"number",		0, NULL, 'n'},
		{"remove-cr",	0, NULL, 'r'},
		{"silent",		0, NULL, 's'},
		{"show-tabs",	0, NULL, 't'},
		{"show-nonprinting", 0, NULL, 'v'},
		{NULL, 0, 0, 0},
	};

	int opt_index = 0, i, ret = 0;
	char c;
	
	C.tty_in = isatty (0);
	C.tty_out = isatty (1);
	C.silent =
	C.insert_lf =
	C.insert_cr =
	C.remove_cr =
	C.nonprinting =
	C.showtabs =
	C.showends =
	C.number =
	C.showmeta = FALSE;
	C.line_cnt = 0L;
		
	optinit (&G);

	while ((c = getopt_long (&G, argc, argv, "eilmnrstv", long_options,
		&opt_index)) != EOF)
	{
		if (!c)			c = long_options[G.option_index].val;
		switch (c)
		{
			case 0:
				break;
			case 'e':
				C.showends = TRUE;
				break;
			case 'i':
				C.insert_cr = TRUE;
				break;
			case 'l':
				C.insert_lf = TRUE;
				break;
			case 'm':
				C.showmeta = TRUE;
				break;
			case 'n':
				C.number = TRUE;
				break;
			case 'r':
				C.remove_cr = TRUE;
				break;
			case 's':
				C.silent = TRUE;
				break;
			case 't':
				C.showtabs = TRUE;
				break;
			case 'v':
				C.nonprinting = TRUE;
				break;
			default:
				return printusage(long_options);
		}
	}

	if (!C.nonprinting)
		C.showtabs = C.showends = C.showmeta = FALSE;

	alloc_buffers (&C);

	if (argc == G.optind)
	{
		C.insert_lf = C.tty_in;

		if (C.tty_in)
			eprintf ("[" CA_DIRECT "]\n");
			
		ret = cat_file (&C, "");		/* cat stdin */
	}
	else
	{
		for (i = G.optind; (i < argc) && !intr(); i++)
		{
			ret = cat_file (&C, argv[i]);
			
			if (ret == 2)	/* fatal write error? */
				break;
		}
	}
	
	free_buffers (&C);
	return ret;
}
