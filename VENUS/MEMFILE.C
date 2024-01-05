/*
 * @(#) Gemini\memfile.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: functions for textfiles in memory
 */

#include <string.h>
#include <tos.h>

#include "vs.h"
#include "memfile.h"
#include "myalloc.h"

store_sccs_id(memfile);

MFileInfo *mopen(const char *name)
{
	MFileInfo *mp;
	unsigned long fsize;
	unsigned long bufsize;
	word fhandle;
	char *cp;
	
	if((fhandle = Fopen(name, 0)) > 0)
	{
		fsize = Fseek(0L, fhandle, 2);	/* seek to end of file */
		Fseek(0L, fhandle, 0);			/* and rewind */

		bufsize = sizeof(MFileInfo) + (fsize * sizeof(char));
		if((mp = malloc(bufsize)) != NULL)
		{
			cp = (char *) mp + sizeof(MFileInfo);
			mp->curP = mp->bufStart = cp;
			mp->bufEnd = cp + fsize - 1;

			if(Fread(fhandle, fsize, cp) == fsize)
			{
				Fclose(fhandle);
				return mp;
			}
			else
			{
				free(mp);	/* free allocated memory */
			}
		}
		Fclose(fhandle);	/* come here only on failure! */
	}

	return NULL;
}

void mclose(MFileInfo *mp)
{
	free(mp);
}

char *mgets(char *str, word n, MFileInfo *mp)
{
	long most;
	char c, *cp;
	
	most = mp->bufEnd - mp->curP;

	if (most > 0)				/* still something to read */
	{
		if (most < n)
			n = (word) most;
			
		cp = str;
		while (n > 0)
		{
			*cp++ = c = *mp->curP++;	/* copy char */
			if((strchr("\n\r\f", c) != NULL) || c == '\0')
			{
				--cp;			/* don't copy newline */
				while (n && (strchr("\n\r\f", *mp->curP)))
				{
					mp->curP++;		/* Atari ST cr+lf match */
					--n;
				}
				break;
			}
			--n;
		}
		*cp = '\0';			/* always give a terminator */

		return str;
	}
	else
		return NULL;
}