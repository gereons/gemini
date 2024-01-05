/*
 * undo.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for undo.c
 *
 * last change: 10.10.1990
 */

void storeWOpenUndo(word open,WindInfo *wp);
void storeWMoveUndo(WindInfo *wp,word wasSized);
void storeWFullUndo(WindInfo *wp);
void doUndo(void);
void clearUndo(void);
void storePathUndo(WindInfo *wp,const char *folder,word isUndo);
void ignoreUndos(word yesno);
void UndoFolderVanished(const char *path);
