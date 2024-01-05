/*
 * @(#) Gemini\Termio.c
 *
 * project: Gemini
 *
 * author: Arnd Beissner (changed by gs & se)
 *
 * description: basic functions for mupfel window
 *
 * last change: 07.02.1991
 */


#include <vdi.h>
#include <aes.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <tos.h>

#include "vs.h"
#include "wintool.h"
#include "util.h"
#include "termio.h"

store_sccs_id(termio);

/* externals
 */
extern word work_in[11];
extern word work_out[57];

/*
 * interne Variablen
 */
 
GRECT work;

TermConf termconf;

word win_handle;

/* Breite und Hîhe des eingestellten Zeichensatzes,
 * dessen Id usw.
 */
word cFontId;
word cFontPoints;
word cFontWidth, cFontHeight;
 
/* Handle fÅr die Textausgabe im Online-Modus
 */
word text_handle;

/* Handle fÅr alle anderen Ausgaben 
 */
word std_handle;

/* Handle fÅr den Textcursor
 */
static word cur_handle;

/* Flag: Cursor an/aus
 */
char cur_on;

/* Flag: invers an/aus
 */
char rev_mode;

/* aktuelle Textcursorposition
 */
word cur_x, cur_y;

#define 	normal  	0
#define 	inverse		1

/* Bildschirm-Buffer
 */
char screen[max_lines][max_columns+1];
char attrib[max_lines][max_columns+1];


void TM_GetCursor(int *x, int *y)
{
	*x = cur_x;
	*y = cur_y;
}

/* VDI initialisieren */
char TM_Init(word *sysfonts)
{
	word dummy;
	word aes_handle;
	word pxy[4];

	aes_handle = graf_handle(&dummy, &dummy, &dummy, &dummy);
	
	text_handle = aes_handle;
	v_opnvwk(work_in, &text_handle, work_out);
	if (text_handle <= 0)
		return FALSE;

	std_handle = aes_handle;
	v_opnvwk(work_in, &std_handle, work_out);
	if (std_handle <= 0)
	{
		v_clsvwk(text_handle);
		return FALSE;
	}

	cur_handle = aes_handle;
	v_opnvwk(work_in, &cur_handle, work_out);
	if (cur_handle <= 0)
	{
		v_clsvwk(text_handle);
		v_clsvwk(std_handle);
		return FALSE;
	}

	/* Anzahl der System-Fonts
	 */
	*sysfonts = work_out[10];
	
	/* Defaults fÅr die Textcursor-VWKS setzen */
	vswr_mode(cur_handle, 3);
	vsf_interior(cur_handle, 1);
	vsf_color(cur_handle, 1);

	termconf.backColor = 1;
	termconf.foreColor = 1;
    
	pxy[0] = 0;
	pxy[1] = 0;
	pxy[2] = work_out[0];
	pxy[3] = work_out[1];
	vs_clip(text_handle, 1, pxy);
	vs_clip(std_handle, 1, pxy);
	vst_alignment(std_handle, 0, 5, &dummy, &dummy);
	vsf_interior(std_handle, 1);
	
	/* Defaults fÅr die Online-Textausgabe-VWKS setzen */
	vst_alignment(text_handle, 0, 5, &dummy, &dummy);
	vswr_mode(text_handle, 1);
	vst_color(text_handle, 1);
	vsf_interior(text_handle, 1);
	
	cur_on = TRUE;
	return TRUE;
}


/* VDI deinitialisieren */
void TM_Deinit(void)
{
	v_clsvwk(std_handle);
	v_clsvwk(text_handle);
	v_clsvwk(cur_handle);
}


void TM_ForeColor(char color)
{
	termconf.foreColor = color - '0';
/*
	vst_color(text_handle, termconf.foreColor);
	vsf_color(cur_handle, termconf.foreColor);
*/
}

void TM_BackColor(char color)
{
	termconf.backColor = color - '0';
/*
	vsf_color(std_handle, termconf.backColor);
*/
}

/* Terminalcursor zeichnen - XOR-Modus */
void draw_cursor(void)
{
    word pxy[4], rect[4];
    word count;

    pxy[0] = work.g_x + cFontWidth * cur_x;
    pxy[1] = work.g_y + cFontHeight * cur_y;
    pxy[2] = pxy[0] + cFontWidth - 1;
    pxy[3] = pxy[1] + cFontHeight - 1;
    
    count = 0;
    while (WT_GetRect(count++, rect))
	{
		WT_Clip(cur_handle, std_handle, text_handle, rect);
		vr_recfl(cur_handle, pxy); 
	}
}


/* Textcursor physikalisch ausschalten */
void rem_cur(void)
{
    if (cur_on==1) draw_cursor();
}
    

