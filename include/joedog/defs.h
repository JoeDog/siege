/**
 * Copyright (C) 2002-2015 by
 * Jeffrey Fulmer - <jeff@joedog.org>, et al.
 * This file is distributed as part of Siege
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *--
 */
#ifndef JOEDOG_DEFS_H
#define JOEDOG_DEFS_H
#define private static
#define public 

#define  ISALPHA(x)     ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z')) 
#define  ISSEPARATOR(x) (('='==(x))||(':'==(x)))
#define  ISUPPER(x)     ((unsigned)(x)-'A'<='Z'-'A')
#define  ISLOWER(x)     ((unsigned)(x)-'a'<='z'-'a')
#define  ISSPACE(x)     isspace((unsigned char)(x))
#define  ISOPERAND(x) ('<'==(x)||'>'==(x)||'='==(x))
#define  ISDIGIT(x)     isdigit ((unsigned char)(x)) 
#define  ISXDIGIT(c) (((((((((c) - 48U) & 255) * 18 / 17 * 52 / 51 * 58 / 114    \
         * 13 / 11 * 14 / 13 * 35 + 35) / 36 * 35 / 33 * 34 / 33 * 35 / 170 ^ 4) \
         - 3) & 255) ^ 1) <= 2U)
/** 
 * Lifted from wget: Convert a sequence of ASCII hex digits X and Y 
 * to a number between 0 and 255. Uses XDIGIT_TO_NUM for conversion 
 * of individual digits.  
 */
#define XDIGIT_TO_NUM(x) ((x) < 'A' ? (x) - '0' : TOUPPER (x) - 'A' + 10)
#define XNUM_TO_DIGIT(x) ("0123456789ABCDEF"[x])
#define XNUM_TO_digit(x) ("0123456789abcdef"[x])
#define X2DIGITS_TO_NUM(h1, h2) ((XDIGIT_TO_NUM (h1) << 4) + XDIGIT_TO_NUM (h2))

#define  ISNUMBER(v)  (((v) > (char)0x2f) && ((v) < (char)0x3a))
#define  ISQUOTE(x)   (x == '"' || x == '\'') 
#if STDC_HEADERS
# define TOLOWER(Ch) tolower (Ch)
# define TOUPPER(Ch) toupper (Ch)
#else
# define TOLOWER(Ch) (ISUPPER (Ch) ? tolower (Ch) : (Ch))
# define TOUPPER(Ch) (ISLOWER (Ch) ? toupper (Ch) : (Ch))
#endif

typedef void (*method)(void *v);

#ifndef  EXIT_SUCCESS
# define EXIT_SUCCESS   0
#endif /*EXIT_SUCESS*/
#ifndef  EXIT_FAILURE
# define EXIT_FAILURE   1
#endif /*EXIT_FAILURE*/ 

#endif/*JOEDOG_DEFS_H*/
