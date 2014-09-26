/**
 * Log file support
 *
 * Copyright (C) 2000-2013 by
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * --
 */
#ifndef  LOG_H
#define  LOG_H
#include <stdlib.h>
#include <data.h>

void    log_transaction(DATA D);
void    write_to_log(int count, float elapsed, int bytes, float ttime, int code, int failed);
void    mark_log_file(char *message); 
BOOLEAN file_exists(char *file);
BOOLEAN create_logfile(const char *file);

#endif/*LOG_H*/

