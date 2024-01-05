/*
 * defines fÅr TOS Fehlernummern
 *
 * by Stefan Eissing
 *
 * 21.01.1989
 */

#define ERROR			-1		/* unspecificable error */
#define DRIVE_NOT_READY	-2
#define UNKNOWN_CMD		-3		/* command unknown to device */
#define CRC_ERROR		-4		/* crc error reading sector */
#define BAD_REQUEST		-5		/* error in calling of device */
#define SEEK_ERROR		-6		/* track not found */
#define UNKNOWN_MEDIA	-7		/* can't recognize bootsector */
#define SECTOR_NOT_FOUND	-8
#define NO_PAPER		-9		/* printer not ready */
#define WRITE_FAULT		-10
#define READ_FAULT		-11
#define GENERAL_MISHAP	-12		/* reserved for future catastrophies */
#define WRITE_PROTECT	-13
#define MEDIA_CHANGE	-14
#define UNKNOWN_DEVICE	-15
#define BAD_SECTORS		-16		/* bad sectors (during format) */
#define INSERT_DISK		-17
#define EINVFN			-32		/* unknown function number */
#define EFILNF			-33		/* file not found */
#define EPTHNF			-34		/* path (folder) not found */
#define ENHNDL			-35		/* out of handles */
#define EACCDN			-36		/* access denied */
#define EIHNDL			-37		/* uncorrect handle */
#define ENSMEM			-38		/* unsufficient memory */
#define EIMBA			-40		/* wrong pointer (to memory block) */
#define EDRIVE			-46		/* drive specifier not correct */
#define ENSAME			-48
#define ENMFIL			-49		/* can't open more files */
#define ERANGE			-64
#define EINTRN			-65		/* internal error */
#define EPLFMT			-66		/* error in format of file */
#define EGSBF			-67		/* error in Mshrink/Mfree */
