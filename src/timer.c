/**
 * Siege timer support.
 *
 * Copyright (C) 2000-2014 by
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
 *
 */  
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <setup.h>
#include <notify.h>
#include <joedog/boolean.h>

void
siege_timer(pthread_t handler)
{
  int err;
  time_t now;
  struct timespec timeout;
  pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t  timer_cond  = PTHREAD_COND_INITIALIZER;

  if (time(&now) < 0) { 
    NOTIFY(FATAL, "unable to set the siege timer!"); 
  }
  timeout.tv_sec=now + my.secs;
  timeout.tv_nsec=0;

  pthread_mutex_lock(&timer_mutex); 
  for (;;) {
    err = pthread_cond_timedwait( &timer_cond, &timer_mutex, &timeout);
    if (err == ETIMEDOUT) { 
      /* timed out  */
      if(my.debug){ printf("TIMED OUT!!\n"); fflush(stdout); }
      /*if(our.shutting_down != TRUE){ pthread_kill(handler, SIGTERM); }*/
      my.verbose = FALSE;
      pthread_kill(handler, SIGTERM);
      break;  
    } else {
      continue;
    }
  }
  pthread_mutex_unlock(&timer_mutex);
  return;
}


