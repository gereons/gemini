/*
 * token.c  -  tokenize a command string
 * 05.06.90
 */

#include "alloc.h" 
#include "mupfel.h"
#include "string.h"
#include "token.h"

typedef struct gen_list
{
	struct gen_list *next;
} GEN_LIST;

#define LL_ENTRY GEN_LIST
#include "linklist.h"

#define tappend(a,b,c)	append((GEN_LIST *)a,(GEN_LIST **)b,c)

static int isquoted(const char *word)
{
	return *word == '"' || *word == '\'';
}

static int hasdollar(const char *word)
{
	return strchr(word,'$') != NULL;
}

static int isassign(const char *word)
{
	return strchr(word,'=') != NULL;
}

static void splitup(char *cmdline, WORD_LIST **wordlist)
{
	char *l = cmdline;
	char token[200];
	char *tp = token;
	WORD_DESC *wd = malloc(sizeof(WORD_DESC));
	WORD_LIST wl;
	
	while (*l != '\0' && *l != ';')
	{
		switch (*l)
		{
			case ' ':
			case '\t':
				if (tp != token)
				{
					*tp = '\0';
					wd->word = strdup(token);
					wd->quoted = isquoted(token);
					wd->dollar = hasdollar(token);
					wd->assign = isassign(token);
					wl.word = wd;
					tappend(&wl,wordlist,sizeof(WORD_DESC));
					tp = token;
				}
				++l;
				break;
			case '"':
			case '\'':
				{
					char end = *l;
					
					while (*l!='\0' && *l!=end)
						*tp++ = *l++;
				}
			default:
				*tp++ = *l++;
				break;
		}
	}
}

static int buildargv(WORD_LIST *wordlist, char **argv)
{
	int argc = 0;
	
	while (wordlist)
	{
		argv[argc] = strdup(wordlist->word->word);
		++argc;
		wordlist = wordlist->next;
	}
	return argc;
}

int tokenize(char *cmdline,char **argv)
{
	int argc = 0;
	WORD_LIST *wordlist = NULL;
	
	splitup(cmdline,&wordlist); /* also does alias expansion */
/*
	dollarexpand(wordlist);
	wildcardexpand(wordlist);
	extractredirect(wordlist);
*/
	argc = buildargv(wordlist,argv);

	return argc;
}
