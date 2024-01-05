/*
 * @(#) Gemini\iconrule.h
 * @(#) Stefan Eissing, 17. April 1991
 *
 * description: Header File for iconrule.c
 */

word initIcons(const char *name);		/* must be called before
								 		 * loading desktop.rsc */
void FixIcons(void);

word editIconRule(void);
ConfInfo *makeIconConf (ConfInfo *aktconf, char *buffer);
void addIconRule(const char *line);
void freeIconRules(void);
void insDefIconRules(void);

void SetIconTextWidth(ICONBLK *pib);

OBJECT *getDeskObject(word nr);
OBJECT *getStdDeskIcon(void);
OBJECT *getStdFileIcon(void);
OBJECT *getIconObject(word isFolder, char *fname, char *color);
OBJECT *getBigIconObject(word isFolder, char *fname, char *color);
OBJECT *getSmallIconObject(word isFolder, char *fname, char *color);

DisplayRule *getFirstDisplayRule(void);
