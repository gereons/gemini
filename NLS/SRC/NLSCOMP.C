/*
	@(#)Nls/nlscomp.c
	@(#)Stefan Eissing, 16. Januar 1991
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
	return "@(#)Nls/nlscomp.ttp, Copyright (c) Stefan Eissing, "__DATE__;
}

/* Name des Programms */
char *progname;

/* Name des Eingabefiles */
char *infilename;

/* Name des Ausgabefiles */
char *outfilename;

/* FilePointer dazu */
FILE *pfin, *pfout;

/* Flag fÅr die GeschwÑtzigkeit */
static int verbose = 0;

/* eingelesene Zeile aus der Eingabedatei */
static char line[MAXLEN+1];

/* Nummer der gerade eingelesenen Zeile */
static long linecount = 0;

/* aktueller Offset im Ausgabefile */
static long fileoffset = 0;

/* Sektion, die gerade bearbeitet wird */
static BINSECTION Section;

void usage (void)
{
	fprintf (stderr, "usage: %s [-v] infile outfile\n", progname);
	exit (2);
}

static char *getLine(char *line)
{
	if (fgets(line, MAXLEN, pfin))
	{
		size_t len = strlen(line);
		
		if (len && (line[len-1] == '\n'))
			line[len-1] = '\0';
		
		++linecount;
		return line;
	}
	else
		return NULL;
}

static void writeError(void)
{
	fprintf(stderr, "%s: error writing file %s!\n",
		progname, outfilename);
}

static int writeMagic(void)
{
	size_t len = strlen(NLS_BIN_MAGIC);
	size_t written;
	
	if (len & 1L)		/* ungerade */
		++len;
		
	written = fwrite(NLS_BIN_MAGIC, 1, len, pfout);
	if (written != len)
	{
		writeError();
		return FALSE;
	}
	fileoffset += len;
	return TRUE;
}

/*
 * Aktualisiert die Werte von Section und Åberschreibt ihn
 * an der in Section.NextSection gemerkten Stelle.
 */
static int updateSection (int lastsection)
{
	long oldoffset;
	char nul = '\0';
	
	/* HÑnge ein 0-Byte an die letzte Sektion, da sonst der String
	 * nicht terminiert ist.
	 */
	if (fwrite(&nul, 1, 1, pfout) != 1)
	{
		writeError();
		return FALSE;
	}
	++fileoffset;
	if (fileoffset & 1L) /* Auf gerade Offsets im File gehen */
	{
		char nul = '\0';
		
		if (fwrite(&nul, 1L, 1, pfout) != 1)
		{
			writeError();
			return FALSE;
		}
		++fileoffset;
	}
	
	oldoffset = (long)Section.NextSection;
	
	if (lastsection)
		Section.NextSection = (BINSECTION *)0L;
	else
		Section.NextSection = (BINSECTION *)fileoffset;
	
	if (fseek(pfout, oldoffset, SEEK_SET))
	{
		writeError();
		return FALSE;
	}
	
	if (fwrite(&Section, sizeof(BINSECTION), 1, pfout) != 1)
	{
		writeError();
		return FALSE;
	}
	
	if (fseek(pfout, fileoffset, SEEK_SET))
	{
		writeError();
		return FALSE;
	}
		
	return TRUE;
}

/* stellt sicher, daû line eine Section ist und schreibt
 * eine neue BINSECTION in outfilename. Wurde zuvor eine
 * Section bearbeitet, wird deren Zustand akutalisiert.
 */
static int writeSection(void)
{
	static int firstSection = TRUE;
	int noSection;
	char *cp;
	size_t len;
	
	noSection = strncmp(line, "Section:", strlen("Section:"));
	if (!firstSection)
	{
		if (!updateSection(noSection))
			return FALSE;
	}
	else
	{
		firstSection = FALSE;
		if (fileoffset & 1L) /* Auf gerade Offsets im File gehen */
		{
			char nul = '\0';
		
			if (fwrite(&nul, 1L, 1, pfout) != 1)
			{
				writeError();
				return FALSE;
			}
			++fileoffset;
		}
	}	
	
	if (noSection)
	{
		fprintf(stderr, "%s: section expected\n", progname);
		return FALSE;
	}
	
	Section.SectionTitel = NULL;
	Section.SectionStrings = NULL;
	Section.StringCount = 0L;

	/* offset dieser Section merken */
	Section.NextSection = (BINSECTION *)fileoffset;
	
	if (fwrite(&Section, sizeof(BINSECTION), 1, pfout) != 1)
	{
		writeError();
		return FALSE;
	}
	fileoffset += sizeof(BINSECTION);
	
	cp = strchr(line, ':')+1;
	len = strlen(cp)+1;
	Section.SectionTitel = (char *)fileoffset;
	
	if (verbose)
		fprintf(stderr, "%s: writing section %s\n", progname, cp);
				
	if (fwrite(cp, len, 1, pfout) != 1)
	{
		writeError();
		return FALSE;
	}
	fileoffset += len;

	return TRUE;
}

