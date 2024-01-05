/*
 * scut.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: handles dialog for Shortcuts
 * 				in venus
 *
 * last change: 11.08.1990
 */


#ifndef __scut__
#define __scut__

/* defines fÅr Shortcuts */
#define SCUT_NO		0
#define SCUT_0		0x0001
#define SCUT_1		0x0002
#define SCUT_2		0x0004
#define SCUT_3		0x0008
#define SCUT_4		0x0010
#define SCUT_5		0x0020
#define SCUT_6		0x0040
#define SCUT_7		0x0080
#define SCUT_8		0x0100
#define SCUT_9		0x0200


word Object2SCut(word index);
word SCut2Object(word shortcut);
OBSPEC SCut2Obspec(word shortcut);

word SCutInstall(word shortcut);
word SCutRemove(word shortcut);

word DoSCut(word kstate, word kreturn);

word SCutSelect(OBJECT *tree, word index, 
				word newshort, word origshort, word circle);

#endif
