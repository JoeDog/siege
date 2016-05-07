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

char *  chomp(char *str);
char *  rtrim(char *str);
char *  ltrim(char *str);
char *  trim(char *str);
int     word_count(char pattern, char *s);
char ** split( char pattern, char *s, int *n_words );
void    split_free(char **split, int length); 
BOOLEAN empty(const char *s);

#endif
