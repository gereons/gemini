/*
 * terminal.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: functions to handle vt52 emulation
 *
 * last change: 21.09.1990
 */


#ifndef __terminal__
#define __terminal__

/* Terminal initialisieren */
char init_terminal(word *sysfonts);

/* Terminal deinitialisieren */
void exit_terminal(void);

void reinit_terminal(void);

/* Bildschirm-Buffer in der Gr”že anpassen */	
void SizeScreenBuffer(void);

/* Bildschirm-Buffer l”schen */	
void DelScreenBuffer(void);

/* Textcursor physikalisch ausschalten */
void rem_cur(void);

/* Textcursor physikalisch einschalten */
void disp_cur(void);

/* VT-52: Invers-Modus einschalten */
void rev_on(void);

/* VT-52: Invers-Modus ausschalten */
void rev_off(void);

/* VT-52: Wrap-Around einschalten */
void wrap_on(void);

/* VT-52: Wrap-Around ausschalten */
void wrap_off(void);
 
/* VT-52: Textcursor logisch einschalten */
void show_cursor(void);

/* VT-52: Textcursor logisch ausschalten */
void hide_cursor(void);

/* VT-52 Funktion: Carriage Return */
void vt_cr(void);
   
/* VT-52 Funktion: Carriage Return */
void vt_lf(void);

void vt_c_right(void);

void vt_c_left(void);

void vt_c_up(void);
    
void vt_c_down(void);

void vt_c_home(void);

void vt_cl_home(void);
   
void vt_c_set(word y, word x);

void vt_cui(void);

void vt_c_save(void);
  
void vt_c_restore(void);

/* insert line */
void vt_ins_line(void);
	
/* delete line */
void vt_del_line(void);

/* erase line */
void vt_erline(void);
 
void erase_to_linestart(void);

/* erase to line start */
void vt_erlst(void);

/* clear to end of line - subroutine */
void clr_eol(void);

/* clear to end of line */
void vt_cleol(void);
	
/* erase to end of page */
void vt_ereop(void);

/* erase to start of page */
void vt_ersop(void);

/* ein Zeichen ausgeben */
void disp_char(char ch);

/* eine Zeile ausgeben - ohne Interpretation */
void disp_string(char *str);

/* Zeichenausgabe mit Interpretation */
extern void (*term_out)(char);

extern void cur_home(void);

char escon(void);

#endif