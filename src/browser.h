/**
 * Browser instance
 *
 * Copyright (C) 2016 by
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
#ifndef __BROWSER_H
#define __BROWSER_H

#include <joedog/defs.h>
#include <joedog/boolean.h>

typedef struct BROWSER_T *BROWSER;
extern  size_t BROWSERSIZE;

BROWSER  new_browser(int id);
BROWSER  browser_destroy(BROWSER this);
void *   start(BROWSER this);
void     browser_set_urls(BROWSER this, ARRAY urls);
void     browser_set_cookies(BROWSER this, HASH cookies);
unsigned long browser_get_hits(BROWSER this);
unsigned long long browser_get_bytes(BROWSER this);
float    browser_get_time(BROWSER this);
unsigned int browser_get_code(BROWSER this);
unsigned int browser_get_okay(BROWSER this);
unsigned int browser_get_fail(BROWSER this);
float    browser_get_himark(BROWSER this);
float    browser_get_lomark(BROWSER this);

#endif/*__BROWSER_H*/
