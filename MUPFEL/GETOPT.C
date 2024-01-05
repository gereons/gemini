/* This version of `getopt' appears to the caller like standard Unix `getopt'
   but it behaves differently for the user, since it allows the user
   to intersperse the options with the other arguments.

   As `getopt' works, it permutes the elements of `argv' so that,
   when it is done, all the options precede everything else.  Thus
   all application programs are extended to handle flexible argument order.

   Setting the environment variable _POSIX_OPTION_ORDER disables permutation.
   Then the behavior is completely standard.

   GNU application programs can use a third alternative mode in which
   they can distinguish the relative order of options and other arguments.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chario.h"
#include "getopt.h"
#include "mupfel.h"

#define bcopy(s, d, n) memcpy ((d), (s), (n))
#define index strchr

/* Describe how to deal with options that follow non-option ARGV-elements.

   If the caller did not specify anything,
   the default is REQUIRE_ORDER if the environment variable
   _POSIX_OPTION_ORDER is defined, PERMUTE otherwise.

   REQUIRE_ORDER means don't recognize them as options.
   Stop option processing when the first non-option is seen.
   This is what Unix does.

   PERMUTE is the default.  We permute the contents of ARGV as we scan,
   so that eventually all the options are at the end.  This allows options
   to be given in any order, even with programs that were not written to
   expect this.

   RETURN_IN_ORDER is an option available to programs that were written
   to expect options and other ARGV-elements in any order and that care about
   the ordering of the two.  We describe each non-option ARGV-element
   as if it were the argument of an option with character code one.
   Using `-' as the first character of the list of option characters
   requests this mode of operation.

   The special argument `--' forces an end of option-scanning regardless
   of the value of `ordering'.  In the case of RETURN_IN_ORDER, only
   `--' can cause `getopt' to return EOF with `optind' != ARGC.  */

static enum { REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER } ordering;

/* Describe the long-named options requested by the application.
   _GETOPT_LONG_OPTIONS is a vector of `struct option' terminated by an
   element containing a name which is zero.
   The field `has_arg' is 1 if the option takes an argument, 
   2 if it takes an optional argument.  */
/*
struct option
{
  char *name;
  int has_arg;
  int *flag;
  int val;
};
*/


/* Exchange two adjacent subsequences of ARGV.
   One subsequence is elements [first_nonopt,last_nonopt)
    which contains all the non-options that have been skipped so far.
   The other is elements [last_nonopt,optind), which contains all
    the options processed since those non-options were skipped.

   `first_nonopt' and `last_nonopt' are relocated so that they describe
    the new indices of the non-options in ARGV after they are moved.  */

static void exchange (GETOPTINFO *G, char **argv)
{
  size_t nonopts_size
    = (G->last_nonopt - G->first_nonopt) * sizeof (char *);
  char **temp = (char **) malloc (nonopts_size);

  /* Interchange the two blocks of data in argv.  */

  bcopy (&argv[G->first_nonopt], temp, nonopts_size);
  bcopy (&argv[G->last_nonopt], &argv[G->first_nonopt],
	 (G->optind - G->last_nonopt) * sizeof (char *));
  bcopy (temp, &argv[G->first_nonopt + G->optind - G->last_nonopt],
	 nonopts_size);

  /* Update records for the slots the non-options now occupy.  */

  G->first_nonopt += (G->optind - G->last_nonopt);
  G->last_nonopt = G->optind;

  free (temp);
}

/* Scan elements of ARGV (whose length is ARGC) for option characters
   given in OPTSTRING.

   If an element of ARGV starts with '-', and is not exactly "-" or "--",
   then it is an option element.  The characters of this element
   (aside from the initial '-') are option characters.  If `getopt'
   is called repeatedly, it returns successively each of the option characters
   from each of the option elements.

   If `getopt' finds another option character, it returns that character,
   updating `optind' and `nextchar' so that the next call to `getopt' can
   resume the scan with the following option character or ARGV-element.

   If there are no more option characters, `getopt' returns `EOF'.
   Then `optind' is the index in ARGV of the first ARGV-element
   that is not an option.  (The ARGV-elements have been permuted
   so that those that are not options now come last.)

   OPTSTRING is a string containing the legitimate option characters.
   If an option character is seen that is not listed in OPTSTRING,
   return '?' after printing an error message.  If you set `opterr' to
   zero, the error message is suppressed but we still return '?'.

   If a char in OPTSTRING is followed by a colon, that means it wants an arg,
   so the following text in the same ARGV-element, or the text of the following
   ARGV-element, is returned in `optarg'.  Two colons mean an option that
   wants an optional arg; if there is text in the current ARGV-element,
   it is returned in `optarg', otherwise `optarg' is set to zero.

   If OPTSTRING starts with `-', it requests a different method of handling the
   non-option ARGV-elements.  See the comments about RETURN_IN_ORDER, above.

   Long-named options begin with `+' instead of `-'.
   Their names may be abbreviated as long as the abbreviation is unique
   or is an exact match for some defined option.  If they have an
   argument, it follows the option name in the same ARGV-element, separated
   from the option name by a `=', or else the in next ARGV-element.
   `getopt' returns 0 when it finds a long-named option.  */

