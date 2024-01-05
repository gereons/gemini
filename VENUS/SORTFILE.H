/*
 * sortfile.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for sortfile.c
 *
 * last change: 23.11.1990
 */

/*
 * SORT_TYPE must be defined in the module which uses
 * fileSort; normally it will be FileInfo
 */
typedef word ( *CmpFkt)(SORT_TYPE *f1, SORT_TYPE *f2);
typedef word ( *FileCmpFkt)(FileInfo *f1, FileInfo *f2);

void qSort(SORT_TYPE **list,CmpFkt cmpfiles,word left,word right);

void fileSort(WindInfo *wp, word sortmode);