/*
 * ermittelt, ob es sich um eine text-Zeile handelt, und
 * welche Nummer sie hat
 */
static int isTextLine(long *nr, char **string)
{
	char *cp;
	
	*string = strchr(line, ':');
	if (!*string)
		return FALSE;
	
	if (line == *string)
		return FALSE;
	
	*nr = 0L;
	cp = line;
	while (cp < *string)
	{
		if (isdigit(*cp))
		{
			*nr = (*nr * 10) + (*cp - '0');
		}
		else
			return FALSE;
		++cp;
	}
	
	(*string)++;
	return TRUE;
}

static int writeString(char *str)
{
	size_t len = strlen(str);
	
	if (fwrite(str, len, 1, pfout) != 1)
		return FALSE;

	fileoffset += len;
	return TRUE;
}

static int writeHex(char *str, char **line)
{
	char c = 0;
	char legalupperchar[] = {"0123456789ABCDEF"};
	char legallowerchar[] = {"0123456789abcdef"};
	char *cp;
	int i, done = FALSE;
	
	for (i = 1; (i < 3) && (!done); ++i)
	{
		if (str[i] 
			&& (cp = strchr(legalupperchar, str[i])) != NULL)
		{
			c = (c<<4) + (char)(cp - legalupperchar);
		}
		else if (str[i] 
			&& (cp = strchr(legallowerchar, str[i])) != NULL)
		{
			c = (c<<4) + (char)(cp - legallowerchar);
		}
		else
			done = TRUE;
	
		if (!done)
			++(*line);
	}
	
	if (c == 0)
	{
		fprintf(stderr, "%s: illegal escape sequence\n", progname);
		return FALSE;
	}

	return (fwrite(&c, 1, 1, pfout) == 1);
}

static int writeOctal(char *str, char **line)
{
	char c = 0;
	char legalchar[] = {"01234567"};
	char *cp;
	int i, done = FALSE;
	
	for (i = 0; (i < 3) && (!done); ++i)
	{
		if (str[i] && (cp = strchr(legalchar, str[i])) != NULL)
			c = (c<<3) + (char)(cp - legalchar);
		else
			done = TRUE;
	
		if (!done && i)
			++(*line);
	}
	
	if (c == 0)
	{
		fprintf(stderr, "%s: illegal escape sequence\n", progname);
		return FALSE;
	}
	
	return (fwrite(&c, 1, 1, pfout) == 1);
}

static int writeLine(char *line)
{
	char *cp;
	int ok;

	while ((cp = strchr(line, '\\')) != NULL)
	{
		*cp = '\0';
		if (!writeString(line))
			return FALSE;

		line = cp+2;
		switch (cp[1])
		{
			case '\'':
			case '\"':
			case '\?':
				ok = TRUE;
				--line;
				break;
			
			case '\\':
				ok = writeString("\\");
				break;

			case 'a':
				ok = writeString("\a");
				break;
			
			case 'b':
				ok = writeString("\b");
				break;
			
			case 'f':
				ok = writeString("\f");
				break;
			
			case 'n':
				/* Wird auf Atari in BinÑrmodus geschrieben,
				 * deshalb \r. Unter Unix muû dies entfallen
				 */
				ok = writeString("\r\n");
				break;
			
			case 'r':
				ok = writeString("\r");
				break;

			case 't':
				ok = writeString("\t");
				break;

			case 'v':
				ok = writeString("\v");
				break;
			
			case 'x':
				ok = writeHex(cp+1, &line);
				break;
			
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				ok = writeOctal(cp+1, &line);
				break;
			
			default:
				fprintf(stderr, 
					"%s: illegal escape sequence in line %ld\n",
					progname, linecount);
				ok = TRUE;
				break;
		}
		if (!ok)
			return FALSE;
	}
	
	if (*line)
		return writeString(line);
	
	return TRUE;
}

