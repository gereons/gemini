/* 
 * attrib.h - file attribute bit definitions
 * 03.10.89
 */

#ifndef _M_ATTRIB
#define _M_ATTRIB

/* File attribute bits */
#define FA_RDONLY	0x01		/* File is read-only */
#define FA_HIDDEN	0x02		/* File is hidden */
#define FA_SYSTEM	0x04		/* File is a system file */
#define FA_LABEL	0x08		/* File is the volume label */
#define FA_DIREC	0x10		/* File is a directory name */
#define FA_ARCH	0x20		/* File is modified */

/* Attribute to match all files and dirs */
#define FA_ATTRIB	(FA_DIREC|FA_RDONLY|FA_HIDDEN|FA_SYSTEM)

/* Attribute to match all files */
#define FA_FILES	(FA_RDONLY|FA_HIDDEN|FA_SYSTEM)

#endif