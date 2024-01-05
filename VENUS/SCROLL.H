/*
 * scroll.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for scroll.c
 *
 * last change: 24.09.1990
 */

void doArrowed(word whandle, word desire);
void doVslid(word whandle, word position);
void charScroll(char c, word kstate);
void scrollit(WindInfo *wp, word div_skip, word redraw);
word calcYSkip(WindInfo *wp, word toskip);
