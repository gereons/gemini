/*
 * filebuf.h  -  definitions for filebuf.c
 * 11.02.90
 */
 
#ifndef _M_FILEBUF
#define _M_FILEBUF

#define BUF_ERR	(size_t)-1

size_t filebuf(int handle,const char *file, char **buffer,
	const char *cmd);

#endif

 