/*
 * tree.h - definitions for tree.c
 * 18.02.90
 */

#ifndef _M_TREE
#define _M_TREE
 
int cptree(char *fromdir, char *todir);
int rmtree(const char *path);
int hashtree(const char *path);

#endif