/*
 * filebuf.c  -  file buffering routines
 * 16.05.90
 */

#include <stdio.h> 
#include <tos.h>

#include "alloc.h"
#include "chario.h"
#include "filebuf.h"
#include "mupfel.h"

SCCS(filebuf);

size_t filebuf(int handle, const char *file, char **buffer,
	const char *cmd)
{
	long endpos;
	
	if ((endpos=Fseek(0L,handle,SEEK_END))<0)
		return BUF_ERR;

	Fseek(0L,handle,SEEK_SET);

	*buffer = Malloc(endpos+1);
	
	if (*buffer == NULL)
	{
		eprintf(FB_NOMEM "\n",cmd,file);
		return BUF_ERR;
	}		
	if (Fread(handle,endpos,*buffer)!=endpos)
	{
		eprintf(FB_READERR "\n",cmd,file);
		Mfree(*buffer);
		return BUF_ERR;
	}

	Fclose(handle);
	(*buffer)[endpos] = '\0';
	return (size_t)endpos;
}
