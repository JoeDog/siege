/**
 * Storage for siege data
 *
 * Copyright (C) 2000-2016 by
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
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <data.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif/*HAVE_SYS_TIMES_H*/

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif/*HAVE_SYS_TIME_H   */
#endif /*TIME_WITH_SYS_TIME*/


struct DATA_T
{
  float    total;  /*ttime*/
  float    available;
  float    lowest;
  float    highest;
  float    elapsed;
  clock_t  start;   
  clock_t  stop;      
  struct   tms  t_start; 
  struct   tms  t_stop;
  unsigned int  code;
  unsigned int  count;
  unsigned int  okay;
  unsigned int  fail;
  unsigned long long bytes;
};

DATA
new_data()
{
  DATA this;

  this = calloc(sizeof(*this),1);
  this->total     = 0.0;
  this->available = 0.0;
  this->count     = 0.0;
  this->okay      = 0;
  this->fail      = 0.0;
  this->lowest    =  -1;
  this->highest   = 0.0;
  this->elapsed   = 0.0;
  this->bytes     = 0.0;
  return this;
}

DATA
data_destroy(DATA this)
{
  xfree(this);
  return NULL;
} 

void
data_increment_bytes(DATA this, unsigned long bytes)
{
  this->bytes += bytes;
  return;
}

void 
data_increment_count(DATA this, unsigned long count)
{
  this->count += count;
  return;
}

void 
data_increment_total(DATA this, float total)
{
  this->total += total;
  return;
}

void 
data_increment_code(DATA this, int code)
{
  this->code += code;
  return;
}

void 
data_increment_fail(DATA this, int fail)
{
  this->fail += fail;
  return;
}

void
data_increment_okay(DATA this, int okay)
{
  this->okay += okay;
  return;
}

void
data_set_start(DATA this)
{
  this->start = times(&this->t_start);
  return;
}

void
data_set_stop(DATA this)
{
  this->stop = times(&this->t_stop);
  return;
}

void
data_set_highest(DATA this, float highest)
{
  if(this->highest < highest){
    this->highest = highest;
  }
  return;
}

void
data_set_lowest(DATA this, float lowest)
{
  if((this->lowest <= 0)||(this->lowest > lowest)){
    this->lowest = lowest;
  }
  return;
}

unsigned int
data_get_count(DATA this)
{
  return this->count;
}

unsigned int 
data_get_code(DATA this)
{
  return this->code;
}

unsigned int
data_get_fail(DATA this)
{
  return this->fail;
}

unsigned int
data_get_okay(DATA this)
{
  return this->okay;
}

float
data_get_total(DATA this)
{
  return this->total;
}

float
data_get_bytes(DATA this)
{
  return (float)this->bytes;
}

float
data_get_highest(DATA this)
{
  return this->highest;
}

float
data_get_lowest(DATA this)
{
  if(this->code){
    return this->lowest;
  } else {
    return this->code; 
  }
}

float
data_get_megabytes(DATA this)
{
  return (float)this->bytes/(1024.0*1024.0);
}

float
data_get_elapsed(DATA this)
{
  long    tps;
  clock_t time;

  time = this->stop - this->start;
  tps  = sysconf(_SC_CLK_TCK);
  this->elapsed = (float)time/tps;
  return this->elapsed;
}

float
data_get_availability(DATA this)
{
  this->available = (this->count==0)?0:((this->count/(this->count+this->fail))*100);
  return this->available;  
}

float
data_get_response_time(DATA this)
{
  if((this->total==0)||(this->count==0))
    return 0;
  return (this->total / this->count); 
}

float
data_get_transaction_rate(DATA this)
{
  if((this->count==0)||(this->elapsed==0))
    return 0;
  return (this->count / this->elapsed); 
}

float
data_get_throughput(DATA this)
{
  if(this->elapsed==0)
    return 0;
  return this->bytes / (this->elapsed * 1024.0*1024.0);
}

float
data_get_concurrency(DATA this)
{
  if(this->elapsed==0)
    return 0;
  /* total transaction time / elapsed time */
  return (this->total / this->elapsed);
}

