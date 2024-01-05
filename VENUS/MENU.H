/*
 * menu.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for menu.c
 *
 * last change: 11.08.1990
 */


word doMenu(word mtitle, word mentry, word kstate);
void manageMenu(void);
word callMupfel(void);
word doMupfel(const char *command, word waitkey);
