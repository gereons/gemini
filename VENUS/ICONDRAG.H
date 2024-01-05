/*
 * icondrag.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for icondrag.c
 *
 * last change: 22.09.1990
 */

#ifndef __icondrag__
#define __icondrag__

word FindObject(OBJECT *tree, word x, word y);

void doIcons(WindInfo *wp,word mx,word my,word kstate);
word isOnIcon(word mx,word my,OBJECT *tree,word obj);

#endif