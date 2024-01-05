/*
 * toserr.h - TOS error code definitions
 * 30.06.88
 */

#ifndef _M_TOSERR
#define _M_TOSERR

/* BIOS/XBIOS error codes */ 
#define	ERROR			-1
#define	DRIVE_NOT_READY	-2
#define	UNKNOWN_CMD		-3
#define	CRC_ERROR			-4
#define	BAD_REQUEST		-5
#define	SEEK_ERROR		-6
#define	UNKNOWN_MEDIA		-7
#define	SECTOR_NOT_FOUND	-8
#define	NO_PAPER			-9
#define	WRITE_FAULT		-10
#define	READ_FAULT		-11
#define	GENERAL_HISHAP		-12
#define	WRITE_PROTECT		-13
#define	MEDIA_CHANGE		-14
#define	UNKNOWN_DEVICE		-15
#define	BAD_SECTORS		-16
#define	INSERT_DISK		-17

/* GEMDOS error codes */
#define	EINVFN	-32	/* invalid function number */
#define	EFILNF	-33	/* file not found */
#define	EPTHNF	-34	/* path not found */
#define	ENHNDL	-35	/* no more handles */
#define	EACCDN	-36	/* access denied */
#define	EIHNDL	-37	/* invalid handle */
#define	ENSMEM	-39	/* no more memory */
#define	EIMBA	-40	/* invalid memory block address */
#define	EDRIVE	-46	/* invalid drive */
#define	ENSAME	-48	/* not same drive */
#define	ENMFIL	-49	/* no more files */
#define	ERANGE	-64	/* invalid file offset */
#define	EINTRN	-65	/* internal error */
#define	EPLFMT	-66	/* invalid program load format */
#define	EGSBF	-67	/* Mshrink()/Mfree() error */

#define	bioserror(x)	(x<=ERROR && x>=INSERT_DISK)
#define	toserror(x)	(x<=EINVFN && x>=EGSBF)

#endif