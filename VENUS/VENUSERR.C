/*
 * @(#) Gemini\Venuserr.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 */

#include <string.h>
#include <aes.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "changbox.rh"

#include "toserror.h"
#include "util.h"
#include "stand.h"

store_sccs_id(venuserr);

/* externals
 */
extern OBJECT *pchangebox;
#if MERGED
extern struct CommInfo CommInfo;
#endif

#define DEBUG	0

/* internal texts
 */
#define NlsLocalSection	"VenusErr"
enum NlsLocalText{
T_ERR,		/*[Oh*/
T_CHOICE,	/*[Nein|[Ja*/
T_INFO,		/*[OK*/
T_INFOFOLLOW,		/*[Weiter*/
T_DRVNOTREADY,	/*Das Laufwerk ist nicht bereit. Ist
 vielleicht keine Diskette/Cartridge eingelegt?*/
T_NOWRITE,	/*Ein Schreibfehler ist aufgetreten!*/
T_ILLMEDIA,	/*Dieses Medium hat ein unbekanntes Format und
 kann daher nicht verarbeitet werden!*/
T_NOREAD,	/*Es ist ein Fehler beim Zugriff auf das Medium aufgetreten!*/
T_WRITEPROT,/*Das Medium ist schreibgeschtzt!*/
T_ACCESS,	/*Die Datei/das Medium kann nicht beschrieben werden!*/
T_NOMEM,	/*Es ist nicht genung Speicher frei, um diese Aktion auszufhren!*/
T_NOEXEC,	/*Diese Datei ist nicht ausfhrbar!*/
};

/*
 * void venusErr(const char *errmessage)
 * makes form_alert with errmessage
 */
word venusErr(const char *errmessage,...)
{
	char tmp[1024];
	va_list argpoint;
	
	va_start(argpoint,errmessage);
	vsprintf(tmp,errmessage,argpoint);
	va_end(argpoint);
	return DialAlert(ImSqExclamation(), tmp, 0, NlsStr (T_ERR));
}

word venusDebug(const char *s,...)
{
#if DEBUG
	char tmp[1024];
	va_list argpoint;
	
	va_start(argpoint, s);
	vsprintf(tmp, s, argpoint);
	va_end(argpoint);
	return DialAlert(ImSqExclamation(), tmp, 0, NlsStr (T_ERR));
#else
	(void)s;
	return 1;
#endif
}

/*
 * word venusChoice(const char *message)
 * return 1, if the answer was "Yes", 0 otherwise
 */
word venusChoice(const char *message,...)
{
	char tmp[1024];
	va_list argpoint;
	
	va_start(argpoint,message);
	vsprintf(tmp,message,argpoint);
	va_end(argpoint);
	return DialAlert(ImSqQuestionMark(), tmp, 1, NlsStr (T_CHOICE));
}

word venusInfo(const char *s,...)
{
	char tmp[1024];
	va_list argpoint;
	
	va_start(argpoint,s);
	vsprintf(tmp,s,argpoint);
	va_end(argpoint);
	return DialAlert(ImInfo(), tmp, 0, NlsStr (T_INFO));
}

word venusInfoFollow(const char *s,...)
{
	char tmp[1024];
	va_list argpoint;
	
	va_start(argpoint,s);
	vsprintf(tmp,s,argpoint);
	va_end(argpoint);
	return DialAlert(ImInfo(), tmp, 0, NlsStr (T_INFOFOLLOW));
}

/* 
 * word changeDisk(const char drive,const char *label)
 * give possibility to change disk(TRUE) or cancel command(FALSE)
 */
word changeDisk(const char drive,const char *label)
{
	DIALINFO d;
	word retcode;
	char tmp[MAX_FILENAME_LEN];
	char *pdisk,*pname;
	word tmp_mouse;
	MFORM tmp_mform;
	
	GrafGetForm (&tmp_mouse, &tmp_mform);
	GrafMouse (ARROW, NULL);
	
	pdisk = pchangebox[CHANDRIV].ob_spec.tedinfo->te_ptext;
	pname = pchangebox[CHANLABL].ob_spec.tedinfo->te_ptext;
	
	pdisk[0] = drive;
	strcpy(tmp,label);
	makeEditName(tmp);
	strcpy(pname,tmp);
	
	DialCenter(pchangebox);
	DialStart(pchangebox,&d);
	DialDraw(&d);
	
	retcode = DialDo(&d,0) & 0x7FFF;
	setSelected(pchangebox,retcode,FALSE);
	
	DialEnd(&d);
	GrafMouse (tmp_mouse, &tmp_mform);
	
	return (retcode == CHANOK);
}

void sysError(word errnumber)
{
	char *message = NULL;

#if MERGED
	(void)errnumber;
	message = CommInfo.errmsg;
	CommInfo.errmsg = NULL;
#else
	switch(errnumber)
	{
		case DRIVE_NOT_READY:
			message = NlsStr (T_DRVNOTREADY);
			break;
		case UNKNOWN_MEDIA:
			message = NlsStr (T_ILLMEDIA);
			break;
		case UNKNOWN_CMD:
		case BAD_REQUEST:
		case CRC_ERROR:
		case SEEK_ERROR:
		case SECTOR_NOT_FOUND:
		case READ_FAULT:
		case BAD_SECTORS:
			message = NlsStr (T_NOREAD);
			break;
		case NO_PAPER:
			break;
		case WRITE_FAULT:
			message = NlsStr (T_NOWRITE);
			break;
		case MEDIA_CHANGE:
		case UNKNOWN_DEVICE:
			break;
		case WRITE_PROTECT:
			message = NlsStr (T_WRITEPROT);
			break;
		case INSERT_DISK:
			break;
		case EFILNF:
			break;
		case EACCDN:
			message = NlsStr (T_ACCESS);
			break;
		case ENSMEM:
		case -39:			/* pexec failed */
			message = NlsStr (T_NOMEM);
			break;
		case EDRIVE:
			break;
		case ENMFIL:
			break;
		case EPLFMT:
			message = NlsStr (T_NOEXEC);
			break;
		default:
			break;
	}
#endif

	if (message)
	{
		if (message[strlen(message)-1] == '\n')
			message[strlen(message)-1] = '\0';
		venusErr(message);
	}
}