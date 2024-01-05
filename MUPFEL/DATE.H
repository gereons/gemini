/*
 * date.h - definitions for date.c
 * 16/03/89
 *
 * 21.01.91: strdate() removed in favor of loctime.h (jr)
 */

#ifndef _M_DATE
#define _M_DATE

typedef union {
	unsigned int d;
	struct {
		unsigned year  :7;
		unsigned month :4;
		unsigned day   :5;
	} s;
} dosdate;

typedef union {
	unsigned int t;
	struct {
		unsigned hour :5;
		unsigned min  :6;
		unsigned sec  :5;
	} s;
} dostime;

typedef union {
	unsigned long dt;
	struct {
	    unsigned year:  7;
	    unsigned month: 4;
	    unsigned day:   5;
	    unsigned hour:  5;
	    unsigned min:   6;
	    unsigned sec:   5;
	} s;
} xtime;

extern xtime systime;
 
#endif