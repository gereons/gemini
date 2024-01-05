/*
	@(#)Mupfel/test.c
	@(#)S. Eissing, J. Reschke, FSF, 9. Februar 1991
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <tos.h>

#include "attrib.h"
#include "chario.h"
#include "getopt.h"
#include "handle.h"
#include "messages.h"
#include "mupfel.h"

#define R_OK A_READ
#define W_OK A_WRITE
#define X_OK A_EXEC
#define F_OK A_EXIST


/* The following few defines control the truth and false output of each stage.
   TRUE and FALSE are what we use to compute the final output value.
   SHELL_BOOLEAN is the form which returns truth or falseness in shell terms.
   TRUTH_OR is how to do logical or with TRUE and FALSE.
   TRUTH_AND is how to do logical and with TRUE and FALSE..
   Default is TRUE = 1, FALSE = 0, TRUTH_OR = a | b, TRUTH_AND = a & b,
   SHELL_BOOLEAN = (!value). 
 */
#define TRUE 1
#define FALSE 0
#define SHELL_BOOLEAN(value) (!(value))
#define TRUTH_OR(a, b) ((a) | (b))
#define TRUTH_AND(a, b) ((a) & (b))


static int pos;			/* position in list			*/
static int glob_argc;		/* number of args from command line	*/
static char **glob_argv;		/* the argument list			*/

static int
test_syntax_error (char *format, char *arg)
{
	eprintf ("%s: ", glob_argv[0]);
	eprintf (format, arg);
	return 1;
}

/*
 * beyond - call when we're beyond the end of the argument list (an
 *	error condition)
 */

static
int beyond (void)
{
	test_syntax_error (TE_ARGEXP "\n", (char *)NULL);
	return 1; /* dummy for typechecking (se) */
}

/*
 * advance - increment our position in the argument list.  Check that
 *	we're not past the end of the argument list.  This check is
 *	supressed if the argument is FALSE.  made a macro for efficiency.
 */

#ifndef lint
#define advance(f)	(++pos, f && (pos < glob_argc ? 0 : beyond()))
#endif

#if !defined(advance)
static int advance (int f)
{
	++pos;
	if (f && pos >= glob_argc)
		beyond ();
}
#endif

#define unary_advance() (advance (1),++pos)

/*
 * int_expt_err - when an integer argument was expected, but something else
 * 	was found.
 */

static void
int_expt_err (char *pch)
{
	test_syntax_error (TE_INTEXP "\n", pch);
}

/*
 * isint - is the argument whose position in the argument vector is 'm' an
 * 	integer? Convert it for me too, returning it's value in 'pl'.
 */

static int
isint (int m, long *pl)
{
	char *pch;

	pch = glob_argv[m];

	/* Skip leading whitespace characters. */
	while (*pch == '\t' || *pch == ' ')
		pch++;
	
	/* accept negative numbers but not '-' alone */
	if ('-' == *pch)
		if ('\000' == *++pch)
	  		return 0;
	
	while ('\000' != *pch)
	{
		switch (*pch)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				break;
			default:
				return (FALSE);
		}
		++pch;
	}
	*pl = atol (glob_argv[m]);
	
	return (TRUE);
}

/*
 * age_of - find the age of the given file.  Return YES or NO depending
 *	on whether the file exists, and if it does, fill in the age with
 *	the modify time.
 */
static int
age_of (int posit, long *age)
{
	DTA *oldDTA, myDTA;

	oldDTA = Fgetdta ();
	Fsetdta (&myDTA);

	if (Fsfirst (glob_argv[posit], FA_ATTRIB))
	{
		Fsetdta (oldDTA);
		return (FALSE);
	}
	
	((int *)age)[0] = myDTA.d_date;
	((int *)age)[1] = myDTA.d_time;

	Fsetdta (oldDTA);
	return (TRUE);
}

/*
 * term - parse a term and return 1 or 0 depending on whether the term
 *	evaluates to true or false, respectively.
 *
 * term ::=
 *	'-'('h'|'d'|'f'|'r'|'s'|'w'|'c'|'b'|'p'|'u'|'g'|'k') filename
 *	'-'('L'|'x') filename
 * 	'-t' [ int ]
 *	'-'('z'|'n') string
 *	string
 *	string ('!='|'=') string
 *	<int> '-'(eq|ne|le|lt|ge|gt) <int>
 *	file '-'(nt|ot|ef) file
 *	'(' <expr> ')'
 * int ::=
 *	'-l' string
 *	positive and negative integers
 */

