/*
 * @(#) Gemini\wildcard.h
 * @(#) Stefan Eissing, 08. Juni 1991
 *
 * description: Header File for wildcard.c
 */

#ifndef __G_wildcard__
#define __G_wildcard__

void doWildcard (WindInfo *wp);
word filterFile (const char* wcard, const char *fname);

#endif /* __G_wildcard__ */