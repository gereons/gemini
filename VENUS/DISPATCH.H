/*
 * dispatch.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for dispatch.c
 *
 * last change: 17.06.1990
 */

void doDclick(word mx,word my, word kstate);
void simDclick(word kstate);
word startFile(WindInfo *wp,word fobj,word showpath,char *label,
				char *path,char *name,char *command);
