/**
 * Date Header - date calculations for Siege
 *
 * Copyright (C) 2007-2013 by
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef DATE_H
#define DATE_H

#include <time.h>
#include <joedog/boolean.h>

typedef struct DATE_T *DATE;
extern  size_t DATESIZE;

/**
 * We decided to make etag a DATE
 * because then we have one destroyer
 * that we can pass to the HASH
 */
DATE   new_date();
DATE   new_etag(char *etag);

DATE    date_destroy(DATE this);
time_t  adjust(time_t tvalue, int secs);
time_t  strtotime(const char *string);
BOOLEAN date_expired(DATE this);
char *  timetostr(const time_t *T);
char *  date_get_etag(DATE this);
char *  date_get_rfc850(DATE this);
char *  date_stamp(DATE this);
char *  date_to_string(DATE this);

#endif/*DATE_H*/
