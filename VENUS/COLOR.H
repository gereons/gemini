/*
 * color.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: functions to color icons
 *
 * last change: 11.08.1990
 */
 
#ifndef __color__
#define __color__

void ColorSetup(char color, OBJECT *tree, word foresel, word forebox,
				word backsel, word backbox);

char ColorSelect(char color, word foreground, OBJECT *tree,
				 word selindex, word boxindex, word circle);

char ColorMap(char truecolor);

#endif