static int
term (void)
{
	int expr (void);
	long int l, r;
	int l_is_l, r_is_l;	/* are the left and right integer
						 * expressions of the form '-l string'?
		   				 */
	int value, fd;

	if (pos >= glob_argc)
		beyond ();
	
	/* Deal with leading "not"'s. */
	while (pos < glob_argc && '!' == glob_argv[pos][0] && '\000' == glob_argv[pos][1])
	{
		advance (1);
		
		/* This has to be rewritten to handle the TRUTH and FALSE stuff. */
		return (!term ());
	}
	
	if ('(' == glob_argv[pos][0] && '\000' == glob_argv[pos][1])
	{
		advance (1);
		value = expr ();
		if (')' != glob_argv[pos][0] || '\000' != glob_argv[pos][1])
			test_syntax_error (TE_ARGEXPF "\n", glob_argv[pos]);
		advance (0);
		return (TRUE == (value));
	}

	/* are there enough arguments left that this could be dyadic? */
	if (pos + 3 <= glob_argc)
	{
		int op;

		if ('-' == glob_argv[pos][0] && 'l' == glob_argv[pos][1] 
			&& '\000' == glob_argv[pos][2])
		{
			l_is_l = 1;
			op = pos + 2;
			advance (0);
		}
		else
		{
			l_is_l = 0;
			op = pos + 1;
		}
		
		if ('-' == glob_argv[op + 1][0] && 'l' == glob_argv[op + 1][1]
			&& '\000' == glob_argv[op + 1][2])
		{
			r_is_l = 1;
			advance (0);
		}
		else
			r_is_l = 0;
	
		if ('-' == glob_argv[op][0])
		{
			/* check for eq, nt, and stuff */
			switch (glob_argv[op][1])
			{
				default:
					break;
				case 'l':
					if ('t' == glob_argv[op][2] && '\000' == glob_argv[op][3])
					{
						/* lt */
						if (l_is_l)
						    l = strlen (glob_argv[op - 1]);
						else
						{
							if (!isint (op - 1, &l))
								int_expt_err (TE_BEFORE " -lt");
						}
	
						if (r_is_l)
							r = strlen (glob_argv[op + 2]);
						else
						{
							if (!isint (op + 1, &r))
								int_expt_err (TE_AFTER " -lt");
						}
						pos += 3;
						return (TRUE == (l < r));
					}
	
					if ('e' == glob_argv[op][2] && '\000' == glob_argv[op][3])
					{
						/* le */
						if (l_is_l)
							l = strlen (glob_argv[op - 1]);
						else
						{
							if (!isint (op - 1, &l))
								int_expt_err (TE_BEFORE " -le");
						}

						if (r_is_l)
							r = strlen (glob_argv[op + 2]);
						else
						{
							if (!isint (op + 1, &r))
								int_expt_err (TE_AFTER " -le");
						}
						pos += 3;
						return (TRUE == (l <= r));
					}
					break;
	
				case 'g':
					if ('t' == glob_argv[op][2] && '\000' == glob_argv[op][3])
					{
						/* gt integer greater than */
						if (l_is_l)
							l = strlen (glob_argv[op - 1]);
						else
						{
							if (!isint (op - 1, &l))
							int_expt_err (TE_BEFORE " -gt");
						}
						
						if (r_is_l)
							r = strlen (glob_argv[op + 2]);
						else
						{
							if (!isint (op + 1, &r))
								int_expt_err (TE_AFTER " -gt");
						}
						pos += 3;
						return (TRUE == (l > r));
					}
	
					if ('e' == glob_argv[op][2] && '\000' == glob_argv[op][3])
					{
						/* ge - integer greater than or equal to */
						if (l_is_l)
							l = strlen (glob_argv[op - 1]);
						else
						{
							if (!isint (op - 1, &l))
								int_expt_err (TE_BEFORE " -ge");
						}
						
						if (r_is_l)
							r = strlen (glob_argv[op + 2]);
						else
						{
							if (!isint (op + 1, &r))
								int_expt_err (TE_AFTER " -ge");
						}
						pos += 3;
						return (TRUE == (l >= r));
					}
					break;
	
				case 'n':
					if ('t' == glob_argv[op][2] && '\000' == glob_argv[op][3])
					{
						/* nt - newer than */
						pos += 3;
						if (l_is_l || r_is_l)
							test_syntax_error (TE_NTL "\n", (char *)NULL);

						if (age_of (op - 1, &l) && age_of (op + 1, &r))
							return (TRUE == (l > r));
						else
							return (FALSE);
					}
	
					if ('e' == glob_argv[op][2] && '\000' == glob_argv[op][3])
					{
						/* ne - integer not equal */
						if (l_is_l)
							l = strlen (glob_argv[op - 1]);
						else
						{
							if (!isint (op - 1, &l))
								int_expt_err (TE_BEFORE " -ne");
						}
						
						if (r_is_l)
							r = strlen (glob_argv[op + 2]);
						else
						{
							if (!isint (op + 1, &r))
								int_expt_err (TE_AFTER " -ne");
						}
						pos += 3;
						return (TRUE == (l != r));
					}
					break;

				case 'e':
					if ('q' == glob_argv[op][2] && '\000' == glob_argv[op][3])
					{
						/* eq - integer equal */
						if (l_is_l)
							l = strlen (glob_argv[op - 1]);
						else
						{
							if (!isint (op - 1, &l))
								int_expt_err (TE_BEFORE " -eq");
						}
						if (r_is_l)
							r = strlen (glob_argv[op + 2]);
						else
						{
							if (!isint (op + 1, &r))
								int_expt_err (TE_AFTER " -eq");
						}
						pos += 3;
						return (TRUE == (l == r));
					}
					break;
	
				case 'o':
					if ('t' == glob_argv[op][2] && '\000' == glob_argv[op][3])
					{
						/* ot - older than */
						pos += 3;
						if (l_is_l || r_is_l)
						{
							test_syntax_error (TE_NTL "\n",
								(char *)NULL);
						}

						if (age_of (op - 1, &l) && age_of (op + 1, &r))
							return (TRUE == (l < r));
						
						return (FALSE);
					}
					break;
			}
		}
	
		if ('=' == glob_argv[op][0] && '\000' == glob_argv[op][1])
		{
			value = (0 == strcmp (glob_argv[pos], glob_argv[pos + 2]));
			pos += 3;
			return (TRUE == value);
		}

		if ('!' == glob_argv[op][0] && '=' == glob_argv[op][1] && '\000' == glob_argv[op][2])
		{
			value = 0 != strcmp (glob_argv[pos], glob_argv[pos + 2]);
			pos += 3;
			return (TRUE == value);
		}
	}
	
	/* Might be a switch type argument */
	if ('-' == glob_argv[pos][0] && '\000' == glob_argv[pos][2] /* && pos < glob_argc-1 */ )
	{
		switch (glob_argv[pos][1])
		{
			default:
				break;

			/* All of the following unary operators use 
			 * unary_advance (), which checks to make sure that there 
			 * is an argument, and then advances pos right past it. 
			 * This means that pos - 1 is the location of the argument.
			 */

			case 'r':		/* file is readable? */
				unary_advance ();
				value = access (glob_argv[pos - 1], R_OK)? TRUE : FALSE;
				return (TRUE == value);

			case 'w':		/* File is writeable? */
				unary_advance ();
				value = access (glob_argv[pos - 1], W_OK)? TRUE : FALSE;
				return (TRUE == value);

			case 'x':		/* File is executable? */
				unary_advance ();
				value = access (strupr(glob_argv[pos - 1]), X_OK)? TRUE : FALSE;
				return (TRUE == value);

			case 'O':		/* File is owned by you? */
				unary_advance ();
				value = access (glob_argv[pos - 1], A_EXIST);
				return (TRUE == value);

			case 'f':		/* File is a file? */
				unary_advance ();
				value = Fopen (glob_argv[pos-1], 0);
				if (value < MINHND)
					return (FALSE);
				else
				{
					Fclose (value);
					return (TRUE);
				}

			case 'd':		/* File is a directory? */
				unary_advance ();
				return (isdir (glob_argv[pos - 1]));

			case 's':		/* File has something in it? */
				{
					DTA myDTA, *oldDTA = Fgetdta ();
					int rc;
					
					unary_advance ();
					Fsetdta (&myDTA);
					rc = Fsfirst (glob_argv[pos - 1], FA_ATTRIB);
					Fsetdta (oldDTA);
					
					if (rc)
						return FALSE;
					else
						return (myDTA.d_length > 0L);
				}
				
			case 'S':		/* File is a socket? */
				unary_advance ();
				return (FALSE);

			case 'c': /* File is character special? */
				unary_advance ();
				if ((fd = Fopen(glob_argv[pos - 1], 0)) >= MINHND)
				{
					value = (TRUE == (isatty (fd) != 0));
					Fclose(fd);
				}
				else
					value = (FALSE);
				
				return (value);

			case 'b':		/* File is block special? */
				unary_advance ();
				return (FALSE);

			case 'p':		/* File is a named pipe? */
				unary_advance ();
				return (FALSE);

			case 'L':		/* Same as -h  */
			case 'h':		/* File is a symbolic link? */
				unary_advance ();
				return (FALSE);

			case 'u':		/* File is setuid? */
				unary_advance ();
				return (FALSE);

			case 'g':		/* File is setgid? */
				unary_advance ();
				return (FALSE);

			case 'k':		/* File has sticky bit set? */
				unary_advance ();
				return (FALSE);

			case 't':		/* File (fd) is a terminal?  
							 * (fd) defaults to stdout. 
							 */
				advance (0);
				if (pos < glob_argc && isint (pos, &r))
				{
					advance (0);
					return (TRUE == (isatty ((int) r) != 0));
				}
				return (TRUE == (isatty (stdin->Handle) != 0));

			case 'n':		/* True if arg has some length. */
				++pos;
				if (pos < glob_argc)
				{
					++pos;
					return (TRUE == (0 != strlen (glob_argv[pos - 1])));
				}
				else
				{	/* kein Argument: wir gehen davon aus, daž
				     * das Argument durch ARGV verschluckt wurde.
				     */
					return (FALSE);
				}

			case 'z':		/* True if arg has no length. */
				++pos;
				if (pos < glob_argc)
				{
					++pos;
					return (TRUE == (0 == strlen (glob_argv[pos - 1])));
				}
				else
				{	/* kein Argument: wir gehen davon aus, daž
				     * das Argument durch ARGV verschluckt wurde.
				     */
					return (TRUE);
				}
		}
	}
	value = 0 != strlen (glob_argv[pos]);
	advance (0);
	return value;
}

