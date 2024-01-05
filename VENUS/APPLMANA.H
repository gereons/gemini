/*
 * @(#) Gemini\applmana.h
 * @(#) Stefan Eissing, 17. April 1991
 *
 * description: Header File for applmana.c
 */

void applDialog(void);
ConfInfo *makeApplConf(ConfInfo *aktconf, char *buffer);
void addApplRule(const char *line);
void freeApplRules(void);

word getApplForData(const char *name, char *program,
					char *label, word *mode);

word getStartMode(const char *name, word *mode);

void ApRenamed(const char *newname, const char *oldname,
					const char *path);
void ApNewLabel(char drive, const char *oldlabel,
					const char *newlabel);

word removeApplInfo(const char *name);
