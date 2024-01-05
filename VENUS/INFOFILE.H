/*
 * infofile.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for infofile.c
 *
 * last change: 10.09.1990
 */

void readInfoDatei(const char *fname1, const char *fname2, word *tmp);
void writeInfoDatei(const char *fname, word update);
void execConfInfo(word todraw);
void makeConfInfo(void);