int getopt (GETOPTINFO *G, int argc, char **argv, const char *optstring)
{
  G->optarg = 0;

  /* Initialize the internal data when the first call is made.
     Start processing options with ARGV-element 1 (since ARGV-element 0
     is the program name); the sequence of previously skipped
     non-option ARGV-elements is empty.  */

  if (G->optind == 0)
    {
      G->first_nonopt = G->last_nonopt = G->optind = 1;

      G->nextchar = 0;

      /* Determine how to handle the ordering of options and nonoptions.  */

      if (optstring[0] == '-')
	ordering = RETURN_IN_ORDER;
      else if (getenv ("_POSIX_OPTION_ORDER"))
	ordering = REQUIRE_ORDER;
      else
	ordering = PERMUTE;
    }

  if (G->nextchar == 0 || *G->nextchar == 0)
    {
      if (ordering == PERMUTE)
	{
	  /* If we have just processed some options following some non-options,
	     exchange them so that the options come first.  */

	  if (G->first_nonopt != G->last_nonopt && G->last_nonopt != G->optind)
	    exchange (G, argv);
	  else if (G->last_nonopt != G->optind)
	    G->first_nonopt = G->optind;

	  /* Now skip any additional non-options
	     and extend the range of non-options previously skipped.  */

	  while (G->optind < argc
		 && (argv[G->optind][0] != '-'
		     || argv[G->optind][1] == 0)
		 && (G->_getopt_long_options == 0
		     || argv[G->optind][0] != '+'
		     || argv[G->optind][1] == 0))
	    G->optind++;
	  G->last_nonopt = G->optind;
	}

      /* Special ARGV-element `--' means premature end of options.
	 Skip it like a null option,
	 then exchange with previous non-options as if it were an option,
	 then skip everything else like a non-option.  */

      if (G->optind != argc && !strcmp (argv[G->optind], "--"))
	{
	  G->optind++;

	  if (G->first_nonopt != G->last_nonopt && G->last_nonopt != G->optind)
	    exchange (G, argv);
	  else if (G->first_nonopt == G->last_nonopt)
	    G->first_nonopt = G->optind;
	  G->last_nonopt = argc;

	  G->optind = argc;
	}

      /* If we have done all the ARGV-elements, stop the scan
	 and back over any non-options that we skipped and permuted.  */

      if (G->optind == argc)
	{
	  /* Set the next-arg-index to point at the non-options
	     that we previously skipped, so the caller will digest them.  */
	  if (G->first_nonopt != G->last_nonopt)
	    G->optind = G->first_nonopt;
	  return EOF;
	}
	 
      /* If we have come to a non-option and did not permute it,
	 either stop the scan or describe it to the caller and pass it by.  */

      if ((argv[G->optind][0] != '-' || argv[G->optind][1] == 0)
	  && (G->_getopt_long_options == 0
	      || argv[G->optind][0] != '+' || argv[G->optind][1] == 0))
	{
	  if (ordering == REQUIRE_ORDER)
	    return EOF;
	  G->optarg = argv[G->optind++];
	  return 1;
	}

      /* We have found another option-ARGV-element.
	 Start decoding its characters.  */

      G->nextchar = argv[G->optind] + 1;
    }

  if (G->_getopt_long_options != 0
      && (argv[G->optind][0] == '+'
	  || (G->_getopt_long_only && argv[G->optind][0] == '-'))
      )
    {
      const struct option *p;
      char *s = G->nextchar;
      int exact = 0;
      int ambig = 0;
      const struct option *pfound = 0;
      int indfound;

      while (*s && *s != '=') s++;

      /* Test all options for either exact match or abbreviated matches.  */
      for (p = G->_getopt_long_options, G->option_index = 0; p->name; 
	   p++, G->option_index++)
	if (!strncmp (p->name, G->nextchar, s - G->nextchar))
	  {
	    if (s - G->nextchar == strlen (p->name))
	      {
		/* Exact match found.  */
		pfound = p;
		indfound = G->option_index;
		exact = 1;
		break;
	      }
	    else if (pfound == 0)
	      {
		/* First nonexact match found.  */
		pfound = p;
		indfound = G->option_index;
	      }
	    else
	      /* Second nonexact match found.  */
	      ambig = 1;
	  }

      if (ambig && !exact)
	{
	  eprintf( GO_AMBIG "\n",
		   argv[0], argv[G->optind]);
	  G->nextchar += strlen (G->nextchar);			   
	  return '?';
	}

      if (pfound != 0)
	{
	  G->option_index = indfound;
	  G->optind++;
	  if (*s)
	    {
	      if (pfound->has_arg > 0)
		G->optarg = s + 1;
	      else
		{
		  eprintf(GO_NOARG "\n",
		  	argv[0], argv[G->optind - 1][0], pfound->name);
		  G->nextchar += strlen (G->nextchar);			   
		  return '?';
		}
	    }
	  else if (pfound->has_arg == 1)
	    {
	      if (G->optind < argc)
		G->optarg = argv[G->optind++];
	      else
		{
		  eprintf(GO_REQARG "\n",
			   argv[0], argv[G->optind - 1]);
		  G->nextchar += strlen (G->nextchar);		   
		  return '?';
		}
	    }
	  G->nextchar += strlen (G->nextchar);
	  if (pfound->flag)
	    *(pfound->flag) = pfound->val;
	  return 0;
	}
      /* Can't find it as a long option.  If this is getopt_long_only,
	 and the option starts with '-' and is a valid short
	 option, then interpret it as a short option.  Otherwise it's
	 an error.  */
      if (G->_getopt_long_only == 0 || argv[G->optind][0] == '+' ||
	  index (optstring, *G->nextchar) == 0)
	{
	  if (G->opterr != 0)
	    eprintf( GO_ILLOPT "\n",
		     argv[0], argv[G->optind][0], G->nextchar);
	  G->nextchar += strlen (G->nextchar);		   
	  return '?';
	}
    }
 
  /* Look at and handle the next option-character.  */

  {
    char c = *G->nextchar++;
    char *temp = index (optstring, c);

    /* Increment `optind' when we start to process its last character.  */
    if (*G->nextchar == 0)
      G->optind++;

    if (temp == 0 || c == ':')
      {
	if (G->opterr != 0)
	  {
	    if (c < 040 || c >= 0177)
	      eprintf( GO_ILLOPT2 "\n", argv[0], c);
	    else
	      eprintf( GO_ILLOPT3 "\n",
		       argv[0], c);
	  }
	return '?';
      }
    if (temp[1] == ':')
      {
	if (temp[2] == ':')
	  {
	    /* This is an option that accepts an argument optionally.  */
	    if (*G->nextchar != 0)
	      {
	        G->optarg = G->nextchar;
		G->optind++;
	      }
	    else
	      G->optarg = 0;
	    G->nextchar = 0;
	  }
	else
	  {
	    /* This is an option that requires an argument.  */
	    if (*G->nextchar != 0)
	      {
		G->optarg = G->nextchar;
		/* If we end this ARGV-element by taking the rest as an arg,
		   we must advance to the next element now.  */
		G->optind++;
	      }
	    else if (G->optind == argc)
	      {
		if (G->opterr != 0)
		  eprintf( GO_REQARG2 "\n",
			   argv[0], c);
		c = '?';
	      }
	    else
	      /* We already incremented `optind' once;
		 increment it again when taking next ARGV-elt as argument.  */
	      G->optarg = argv[G->optind++];
	    G->nextchar = 0;
	  }
      }
    return c;
  }
}