/* Textcursor physikalisch einschalten */
void disp_cur(void)
{
    if (cur_on==1) draw_cursor();
}


/* vereinfachte Bitblit-Funktion */
static void bitblit(word pxy[8])
{
	MFDB src,dest;
    
	src.fd_addr = dest.fd_addr = NULL;
	src.fd_stand = dest.fd_stand = 0;
	vro_cpyfm(std_handle, 3, pxy, &src, &dest); 
}


static void print_line(word line, word yk)
{
	word x, pxy[4];
	char lch[2];

	screen[line][termconf.columns] = '\0';
	v_gtext(text_handle, work.g_x, yk, screen[line]);
    for (x = 0; x < termconf.columns; x++)
   		if (attrib[line][x] == inverse)
		{
   			switch (termconf.inv_type)
			{
	            case 0 : pxy[0] = work.g_x + x * cFontWidth;
	            		 pxy[1] = yk;
	            		 pxy[2] = pxy[0] + cFontWidth - 1;
	                	 pxy[3] = yk + cFontHeight - 1;
	                	 vswr_mode(text_handle, 3);
		    			 vr_recfl(text_handle, pxy);
	                	 vswr_mode(text_handle, 1);
	           			 break;
	           	case 1 : lch[0] = screen[line][x];
	           			 lch[1] = 0;
	           			 vst_effects(text_handle, 8);
	                	 v_gtext(text_handle, 
	                	 	work.g_x + x * cFontWidth, yk, lch);
	                	 vst_effects(text_handle, 0);
	                	 break;
	            case 2 : lch[0] = screen[line][x];
	           			 lch[1] = 0;
	           			 vst_effects(text_handle,1);
	                	 v_gtext(text_handle,
	                	 	work.g_x + x * cFontWidth, yk, lch);
	                	 vst_effects(text_handle, 0);
	                	 break;
            }
        }
}


void TM_DelLine(word which)
{
	word pxy[8], rect[4];
    word line, yk, count;

    vswr_mode(std_handle, 1);
	vsf_interior(std_handle, 1);
    vsf_color(std_handle, 0);

	count = 0;
	while (WT_GetRect(count++, rect))
    {
        if (rect[3] - work.g_y >= which * cFontHeight)
       	{
	        WT_Clip(std_handle, text_handle, cur_handle, rect);
	        WT_Clip(text_handle, std_handle, cur_handle, rect);

	        /* scroll the visible area */
			pxy[0] = pxy[4] = rect[0];
			pxy[2] = pxy[6] = rect[2];
	        if (rect[3] - rect[1] > cFontHeight)
        	{
				pxy[5] = work.g_y + which * cFontHeight;
				if (pxy[5] < rect[1])
					pxy[5] = rect[1];

				pxy[7] = rect[3] - cFontHeight;
				pxy[1] = pxy[5] + cFontHeight;
				pxy[3] = rect[3];
				bitblit(pxy);
			}

			/* die sichtbar gewordene(n) Zeile(n) ausgeben */
	        line = (rect[3] - work.g_y) / cFontHeight;
	        yk = line * cFontHeight + work.g_y;
	        if (rect[3] != work.g_y + work.g_h - 1)
	        	print_line(line-1, yk-cFontHeight);
       		print_line(line, yk);

			pxy[3] = work.g_y + work.g_h - 1;
			pxy[1] = pxy[3] - cFontHeight + 1;
			vr_recfl(std_handle, pxy);
		}
	}
}


void TM_InsLine(word which)
{
	word pxy[8], rect[4];
    word line, yk, count;

    vswr_mode(std_handle, 1);
	vsf_interior(std_handle, 1);
    vsf_color(std_handle, 0);

   	count = 0;
    while (WT_GetRect(count++, rect))
	{
        if (rect[3] - work.g_y >= which * cFontHeight)
       	{
	        WT_Clip(std_handle, text_handle, cur_handle, rect);

	        /* scroll the visible area */
			pxy[0] = pxy[4] = rect[0];
			pxy[2] = pxy[6] = rect[2];
	        if (rect[3] - rect[1] > cFontHeight)
        	{
				pxy[1] = work.g_y + which * cFontHeight;
				if (pxy[1] < rect[1])
					pxy[1] = rect[1];
				pxy[3] = rect[3] - cFontHeight;
	
				pxy[5] = pxy[1] + cFontHeight;
				pxy[7] = pxy[3] + cFontHeight;
				bitblit(pxy);
			}

			/* die sichtbar gewordene(n) Zeile(n) ausgeben */
	        line = (rect[3] - work.g_y) / cFontHeight;
	        yk = line * cFontHeight + work.g_y;
        	print_line(line-1, yk - cFontHeight);
        	print_line(line, yk);
			
			pxy[1] = work.g_y + which * cFontHeight;
			pxy[3] = pxy[1] + cFontHeight - 1;
			vr_recfl(std_handle, pxy);
		}
	}
}


