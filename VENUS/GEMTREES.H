/*
 * gemtrees.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for gemtrees.c
 *
 * last change: 10.09.1990
 */

#ifndef __gemtrees__
#define __gemtrees__

#ifndef word
	#define word	int		/* signed 16 Bit for AES */
#endif

/* Gravity definition
 */
#define D_NORTH		1
#define D_SOUTH		2
#define D_WEST		3
#define D_EAST		4

void walkGemTree(OBJECT *po, word objnr,
				void (*walkfunc)(OBJECT *po,word objnr));
void vStretchTree(OBJECT *po,word objnr,word z,word n);

word sortTree(OBJECT *tree, word startobj, word maxlevel, 
					word grav1, word grav2);

void FixTree(OBJECT *tree);

void SetLastObject(OBJECT *tree);

#endif