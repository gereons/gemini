/*
 * alert.h  -  Definitions for alert.c
 * 16/03/89
 */

#ifndef _M_ALERT
#define _M_ALERT

#define NO_ICON	0		/* No Icon */
#define NOTE_ICON	1		/* ! */
#define WAIT_ICON	2		/* ? */
#define STOP_ICON	3		/* STOP */

int alert(int icon,int defbutton,int numbuttons,char *fmt, ...);
void alertstr(char *str);

#endif