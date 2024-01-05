/*
 * redraw.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for redraw.c
 *
 * last change: 11.09.1990
 */

word rc_intersect(const word r1[4],word r2[4]);
char VDIRectIntersect(word r1[4], word r2[4]);

word pointInRect(word px,word py,word r[4]);
void flushredraw(void);
void buffredraw(WindInfo *wp,word w[4]);
void redraw(WindInfo *wp, word r1[4]);
void redrawObj(WindInfo *wp, word objnr);
void rewindow(WindInfo *wp,word upflag);
void allrewindow(word upflag);
void pathchanged(WindInfo *wp);
void fileChanged(WindInfo *wp);
void allFileChanged(word todraw);