void TM_YErase(word y1, word y2)
{
    word rect[4], pxy[4];
	word count;

	vswr_mode(std_handle, 1);
   	vsf_interior(std_handle, 1);
   	vsf_color(std_handle, 0);
   	pxy[0] = work.g_x;
   	pxy[1] = work.g_y + cFontHeight * y1;
   	pxy[2] = work.g_x + work.g_w - 1;
   	pxy[3] = work.g_y + cFontHeight * (y2+1) - 1;
   	
   	count = 0;
    while (WT_GetRect(count++, rect)) 
    {
        WT_Clip(std_handle, text_handle, cur_handle, rect);
    	vr_recfl(std_handle, pxy);
  	}
}


void TM_XErase(word y, word x1, word x2)
{
    word rect[4], pxy[4];
    word count;

	vswr_mode(std_handle, 1);
   	vsf_interior(std_handle, 1);
   	vsf_color(std_handle, 0);
   	pxy[0] = work.g_x + x1 * cFontWidth;
   	pxy[1] = work.g_y + y * cFontHeight;
   	pxy[2] = work.g_x + x2 * cFontWidth - 1;
   	pxy[3] = pxy[1] + cFontHeight - 1;

   	count = 0;
    while (WT_GetRect(count++, rect))
    {
        WT_Clip(std_handle, text_handle, cur_handle, rect);
    	vr_recfl(std_handle, pxy);
   	}
}


/* ein Zeichen ausgeben */
void TM_DispChar(char ch)
{
	unsigned int lch;
	word pxy[4];
	word count;

	count = 0;
	while (WT_GetRect(count++, pxy)) 
    {
        word xk, yk;
        
        WT_Clip(text_handle, std_handle, cur_handle, pxy);

	    /* Zeichen ausgeben */
		xk = work.g_x + cur_x * cFontWidth;
		yk = work.g_y + cur_y * cFontHeight;
	    
		lch = ((unsigned int)ch) << 8;
		if (!rev_mode)
			v_gtext(text_handle, xk, yk, (char *)&lch);
		else
    	{
	        switch(termconf.inv_type)
      		{
				case 0 : v_gtext(text_handle, xk, yk, (char *)&lch);
						 pxy[0] = xk;
	            		 pxy[1] = yk;
	            		 pxy[2] = xk + cFontWidth - 1;
	                	 pxy[3] = yk + cFontHeight - 1;
	                	 vswr_mode(text_handle, 3);
		    			 vr_recfl(text_handle, pxy);
	                	 vswr_mode(text_handle, 1);
	           			 break;
	           	case 1 : vst_effects(text_handle, 8);
	                	 v_gtext(text_handle, xk, yk, (char *)&lch);
	                	 vst_effects(text_handle, 0);
	                	 break;
	            case 2 : vst_effects(text_handle, 1);
	                	 v_gtext(text_handle, xk, yk, (char *)&lch);
	                	 vst_effects(text_handle, 0);
	                	 break;
			}
    	}
    }
}


/* eine Zeile ausgeben */
void TM_DispString(char *str)
{
	word pxy[4];
	word count;

	count = 0;
	while (WT_GetRect(count++, pxy))
    {
        WT_Clip(text_handle, std_handle, cur_handle, pxy);

		/* Zeile ausgeben */
		if (!rev_mode)
		{
			v_gtext(text_handle,
					work.g_x + cur_x * cFontWidth,
					work.g_y + cur_y * cFontHeight,
					str);
		}
		else
    	{
	        switch(termconf.inv_type)
      		{
				case 0 : v_gtext(text_handle,
								 work.g_x + cur_x * cFontWidth,
								 work.g_y + cur_y * cFontHeight,
								 str);
						 pxy[0] = work.g_x + cur_x * cFontWidth;
	            		 pxy[1] = work.g_y + cur_y * cFontHeight;
						 pxy[2] = pxy[0] + 
						 	cFontWidth * (word)strlen(str) - 1;
	                	 pxy[3] = pxy[1] + cFontHeight - 1;
	                	 vswr_mode(text_handle, 3);
		    			 vr_recfl(text_handle, pxy);
	                	 vswr_mode(text_handle, 1);
	           			 break;
	           	case 1 : vst_effects(text_handle, 8);
	                	 v_gtext(text_handle,
	                	         work.g_x + cur_x * cFontWidth,
	                	         work.g_y + cur_y * cFontHeight,
								 str);
	                	 vst_effects(text_handle, 0);
	                	 break;
	            case 2 : vst_effects(text_handle, 1);
	                	 v_gtext(text_handle,
	                	         work.g_x + cur_x * cFontWidth,
	                	         work.g_y + cur_y * cFontHeight,
								 str);
	                	 vst_effects(text_handle, 0);
	                	 break;
			}
    	}
    }
}


