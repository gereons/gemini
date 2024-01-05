/*
 * @(#) Gemini\files.h
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: Header File for files.c
 */

word getfiles(WindInfo *wp);
void makeftree(WindInfo *wp);
void freeftree(WindInfo *wp);
void freefiles(WindInfo *wp);
int SizeFileTree (WindInfo *wp);

FileInfo *GetSelectedFileInfo(WindInfo *wp);