/*
 * and:
 *	and '-a' term
 *	term
 */
static int
and (void)
{
	int value;

	value = term ();
	while (pos < glob_argc && '-' == glob_argv[pos][0] 
			&& 'a' == glob_argv[pos][1] && '\000' == glob_argv[pos][2])
	{
		advance (0);
		value = TRUTH_AND (value, term ());
	}
	return (TRUE == value);
}

/*
 * or:
 *	or '-o' and
 *	and
 */
static
int or (void)
{
	int value;

	value = and ();
	while (pos < glob_argc && '-' == glob_argv[pos][0] 
			&& 'o' == glob_argv[pos][1] && '\000' == glob_argv[pos][2])
	{
		advance (0);
		value = TRUTH_OR (value, and ());
	}
	return (TRUE == value);
}

/*
 * expr:
 *	or
 */
static int
expr (void)
{
	int value;

	if (pos >= glob_argc)
		beyond ();

	value = FALSE;

	return value ^ or ();		/* Same with this. */
}

/*
 * [:
 *	'[' expr ']'
 * test:
 *	test expr
 */
int
m_test (ARGCV)
{
	int value;
	int expr (void);

	glob_argv = argv;
	
	if (!strcmp (argv[0], "["))
	{
		--argc;

		if (argv[argc] && strcmp (argv[argc], "]"))
			return test_syntax_error (TE_MISSBR "\n", (char *)NULL);

		if (argc < 2)
			return 1;
	}

	glob_argc = argc;
	pos = 1;

	if (pos >= glob_argc)
		return 1;

	value = expr ();
	if (pos != glob_argc)
	{
		eprintf ("%s: " TE_TOOMANY "\n", argv[0]);
	}

	return (SHELL_BOOLEAN (value));
}
