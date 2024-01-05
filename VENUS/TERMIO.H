
#include "implimit.h"

/* Breite und Hîhe des eingestellten Zeichensatzes,
 * dessen Id usw.
 */
extern word cFontId;
extern word cFontPoints;
extern word cFontWidth, cFontHeight;
 
/* Handle fÅr die Textausgabe im Online-Modus
 */
extern word text_handle;

/* Handle fÅr alle anderen Ausgaben
 */
extern word std_handle;

/* Flag: Cursor an/aus
 */
extern char cur_on;

/* Flag: invers an/aus
 */
extern char rev_mode;

/* aktuelle Textcursorposition
 */
extern word cur_x, cur_y;

#define 	normal  	0
#define 	inverse		1

/* Bildschirm-Buffer */
extern char screen[max_lines][max_columns+1];
extern char attrib[max_lines][max_columns+1];


/* VDI initialisieren
 */
char TM_Init(word *sysfonts);

/* VDI deinitialisieren
 */
void TM_Deinit(void);

/* Terminalcursor zeichnen - XOR-Modus
 */
void draw_cursor(void);

/* Textcursor physikalisch ausschalten
 */
void rem_cur(void);

/* Textcursor physikalisch einschalten 
 */
void disp_cur(void);

void TM_DelLine(word which);

void TM_InsLine(word which);

void TM_XErase(word y, word x1, word x2);

void TM_YErase(word y1, word y2);

/* ein Zeichen ausgeben
 */
void TM_DispChar(char ch);

/* eine Zeile ausgeben
 */
void TM_DispString(char *str);

/* ganzes Fenster neuzeichnen
 */
void TM_RedrawTerminal(word clip_pxy[4]);

void TM_FreshenTerminal(void);
	
char TM_IsTopWindow(void);

void UpdateTermio(WindInfo *wp);

void TM_ForeColor(char color);
void TM_BackColor(char color);

typedef struct
{
	word columns,rows;
	word inv_type;
	word wrap;
	word tab_size,tab_destructive,back_destructive;
	char foreColor;
	char backColor;
} TermConf;

extern TermConf termconf;
