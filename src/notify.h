/**
 * Error notification 
 * Library: joedog
 *
 * Copyright (C) 2000-2009 by
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
 */
#ifndef NOTIFY_H
#define NOTIFY_H 

#define UNCOLOR   -1
#define BLACK      0
#define RED        1
#define GREEN      2
#define YELLOW     3
#define BLUE       4
#define MAGENTA    5
#define CYAN       6
#define WHITE      7

typedef enum {
  DEBUG     = 0,
  WARNING   = 1,
  ERROR     = 2,
  FATAL     = 3
} LEVEL;

void OPENLOG(char *program_name);
void CLOSELOG(void);  
void DISPLAY(int color, const char *fmt, ...);
void SYSLOG(LEVEL L, const char *fmt, ...);
void NOTIFY(LEVEL L, const char *fmt, ...);


#endif/*NOTIFY_H*/  
