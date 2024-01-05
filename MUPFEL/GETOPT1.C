/*
 * getopt1.c  -  long argument getopt
 *
 */

#include "getopt.h"

int getopt_long (GETOPTINFO *G, int argc, char **argv, const char *options,
	const struct option *long_options, int *opt_index)
{
  int val;

  G->_getopt_long_options = long_options;
  val = getopt (G, argc, argv, options);
  if (val == 0)
    *opt_index = G->option_index;
  return val;
}

/* Like getopt_long, but '-' as well as '+' can indicate a long option.
   If an option that starts with '-' doesn't match a long option,
   but does match a short option, it is parsed as a short option
   instead. */

int
getopt_long_only (GETOPTINFO *G, int argc, char **argv,
	const char *options, const struct option *long_options,
	int *opt_index)
{
  int val;

  G->_getopt_long_options = long_options;
  G->_getopt_long_only = 1;
  val = getopt (G, argc, argv, options);
  if (val == 0)
    *opt_index = G->option_index;
  return val;
}
     
#ifdef TEST

#include <stdio.h>

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
      int option_index = 0;
      static struct option long_options[]
	= {{ "add", 1, 0, 0 },
	   { "append", 0, 0, 0 },
	   { "delete", 1, 0, 0 },
	   { "verbose", 0, 0, 0 },
	   { "create", 0, 0, 0 },
	   { "file", 1, 0, 0 },
	   { 0, 0, 0, 0}};

      c = getopt_long (argc, argv, "abc:d:0123456789",
		       long_options, &option_index);
      if (c == EOF)
	break;
	switch (c)
	  {
	  case 0:
	    printf ("option %s", (long_options[option_index]).name);
	    if (optarg)
	      printf (" with arg %s", optarg);
	    printf ("\n");
	    break;

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
