/*
 * windstak.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for windstak.c
 *
 * last change: 01.06.1990
 */

void pushWindBox(word x,word y,word w,word h);
word popWindBox(word *x,word *y,word *w,word *h);
word writeBoxes(word fhandle, char *buffer);
void freeWBoxes(void);
