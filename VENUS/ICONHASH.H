/*
 * iconhash.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for iconhash.c
 *
 * last change: 17.04.1989
 */

void builtIconHash(void);
DisplayRule *getHashedRule(word isFolder,const char *name);
void clearHashTable(void);
