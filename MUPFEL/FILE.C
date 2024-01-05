/*
 * file.c  -  internal "file" command
 * 10.11.90
 */
 
#include <string.h>
#include <tos.h>

#include "chario.h"
#include "chmod.h"
#include "handle.h"
#include "mupfel.h"
#include "strsel.h"

#define inbuf(str)	(strstr(buffer,str)!=NULL)

SCCS(file);

static char *guesstype(char *buffer, int bufmax)
{
	int i;
	
	strupr(buffer);
	if (inbuf("#DEFINE") || inbuf("#INCLUDE") || (inbuf("/*") && inbuf("*/")))
		return FI_CSRC;
	if (inbuf("MOVE.W") || inbuf("MOVE.L"))
		return FI_ASMSRC;
	if ((inbuf("IMPLEMENTATION") || inbuf("DEFINITION")) && inbuf("MODULE"))
		return FI_M2SRC;
	if (inbuf("PROGRAM") || inbuf("PROCEDURE"))
		return FI_PASCSRC;
	for (i=0; i<bufmax; ++i)
		if ((uchar)buffer[i] > 128)
			return "";
	return FI_ASCII;
}

static void filetype(char *file)
{
	int hnd, lastdot;
	long bufmax;
	char ext[4], ftype[80], buffer[512];
	
	if ((hnd=Fopen(file,0))<MINHND)
	{
		if (isdir(file))
			mprintf("%s: " FI_DIR "\n",file);
		else
			eprintf("file: " FI_CANTOPEN "\n",file);
		return;
	}
	bufmax=Fread(hnd,512,buffer);
	if (bufmax>0)
		buffer[bufmax-1]='\0';
	Fclose(hnd);

	if (bufmax==0)
	{
		mprintf("%s: %s\n",file,FI_EMPTY);
		return;
	}
		
	lastdot=strrpos(file,'.');
	if (lastdot==-1 || lastdot < strlen(file)-4)
		strcpy(ext,"");
	else
		strcpy(ext,&file[lastdot+1]);
	strupr(ext);
	*ftype='\0';
	STRSELECT(ext)
	WHEN("RSC")
		strcpy(ftype,FI_RSC);
	WHEN2("PRG","APP")
		if (access(file,A_EXEC))
			strcpy(ftype,FI_GEMAPP);
		if (docheckfast(file,"file",FALSE,FALSE))
			strcat(ftype," (fastload)");
	WHEN("ACC")
		if (access(file,A_EXEC))
			strcpy(ftype,FI_GEMACC);
		if (docheckfast(file,"file",FALSE,FALSE))
			strcat(ftype," (fastload)");
	WHEN2("TOS","TTP")
		if (access(file,A_EXEC))
			strcpy(ftype,FI_TOSAPP);
		if (docheckfast(file,"file",FALSE,FALSE))
			strcat(ftype," (fastload)");
	WHEN("ARC")
		strcpy(ftype,FI_ARC);
	WHEN3("PI1","PI2","PI3")
		strcpy(ftype,FI_DEGAS);
	WHEN("ZOO")
		strcpy(ftype,FI_ZOO);
	WHEN("LZH")
		strcpy(ftype,FI_LHARC);
	DEFAULT
		strcpy(ftype,guesstype(buffer,(int)bufmax));
	ENDSEL
	if (!*ftype)
		strcpy(ftype,FI_DATA);
	mprintf("%s: %s\n",file,ftype);
}

int m_file(ARGCV)
{
	int i;
	
	if (argc==1)
		return printusage(NULL);
	for (i=1; i<argc && !intr(); ++i)
		filetype(argv[i]);
	return 0;
}
