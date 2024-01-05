/*
 * @(#) Gemini\color.c
 * @(#) Stefan Eissing, 03. April 1991
 *
 * description: functions to color icons
 */
 
#include <stdlib.h>
#include <string.h>
#include <flydial\flydial.h>

#include "vs.h"
#include "color.h"
#include "util.h"

/* externals
 */
extern OBJECT *pcolorbox;


static word color2Index(unsigned char color, word foreground)
{
	if (foreground)
		color = color >> 4;
	else
		color &= 0x0F;
	
	return color + 1;
}

static void color2Obspecs(char color, OBSPEC *fore, OBSPEC *back)
{
	word foreindex;
	word backindex;
	
	foreindex = color2Index(color, TRUE);
	backindex = color2Index(color, FALSE);
	
	*fore = pcolorbox[foreindex].ob_spec;
	*back = pcolorbox[backindex].ob_spec;
}

static char index2Color(char color, word retcode, word foreground)
{
	char c = (char)retcode - 1;
	
	if (foreground)
		return ((c << 4) | (color & 0x0F));
	else
		return (c | (0xF0 & color));
}

void ColorSetup(char color, OBJECT *tree, word foresel, word forebox,
				word backsel, word backbox)
{
	char fcolor, bcolor;
	OBSPEC fobs, bobs;
	
	fcolor = color >> 4;
	bcolor = color & 0x0F;
	
	color2Obspecs(color, &fobs, &bobs);
	
	if (foresel >= 0)
		tree[foresel].ob_spec = fobs;

	if (forebox >= 0)
		tree[forebox].ob_spec.obspec.interiorcol = fcolor;

	if (backsel >= 0)
		tree[backsel].ob_spec = bobs;

	if (backbox >= 0)
		tree[backbox].ob_spec.obspec.interiorcol = bcolor;
}

char ColorSelect(char color, word foreground, OBJECT *tree,
				 word selindex, word boxindex, word circle)
{
	word retcode;
	OBSPEC obspec;

	obspec = tree[selindex].ob_spec;

	retcode = JazzSelect (tree, selindex, pcolorbox,
					 TRUE, circle? -2:0, (long *)&obspec);
	
	if (retcode > -1)
	{
		color = index2Color(color, retcode, foreground);

		if (foreground)
			ColorSetup(color, tree, selindex, boxindex, -1, -1);
		else
			ColorSetup(color, tree, -1, -1, selindex, boxindex);

		fulldraw(tree, selindex);
		fulldraw(tree, boxindex);
	}

	return color;
}

char ColorMap(char truecolor)
{
	char color;
	word max = (1 << _GemParBlk.global[10]) - 1;
	
	color = ((unsigned char)truecolor) >> 4;
	truecolor &= 0x0F;
	
	if (color > max)
		color = 1;
		
	if (truecolor > max)
		truecolor = 0;

	return (color << 4) | truecolor;
}