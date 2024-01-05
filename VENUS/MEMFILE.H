/*
 * memfile.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: header file with definitions for textfiles in memory
 *
 * last change: 01.06.1990
 */

#if  !defined( __MEMFILE__ )
#define __MEMFILE__

typedef struct
{
	char *bufStart;			/* Start of Buffer */
	char *bufEnd;			/* End of Buffer */
	char *curP;				/* current pointer */
} MFileInfo;

MFileInfo *mopen(const char *name);
void mclose(MFileInfo *mp);
char *mgets(char *str, word n, MFileInfo *mp);


#endif