void optinit (GETOPTINFO *G)
{
	G->optarg = G->nextchar = NULL;
	G->optind = 0;
	G->opterr = 1;
	G->_getopt_long_only = 0;
	G->_getopt_long_options = NULL;
}

#ifdef TEST

/* Compile with -DTEST to make an executable for use in testing
   the above definition of `getopt'.  */

int
argvmain (argc, argv)
     int argc;
     char **argv;
{
  char c;
  int digit_optind = 0;

  while (1)
    {
      int this_option_optind = optind;
      if ((c = getopt (argc, argv, "abc:d:0123456789")) == EOF)
	break;

      switch (c)
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
	  if (digit_optind != 0 && digit_optind != this_option_optind)
	    printf ("digits occur in two different argv-elements.\n");
	  digit_optind = this_option_optind;
	  printf ("option %c\n", c);
	  break;

	case 'a':
	  printf ("option a\n");
	  break;

	case 'b':
	  printf ("option b\n");
	  break;

	case 'c':
	  printf ("option c with value `%s'\n", optarg);
	  break;

	case '?':
	  break;

	default:
	  printf ("?? getopt returned character code 0%o ??\n", c);
	}
    }

  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
	printf ("%s ", argv[optind++]);
      printf ("\n");
    }

  return 0;
}

#endif /* TEST */
