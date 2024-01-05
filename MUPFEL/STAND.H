/*
 * stand.h  -  standalone / merged version definitions
 */

#ifndef _M_STAND
#define _M_STAND

#define MERGED		1
#define GEMINI		MERGED
#define STANDALONE	(!MERGED)

#if MERGED
	#define PGMNAME	"Gemini"
	void disp_string(char *s);
	void setBackDestr(int back_dest);
	void greetings(void);
#else
	#define PGMNAME	"Mupfel"
#endif

#endif