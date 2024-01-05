/*
 * select.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for select.c
 *
 * last change: 29.01.1991
 */

#ifndef __select__
#define __select__

void objselect(WindInfo *wp,word obj);
void objdeselect(WindInfo *wp,word obj);
void desWindObjects(WindInfo *wp,word type);
void desObjExceptWind(WindInfo *wp, word type);
void deselectObjects(word type);
word thereAreSelected(WindInfo **wpp);
word getOnlySelected(WindInfo **wpp,word *objnr);
void desNotDragged(WindInfo *wp);
word charSelect(WindInfo *wp,char c,word forfolder);
word stringSelect(WindInfo *wp,const char *name, word redraw);

word SelectAllInTopWindow(void);

char *GetSelectedObjects(void);

void MarkDraggedObjects(WindInfo *wp);
void UnMarkDraggedObjects(WindInfo *wp);

#endif