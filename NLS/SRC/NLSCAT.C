/*
	@(#)Nls/nlscat.c
	@(#)Stefan Eissing, 23. Januar 1991
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt.h"
#include "stderr.h"

#include "nlsdef.h"

#ifndef TRUE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

#define MAXLEN	4096


char *sccsid (void)
{
	return "@(#)Nls/nlscat.ttp, Copyright (c) Stefan Eissing, "__DATE__;
}


char *progname;
static int verbose = 0;
static int done, retcode;
static long stringCount = 0;
static char line[MAXLEN+1];

void usage (void)
{
	fprintf (stderr, "usage: %s [-v] infile ...\n", progname);
	exit (2);
}

static void printSection(char *line)
{
	char *firstq, *lastq;
	
	printf("Section:");
	firstq = strchr(line, '\"');
	lastq = strrchr(line, '\"');
	
	if (firstq && lastq)
	{
		*lastq = '\0';
		printf("%s\n", firstq+1);
	}
	else
		printf("\n");
}

/*
 * ersetzt die Escapesequenzen \[ und \] durch C-Kommentar-
 * anfang und -ende
 */
static void replaceEscapes(char *line)
{
	char *back;

	while((back = strchr(line, '\\')) != NULL)
	{
		switch (back[1])
		{
			case '\0':
				line = back + 1;
				break;

			case '[':
				back[0] = '/';
				back[1] = '*';
				line = back + 2;
				break;
				
			case ']':
				back[0] = '*';
				back[1] = '/';
				line = back + 2;
				break;

			default:
				line = back + 2;
				break;
		}
	}
}

static int printString(char *line, int wasInString)
{
	char *comment;
	int nowInString = TRUE;
	
	/* Hatten wir einen Kommentaranfang? */
	if (wasInString)
	{
		comment = strstr(line, "*/");
		if (comment)
		{
			nowInString = FALSE;
			*comment = '\0';
			replaceEscapes(line);
			printf("%ld:%s\n", stringCount, line);
			++stringCount;
		}
		else
		{
			if ((comment = strchr(line, '\n')) != NULL)
				*comment = '\0';
			replaceEscapes(line);
			printf("%ld:%s\n", stringCount, line);
		}
	}
	else
	{
		comment = strstr(line, "/*");
		if (!comment)
			return FALSE;

		nowInString = printString(comment+2, TRUE);
	}
	
	return nowInString;
}

void copyNlsFile(FILE *fp)
{
	while (fgets(line, MAXLEN, fp))
		printf("%s", line);
}

static int convert(const char *filename)
{
	FILE *fp;
	int inSection = FALSE;
	int inEnum = FALSE;
	int inString = FALSE;
	int needString = FALSE;
	char *token;
	int firstline = TRUE;
	
	if (verbose)
		fprintf(stderr, "%s: converting %s...", progname, filename);

	if ((fp = fopen(filename, "r")) == NULL)
	{
		fprintf(stderr, "%s: WARNING: cannot open %s!\n", progname, filename);
		return TRUE;
	}
	
	done = FALSE;
	retcode = TRUE;
	
	while (!done && fgets(line, MAXLEN, fp))
	{
		if (firstline)
		{
			firstline = FALSE;
			if (!strncmp(line, NLS_TEXT_MAGIC, strlen(NLS_TEXT_MAGIC)))
			{
				copyNlsFile(fp);
				done = TRUE;
				break;
			}
		}
		
		/* Bearbeiten wir die enum-Definition? */
		if (!inEnum)
		{
			token = strtok(line, " \t\n");
			
			/* kein enum, haben wir ein Section-Definition? */
			if (token && !strcmp(token, "#define"))
			{
				token = strtok(NULL, " \t\n");
				if (token && !strcmp(token, SECTION_MAGIC))
				{
					printSection(strtok(NULL, "\n"));
					inSection = TRUE;
				}
			}
			else if (token && !strcmp(token, "enum"))
			{
				token = strtok(NULL, " {\t\n");
				
				/* Haben wir DIE enum-Definition? */
				if (token && !strcmp(token, ENUM_MAGIC))
				{
					/* Haben wir schon eine Section? */
					if (!inSection)
					{
						fprintf(stderr, 
						"\nNlsLocalText without NlsLocalSection!\n");
						done = TRUE;
						retcode = FALSE;
					}
					else
					{
						stringCount = 0L;
						needString = FALSE;
						inEnum = TRUE;
					}
				}
			}
		}
		else
		{
			/* Wir sind in der enum-Defintion*/
			/* Sind wir beim Parsen eines Strings?*/
			if (inString)
			{
				inString = printString(line, inString);
			}
			else
			{
				if (!strlen(line) || (*line == '\n'))
					continue;
				
				/* brauchen wir einen String (nach enumwort) */
				if (needString)
				{
					if (!strstr(line, "/*"))
					{
						fprintf(stderr, "Needed string for enum\n");
						done = TRUE;
						retcode = FALSE;
					}
					else
					{
						inString = printString(line, inString);
						needString = FALSE;
					}
				}
				else
				{
					token = strtok(line, " ,\t\n");
					
					/* Haben wir ein enum-Wort? */
					if (!token)
						continue;
					
					if (strchr(token, '}'))
					{
						inSection = inEnum = FALSE;
						stringCount = 0L;
						done = TRUE;
					}
					else
					{
						token = strtok(NULL, "\n");

						/* Haben wir einen String? */
						if (token)
							inString = printString(token, inString);
						else
							needString = TRUE;
					}
				}
			}
		}
	}
	
	if (retcode)
	{
		if (needString)
		{
			fprintf(stderr, "need a string badly!\n");
			retcode = FALSE;
		}
		else if (inSection && !inEnum)
		{
			fprintf(stderr, "no strings in this section!\n");
			retcode = FALSE;
		}
		else if (inEnum)
		{
			fprintf(stderr, "enum definition not complete!\n");
			retcode = FALSE;
		}
	}
	
	fclose(fp);
	if (verbose)
		fprintf(stderr, " done.\n");
	return retcode;
}

int argvmain (int argc, char *argv[])
{
	int c, option_index, i;
	static int giveHelp = 0;
	static struct option long_options[]=
	{
		{"help", 0, &giveHelp, 1},
		{"verbose", 0, &verbose, 1},
		{ 0, 0, 0, 0}
	};
	patchstderr ();

	if ((!argv[0]) || (!argv[0][0])) argv[0] = "nlscat";
	progname = argv[0];

	while ((c = getopt_long(argc, argv, "hv", long_options, 
				&option_index)) != EOF)
		switch (c)
		{
			case 0:
				break;
		
			case 'h':
				giveHelp = 1;
				break;
			
			case 'v':
				verbose = 1;
				break;
					
			default :
				usage();
		}

	if (giveHelp || (optind >= argc))
		usage();

	printf("%s\n", NLS_TEXT_MAGIC);

	if ((optind == argc-1) && (!strcmp(argv[optind],"-")))
	{
		char filename[1025];
		
		while (fgets(filename, 1024, stdin))
		{
			filename[1024] = '\0';
			if (filename[strlen(filename)-1] == '\n')
				filename[strlen(filename)-1] = '\0';
			
			if (!convert(filename))
				return 1;
		}
	}
	else
	{
		for (i = optind; i < argc; ++i)
		{
			if (!convert(argv[i]))
				return 1;
		}
	}
	
	return 0;
}

