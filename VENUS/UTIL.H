/*
 * util.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for util.c
 *
 * last change: 19.01.1991
 */

#ifndef __util__
#define __util__

typedef enum {M_CP, M_MV, M_RM} 
MFunction;

word system(const char *cmd);

char *getDraggedNames(WindInfo *wp);

word GotBlitter(void);
word SetBlitter(word mode);

word getScrapIcon(void);
word getTrashIcon(void);

word updateSpecialIcon(word iconNr);
void pathUpdate(char *path1,char *path2);
word buffPathUpdate(const char *path);
void flushPathUpdate(void);

long scale123(register long p1,register long p2,register long p3);
word checkPromille(word pm, word def);
void deskAlign(OBJECT *deskobj);
void fulldraw(OBJECT *tree,word objnr);

word escapeKeyPressed(void);
word killEvents(word eventtypes, word maxtimes);
void WaitKeyButton(void);
word ButtonPressed(word *mx, word *my, word *bstate, word *kstate);

void setSelected(OBJECT *tree, word objnr, word flag);
word isSelected(OBJECT *tree, word objnr);

void setHideTree(OBJECT *tree, word objnr, word flag);
word isHidden(OBJECT *tree, word objnr);

void setDisabled(OBJECT *tree, word objnr, word flag);
word isDisabled(OBJECT *tree, word objnr);

word countObjects(OBJECT *tree,word startobj);
void doFullGrowBox(OBJECT *tree,word objnr);
void makeFullName(char *name);
void makeEditName(char *name);
word charAlign(word x);

int fileWasMoved(const char *oldname,const char *newname);
FileInfo *getfinfo(OBJECT *po);
IconInfo *getIconInfo(OBJECT *po);
word getBootPath(char *path,const char *fname);

void setBigClip(word handle);
void remchr(char *str, char c);
word GotGEM14(void);

long GetHz200(void);

/* nur bei Gemini */
int CallMupfelFunction(MFunction f, const char *fpath,
						const char *tpath, word *memfailed);

#endif