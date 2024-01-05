/* 
 * handle.h - file handle constants 
 * 17/03/89
 */

#ifndef _M_HANDLE
#define _M_HANDLE

#define ILLHND (-99)	/* illegal handle */
#define PRNHND (-3)		/* handle for PRN: */
#define AUXHND (-2)		/* handle for AUX: */
#define CONHND (-1)		/* handle for CON: */
#define MINHND PRNHND	/* smallest legal handle */

#endif