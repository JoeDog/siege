/**
 * Perly functions
 * Library: joedog
 * 
 * Copyright (C) 2000-2007 by
 * Jeffrey Fulmer - <jeff@joedog.org>
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
 *
 */

#ifndef PERL_H
#define PERL_H

#include <joedog/defs.h>
#include <joedog/boolean.h>

/**
 * not quite perl chomp, this function
 * hacks the newline off the end of
 * string str.
 */
char *chomp( char *str );

/**
 * trims the white space from the right 
 * of a string.
 */
char *rtrim(char *str);

/**
 * trims the white space from the left 
 * of a string.
 */
char *ltrim(char *str);

/**
 * trims the white space from the left
 * and the right sides of a string.
 */ 
char * trim(char *str);

/**
 * local library function prototyped here 
 * for split.
 */
int word_count( char pattern, char *s );

/**
 * split string *s on pattern pattern pointer
 * n_words holds the size of **
 */
char **split( char pattern, char *s, int *n_words );

/**
 * free memory allocated by split
 */
void split_free( char **split, int length ); 

/**
 * tests for empty string; warns if invalid
 */
int empty(const char *s);

#endif
