/**
 * Thread pool
 *
 * Copyright (C) 2000-2015 by
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
#include <setup.h>
#include <pthread.h>
#include <crew.h>
#include <memory.h>
#include <notify.h>
#include <joedog/defs.h>
#include <joedog/boolean.h>

private void *crew_thread(void *);

struct CREW_T
{
  int              size;
  int              maxsize;
  int              cursize;
  int              total;
  WORK             *head;
  WORK             *tail;
  BOOLEAN          block;
  BOOLEAN          closed;
  BOOLEAN          shutdown;
  pthread_t        *threads;
  pthread_mutex_t  lock;
  pthread_cond_t   not_empty;
  pthread_cond_t   not_full;
  pthread_cond_t   empty;
};

CREW
new_crew(int size, int maxsize, BOOLEAN block)
{
  int    x;
  int    c;
  CREW this;
  
  if ((this = calloc(sizeof(*this),1)) == NULL)
    return NULL;
  
  if ((this->threads = (pthread_t *)malloc(sizeof(pthread_t)*size)) == NULL)
    return NULL;

  this->size     = size;
  this->maxsize  = maxsize;
  this->cursize  = 0;
  this->total    = 0;
  this->block    = block;
  this->head     = NULL; 
  this->tail     = NULL;
  this->closed   = FALSE;  
  this->shutdown = FALSE; 

  if ((c = pthread_mutex_init(&(this->lock), NULL)) != 0)
    return NULL;
  if ((c = pthread_cond_init(&(this->not_empty), NULL)) != 0)
    return NULL;
  if ((c = pthread_cond_init(&(this->not_full), NULL)) != 0)
    return NULL;
  if ((c = pthread_cond_init(&(this->empty), NULL)) != 0)
    return NULL;

  for (x = 0; x != size; x++) {
    if ((c = pthread_create(&(this->threads[x]), NULL, crew_thread, (void *)this)) != 0) {
      switch (errno) {
        case EINVAL: { NOTIFY(ERROR, "Error creating additional threads %s:%d",     __FILE__, __LINE__);  break; }
        case EPERM:  { NOTIFY(ERROR, "Inadequate permission to create pool %s:%d",  __FILE__, __LINE__);  break; }
        case EAGAIN: { NOTIFY(ERROR, "Inadequate resources to create pool %s:%d",   __FILE__, __LINE__);  break; }
        case ENOMEM: { NOTIFY(ERROR, "Exceeded thread limit for this system %s:%d", __FILE__, __LINE__);  break; }
        default:     { NOTIFY(ERROR, "Unknown error building thread pool %s:%d",    __FILE__, __LINE__);  break; }
      } return NULL;
    } 
  }
  return this;
}

private void
*crew_thread(void *crew)
{
  int  c;
  WORK *workptr;
  CREW this = (CREW)crew;

  while (TRUE) {
    if ((c = pthread_mutex_lock(&(this->lock))) != 0) {
      NOTIFY(FATAL, "mutex lock"); 
    }
    while ((this->cursize == 0) && (!this->shutdown)) {
      if ((c = pthread_cond_wait(&(this->not_empty), &(this->lock))) != 0)
        NOTIFY(FATAL, "pthread wait");
    }

    if (this->shutdown == TRUE) {
      if ((c = pthread_mutex_unlock(&(this->lock))) != 0) {
        NOTIFY(FATAL, "mutex unlock");
      }
      pthread_exit(NULL);
    }
    workptr = this->head;

    this->cursize--;
    if (this->cursize == 0) {
      this->head = this->tail = NULL;
    } else {
      this->head = workptr->next;
    }
    if ((this->block) && (this->cursize == (this->maxsize - 1))) {
      if ((c = pthread_cond_broadcast(&(this->not_full))) != 0) {
        NOTIFY(FATAL, "pthread broadcast");
      }
    }
    if (this->cursize == 0) {
      if ((c = pthread_cond_signal(&(this->empty))) != 0){
        NOTIFY(FATAL, "pthread signal");
      }
    }
    if ((c = pthread_mutex_unlock(&(this->lock))) != 0) {
      NOTIFY(FATAL, "pthread unlock");
    }

    (*(workptr->routine))(workptr->arg);

    xfree(workptr);
  }
 
  return(NULL);
}

BOOLEAN
crew_add(CREW crew, void (*routine)(), void *arg)
{
  int c;
  WORK *workptr;

  if ((c = pthread_mutex_lock(&(crew->lock))) != 0) {
    NOTIFY(FATAL, "pthread lock");
  }
  if ((crew->cursize == crew->maxsize) && !crew->block) {
    if ((c = pthread_mutex_unlock(&(crew->lock))) != 0) {
      NOTIFY(FATAL, "pthread unlock");
    }
    return FALSE;
  }

  while ((crew->cursize == crew->maxsize ) && (!(crew->shutdown || crew->closed))) {
    if ((c = pthread_cond_wait(&(crew->not_full), &(crew->lock))) != 0) {
      NOTIFY(FATAL, "pthread wait");
    }
  }
  if (crew->shutdown || crew->closed) {
    if ((c = pthread_mutex_unlock(&(crew->lock))) != 0) {
      NOTIFY(FATAL, "pthread unlock");
    } 
    return FALSE;
  }
  if ((workptr = (WORK *)malloc(sizeof(WORK))) == NULL) {
    NOTIFY(FATAL, "out of memory");
  }
  workptr->routine = routine;
  workptr->arg     = arg;
  workptr->next    = NULL;

  if (crew->cursize == 0) {
    crew->tail = crew->head = workptr;
    if ((c = pthread_cond_broadcast(&(crew->not_empty))) != 0) {
      NOTIFY(FATAL, "pthread signal");
    }
  } else {
    crew->tail->next = workptr;
    crew->tail       = workptr;
  }

  crew->cursize++; 
  crew->total  ++;
  if ((c = pthread_mutex_unlock(&(crew->lock))) != 0) {
    NOTIFY(FATAL, "pthread unlock");
  }
  
  return TRUE;
}

BOOLEAN
crew_cancel(CREW this)
{
  int x;
  int size;

  /* XXX we store the size in a local 
     variable because crew->size gets
     whacked when we cancel threads  */
  size = this->size;

  crew_set_shutdown(this, TRUE);
  for (x = 0; x < size; x++) {
#if defined(hpux) || defined(__hpux)
    pthread_kill(this->threads[x], SIGUSR1); 
#else
    pthread_cancel(this->threads[x]); 
#endif
  }
  return TRUE;
}

