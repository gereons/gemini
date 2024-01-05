/*
 * vt52.h - vt52 escape sequence definitions
 * 11.12.89
 */

#ifndef _M_VT52
#define _M_VT52
 
#define cursoron()		vt52('e')
#define cursoroff()		vt52('f')

#define clearscreen()	vt52('E')
#define cleareol()		vt52('K')
#define cleareos()		vt52('J')

#define cursorhome()	vt52('H')

#define wrapon()		vt52('v')
#define wrapoff()		vt52('w')

#define reverseon()		vt52('p')
#define reverseoff()	vt52('q')

#define deleteline()	vt52('M')
#define insertline()	vt52('L')

#endif