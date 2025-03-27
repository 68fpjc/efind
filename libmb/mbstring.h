/*
 * PROJECT C Library, X68000 PROGRAMMING INTERFACE DEFINITION
 * --------------------------------------------------------------------
 * This file is written by the Project C Library Group,  and completely
 * in public domain. You can freely use, copy, modify, and redistribute
 * the whole contents, without this notice.
 * --------------------------------------------------------------------
 * $Id: mbstring.h,v 1.3 1993/10/06 16:44:29 mura Exp $
 */

#ifndef _MBSTRING_H_
#define _MBSTRING_H_

#include <stddef.h>

#define _MBC_SINGLE   0
#define _MBC_LEAD     1
#define _MBC_TRAIL    2
#define _MBC_ILLEGAL -1

#define _MBIS16(c) ((c) & 0xff00)

int mbbtombc (int);
int mbbtype (unsigned char, int);
int mbcjistojms (int);
int mbcjmstojis (int);
int mbctombb (int);
int mbsbtype (const unsigned char *, size_t);
int mbscmp (const unsigned char *, const unsigned char *);
int mbsicmp (const unsigned char *, const unsigned char *);
int mbsncmp (const unsigned char *, const unsigned char *, size_t);
int mbsnextc (const unsigned char *);
int mbsnicmp (const unsigned char *, const unsigned char *, size_t);
size_t mbscspn (const unsigned char *, const unsigned char *);
size_t mbslen (const unsigned char *);
size_t mbsnbcnt (const unsigned char *, size_t);
size_t mbsnccnt (const unsigned char *, size_t);
size_t mbsspn (const unsigned char *, const unsigned char *);
unsigned char *mbscat (unsigned char *, const unsigned char *);
unsigned char *mbschr (const unsigned char *, int);
unsigned char *mbscpy (unsigned char *, const unsigned char *);
unsigned char *mbsdec (const unsigned char *, unsigned char *);
unsigned char *mbsdup (const unsigned char *);
unsigned char *mbsinc (unsigned char *);
unsigned char *mbslwr (unsigned char *);
unsigned char *mbsncat (unsigned char *, const unsigned char *, size_t);
unsigned char *mbsncpy (unsigned char *, const unsigned char *, size_t);
unsigned char *mbsninc (const unsigned char *, size_t);
unsigned char *mbsnset (unsigned char *, int, size_t);
unsigned char *mbspbrk (const unsigned char *, const unsigned char *);
unsigned char *mbsrchr (const unsigned char *, int);
unsigned char *mbsrev (unsigned char *);
unsigned char *mbsset (unsigned char *, int);
unsigned char *mbsspnp (const unsigned char *, const unsigned char *);
unsigned char *mbsstr (const unsigned char *, const unsigned char *);
unsigned char *mbstok (unsigned char *, const unsigned char *);
unsigned char *mbsupr (unsigned char *);

#endif