BOOLEAN
crew_join(CREW crew, BOOLEAN finish, void **payload)
{
  int    x; 
  int    c;

  if ((c = pthread_mutex_lock(&(crew->lock))) != 0) {
    NOTIFY(FATAL, "pthread lock");
  }

  if (crew->closed || crew->shutdown) {
    if ((c = pthread_mutex_unlock(&(crew->lock))) != 0) {
      NOTIFY(FATAL, "pthread unlock");
    }
    return FALSE;
  }
  
  crew->closed = TRUE;

  if (finish == TRUE) {
    while ((crew->cursize != 0) && (!crew->shutdown)) {
      int rc;
      struct timespec ts;
      struct timeval tp;

      rc = gettimeofday(&tp,NULL);
      if( rc != 0 )
        perror("gettimeofday");
      ts.tv_sec = tp.tv_sec+60;
      ts.tv_nsec = tp.tv_usec*1000;
      rc = pthread_cond_timedwait(&(crew->empty), &(crew->lock), &ts);
      if (rc==ETIMEDOUT) {
        pthread_mutex_unlock(&crew->lock);
      }

      if (rc != 0) {
	NOTIFY(FATAL, "pthread wait");
      }
    }
  }

  crew->shutdown = TRUE;

  if ((c = pthread_mutex_unlock(&(crew->lock))) != 0) {
    NOTIFY(FATAL, "pthread_mutex_unlock");
  }

  if ((c = pthread_cond_broadcast(&(crew->not_empty))) != 0) {
    NOTIFY(FATAL, "pthread broadcast");
  }
  
  if ((c = pthread_cond_broadcast(&(crew->not_full))) != 0) {
    NOTIFY(FATAL, "pthread broadcast");
  }

  for (x = 0; x < crew->size; x++) {
    if ((c = pthread_join(crew->threads[x], payload)) != 0) {
      NOTIFY(FATAL, "pthread_join");
    }
  }

  return TRUE;
}

void crew_destroy(CREW crew) {
  WORK  *workptr;

  xfree(crew->threads);
  while (crew->head != NULL) {
    workptr  = crew->head; 
    crew->head = crew->head->next;
    xfree(workptr);
  }
 
  xfree(crew);
}

/**
 * getters and setters
 */
public void 
crew_set_shutdown(CREW this, BOOLEAN shutdown)
{
//  pthread_mutex_lock(&this->lock);
  this->shutdown = shutdown;
//  pthread_mutex_unlock(&this->lock);
  
  pthread_cond_broadcast(&this->not_empty);
  pthread_cond_broadcast(&this->not_full);
  pthread_cond_broadcast(&this->empty);
  return;
}

public int
crew_get_size(CREW this)
{
  return this->size;
}

public int
crew_get_total(CREW this)
{
  return this->total;
}

public BOOLEAN 
crew_get_shutdown(CREW this)
{
  return this->shutdown;
}


