/**
 * Signal Handling
 *
 * Copyright (C) 2013-2014 by
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
#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#ifdef  HAVE_PTHREAD_H
# include <pthread.h>
#endif/*HAVE_PTHREAD_H*/ 

#include <setup.h>
#include <handler.h> 
#include <util.h>
#include <crew.h>
#include <joedog/boolean.h>

void
spin_doctor(CREW crew)
{
  long x;
  char h[4] = {'-','\\','|','/'};

  if(my.spinner==FALSE){
    return;
  }

  for (x = 0; crew_get_total(crew) > 1 || x < 55; x++) {
    fflush(stderr);
    fprintf(stderr,"%c", h[x%4]);
    pthread_usleep_np(20000);
    fprintf(stderr, "\b"); 
  }
  return;
}

void 
sig_handler(CREW crew)
{
  int gotsig = 0; 
  sigset_t  sigs;
 
  sigemptyset(&sigs);
  sigaddset(&sigs, SIGHUP);
  sigaddset(&sigs, SIGINT);
  sigaddset(&sigs, SIGTERM);
  sigprocmask(SIG_BLOCK, &sigs, NULL);

  /** 
   * Now wait around for something to happen ... 
   */
  sigwait(&sigs, &gotsig);
  my.verbose = FALSE;
  fprintf(stderr, "\nLifting the server siege..."); 
  crew_cancel(crew);

  /**
   * The signal consistently arrives early,
   * so we artificially extend the life of
   * the siege to make up the discrepancy. 
   */
  pthread_usleep_np(501125); 
  pthread_exit(NULL);
}