/* schreibt die Textzeilen (sofern line eine ist) in outfilename
 * und ÅberprÅft die Reihenfolge der Zeilennummern. Aktualisiert
 * die Werte in Section.
 */
static int writeTextLine(int firstline)
{
	static long lastnr;
	long nr;
	char *line;
	char nul = '\0';
	
	/* Ist es eine Textzeile? */
	if (!isTextLine(&nr, &line))
		return FALSE;
	
	/* erste Zeile einer Sektion? */
	if (firstline)
		lastnr = -1L;
	
	/* ist die Nummer der Zeile korrekt? */
	if ((nr != lastnr) && (nr != lastnr+1))
	{
		fprintf(stderr, "%s: wrong number in text line\n", progname);
		return FALSE;
	}
	
	/* setze die Werte ein */
	if (firstline)
	{
		Section.SectionStrings = (char *)fileoffset;
		Section.StringCount = 1L;
		lastnr = 0L;
	}
	else if (nr == lastnr+1)
	{
		/* neuer String (neue Nummer) -> schreibe ein 0-Byte */
		if (fwrite(&nul, 1, 1, pfout) != 1)
		{
			writeError();
			return FALSE;
		}
		++fileoffset;
		++lastnr;
		++Section.StringCount;
	}
	
	/* schreibe den String line in die Datei outfilename */
	if (!writeLine(line))
	{
		writeError();
		return FALSE;
	}
	
	return TRUE;
}

static int compile(void)
{
	int gotlines;
	
	if (!getLine(line))
		return 1;
		
	if (strcmp(line, NLS_TEXT_MAGIC))
	{
		fprintf(stderr, "%s: %s ist keine NLS-Datei\n",
				progname, infilename);
		return 1;
	}
	
	if (!writeMagic())
		return 1;
	
	if (!getLine(line))
	{
		fprintf(stderr, "%s: no sections or strings!\n", progname);
		return 1;
	}
	
	gotlines = TRUE;
	
	while (gotlines && writeSection())
	{
		int firstline = TRUE;
		
		gotlines = (getLine(line) != NULL);
		
		while (gotlines && writeTextLine(firstline))
		{
			firstline = FALSE;
			gotlines = (getLine(line) != NULL);
		}
		
		if (Section.StringCount == 0)
		{
			fprintf(stderr, "%s: warning, empty section in line %ld\n",
				progname, linecount);
		}
	}

	if (gotlines)
	{
		fprintf(stderr, "%s: error in %s line %ld\n", progname,
				infilename, linecount);
		return 1;
	}
	else
	{
		updateSection(TRUE);
	}
	return 0;
}

int argvmain (int argc, char *argv[])
{
	int c, option_index, retcode;
	static int giveHelp = 0;
	static struct option long_options[]=
	{
		{"help", 0, &giveHelp, 1},
		{"verbose", 0, &verbose, 1},
		{ 0, 0, 0, 0}
	};
	patchstderr ();

	if ((!argv[0]) || (!argv[0][0])) argv[0] = "nlscomp";
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

	if (giveHelp || (optind >= (argc-1)))
		usage();

	infilename = argv[optind];
	outfilename = argv[optind+1];
	
	if (!strcmp(infilename, "-"))
	{
		pfin = stdin;
	}
	else
	{
		pfin = fopen(infilename, "r");
		if (pfin == NULL)
		{
			fprintf(stderr, "%s: cannot open %s!\n", progname,
					infilename);
			return 1;
		}
	}

	pfout = fopen(outfilename, "wb");
	if (pfout == NULL)
	{
		fprintf(stderr, "%s: cannot open %s for writing!\n", 
				progname, outfilename);
		if (pfin != stdin)
			fclose(pfin);
		return 1;
	}
	
	retcode = compile();
	
	if (pfin != stdin)
		fclose(pfin);
	fclose(pfout);
	
	if (retcode) /* Fehler beim Erzeugen des outfile */
		remove(outfilename);
	return retcode;
}

