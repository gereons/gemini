/*
 * venuserr.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for venuserr.c
 *
 * last change: 14.10.1990
 */


word venusErr(const char *errmessage,...);
word changeDisk(const char drive,const char *label);
void sysError(word errnumber);
word venusChoice(const char *message,...);
word venusInfo(const char *s,...);
word venusInfoFollow(const char *s,...);
word venusDebug(const char *s,...);