/*
 * @(#) Venus\fileutil.h
 * @(#) Stefan Eissing, 17. Mai 1991
 *
 * description: Header File for fileutil.c
 *
 */

word ValidFileName (const char *name);

word legalDrive (word drive);			/* drive as number */
word isDirectory (const char *path);
word fileExists (const char *name);
long getFileSize (const char *name);
word addFileAnz (char *path, uword *foldnr, uword *filenr,long *size);
word countMarkedFiles (WindInfo *wp, char *path, uword *foldnr,
				uword *filenr,long *size);
word stripFileName (char *path);
word stripFolderName (char *path);
void addFileName (char *path,const char *name);
void addFolderName (char *path,const char *name); 
word getBaseName (char *base, const char *path);
void timeString (char *cp, word time);
void dateString (char *cp, word date);
word getLabel (word drive,char *labelname);

char getDrive (void);
word setDrive (const char drive);
word getFullPath (char *path);
word setFullPath (const char *path);

word isExecutable (const char *name);
word sameLabel (const char *label1,const char *label2);
word isEmptyDir (const char *path);
void getSPath (char *path);
void getTPath (char *path);
word forceMediaChange (word drive);
word getDiskSpace (word drive, long *bytesused, long *bytesfree);
word fileRename (char *oldname,char *newname);
word setFileAttribut (char *name,word attrib);
word dirCreate (char *path);
word isAccessory (const char *filename);
word tosVersion (void);
word delFile (const char *fname);
void char2LongKey (char c, long *key);

word Fputs (word fhandle, char *string);