/* ganzes Fenster neuzeichnen */
void TM_RedrawTerminal(word clip_pxy[4])
{
	word dummy;
    word pxy[4];
    register word y,x,i;
    register word yk;
    word start_line,end_line, len;
    char *ch_p, *temp;
    char ach[2], ch;

	vs_clip(std_handle, 1, clip_pxy);

    /* redraw background */ 
    pxy[0] = work.g_x;
    pxy[1] = work.g_y;
    pxy[2] = work.g_x + work.g_w - 1;
    pxy[3] = work.g_y + work.g_h - 1;
    vswr_mode(std_handle, 1);
    vsf_interior(std_handle, 1);
    vsf_color(std_handle, 0);
    vr_recfl(std_handle, pxy);

    /* redraw text area */
    vst_alignment(std_handle, 0, 5, &dummy, &dummy);
    vsf_interior(std_handle, 1);
    vsf_color(std_handle, 1);
    vsf_perimeter(std_handle, 0);
    vswr_mode(std_handle, 1);
    ach[1] = 0;

    /* berechnen, welcher Teil sichtbar ist */
    start_line = (clip_pxy[1] - work.g_y) / cFontHeight;
	if (start_line < 0)
		start_line = 0;
	end_line = termconf.rows - 1;
		
    yk = work.g_y + start_line * cFontHeight;
    for (y = start_line; y <= end_line; y++)
    {
        /* erstes non-space-Zeichen der Zeile finden */
        ch_p = screen[y];
        x = 0;
        while ((*ch_p == ' ') && (*ch_p != 0))
       	{
        	ch_p++;
        	x++;
       	}

		if (*ch_p != 0)
		{
			/* letztes non-space-Zeichen der Zeile finden */
			len = (word)strlen(ch_p);
			temp = ch_p + len - 1;
			for (i = 0; i < len; i++)
			{
				if (*temp != ' ')
					break;
				temp--;
			}
			
			if (i < len)
			{
				ch = *(temp + 1);
				*(temp + 1) = 0;
	   	    	v_gtext(std_handle,
	   	    		work.g_x + x * cFontWidth, yk, ch_p);
	   	    	*(temp+1) = ch;
	    	}
		}

        ch_p = attrib[y];
        for (x = 0; x < termconf.columns; x++)
			if (*ch_p++ == inverse)
            {
                switch(termconf.inv_type)
                {
                	case 0 :	vswr_mode(std_handle, 3);
                			 	pxy[0] = work.g_x + x * cFontWidth;
                				pxy[1] = yk;
                				pxy[2] = pxy[0] + cFontWidth - 1;
                				pxy[3] = yk + cFontHeight - 1;
                				v_bar(std_handle, pxy);
                				vswr_mode(std_handle, 1);
                				break;
                	case 1 :	ach[0] = screen[y][x];
                				vst_effects(std_handle, 8);
                			 	v_gtext(std_handle,
                			 		work.g_x + x * cFontWidth, yk, ach);
                			 	vst_effects(std_handle, 0);
                			 	break;
                	case 2 :	ach[0] = screen[y][x];
                				vst_effects(std_handle, 1);
                			 	v_gtext(std_handle,
                			 		work.g_x + x * cFontWidth, yk, ach);
                			 	vst_effects(std_handle, 0);
                			 	break;
				}
            }
        yk += cFontHeight;
    }
    
    /* Cursor zeichnen, falls eingeschaltet */
    if (cur_on == 1)
    {
	    vswr_mode(std_handle, 3);
	    vsf_interior(std_handle, 1);
	    vsf_color(std_handle, 1);
	    
		pxy[0] = work.g_x + cFontWidth * cur_x;
	   	pxy[1] = work.g_y + cFontHeight * cur_y;
	    pxy[2] = pxy[0] + cFontWidth - 1;
	    pxy[3] = pxy[1] + cFontHeight - 1;
		vr_recfl(std_handle, pxy); 
	}
}


void TM_FreshenTerminal(void)
{
    word Rect[4],count;

	WT_BuildRectList(win_handle);

	count = 0;
    while (WT_GetRect(count++, Rect)) 
        TM_RedrawTerminal(Rect);
}

