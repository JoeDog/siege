/**
 * Utility functions
 * Library: joedog
 *
 * Copyright (C) 2000-2009 by
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
#include <util.h>
#include <notify.h>
#include <memory.h>

/**
 * substring        returns a char pointer from
 *                  start to start + length in a
 *                  larger char pointer (buffer).
 */
char *
substring(char *str, int start, int len)
{
  int   i;
  char  *ret;
  char  *res;
  char  *ptr;
  char  *end;

//  if ((len < 1) || (start < 0) || (start > (int)strlen (str)) || start+len > (int)strlen(str))
//    return NULL;

  if ((len < 1) || (start < 0) || (start > (int)strlen (str)))
    return NULL;

  if (start+len > (int)strlen(str)) 
    len = strlen(str) - start;

  ret = xmalloc(len+1);
  res = ret;
  ptr = str;
  end = str;

  for (i = 0; i < start; i++, ptr++) ;
  for (i = 0; i < start+len; i++, end++) ;
  while (ptr < end)
    *res++ = *ptr++;

  *res = 0;
  return ret;
}

/**
 * my_random        returns a random int
 */  
int 
my_random( int max, int seed )
{
  srand( (unsigned)time( NULL ) * seed ); 
  return (int)((double)rand() / ((double)RAND_MAX + 1) * max ); 
}

void 
itoa( int n, char s[] )
{
  int i, sign;
  if(( sign = n ) < 0 )
    n = -n;
  i = 0;
  do{
    s[i++] = n % 10 + '0';
  } while(( n /= 10 ) > 0 );
  if( sign < 0  )
    s[i++] = '-';
  s[i] = '\0';
 
  reverse( s );
}

void 
reverse( char s[] )
{
  int c, i, j;
  for( i = 0, j = strlen( s )-1; i < j; i++, j-- ){
    c    = s[i];
    s[i] = s[j];
    s[j] = c;
  } /** end of for     **/
}   /** end of reverse **/

float 
elapsed_time( clock_t time )
{
  long tps = sysconf( _SC_CLK_TCK );
  return (float)time/tps;
}


