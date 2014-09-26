/**
 * Transacation logging support
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
 *--
 */
#include <setup.h>
#include <fcntl.h>
#include <log.h>
#include <data.h>
#include <locale.h>

/**
 * writes the output from siege to a formatted
 * log file.  checks if the log exists, if not
 * it creates a new formatted file and appends
 * text to it.  If a file does exist,  then it
 * simply appends to it. 
 */
void
log_transaction(DATA D)
{
  write_to_log(
    data_get_count(D),
    data_get_elapsed(D),
    data_get_megabytes(D),
    data_get_total(D),
    data_get_code(D),
    my.failed
  );
  return;
}

void
write_to_log(int count, float elapsed, int bytes, float ttime, int code, int failed)
{
  int     fd; 
  char    entry[512];
#ifdef  HAVE_LOCALTIME_R
  struct  tm keepsake;
#endif/*HAVE_LOCALTIME_R*/ 
  struct  tm *tmp;  
  time_t  now;
  size_t  len;
  char    date[65];

  now = time(NULL);
#ifdef HAVE_LOCALTIME_R
  tmp = (struct tm *)localtime_r(&now, &keepsake);
#else
  tmp = localtime(&now);
#endif/*HAVE_LOCALTIME_R*/  

  setlocale(LC_TIME, "C");
  len = strftime(date, sizeof date, "%Y-%m-%d %H:%M:%S", tmp);

  /* if the file does NOT exist then we'll create it. */
  if(my.shlog){ 
    printf( "FILE: %s\n", my.logfile ); 
    puts("You can disable this annoying message by editing");
    puts("the .siegerc file in your home directory; change");
    puts("the directive \'show-logfile\' to false." );
  }

  if (!file_exists(my.logfile)) {
    if (!create_logfile(my.logfile)) {
      NOTIFY(ERROR, "unable to create log file: %s", my.logfile);
      return;
    }
  }

  /* create the log file entry with function params. */
  snprintf(
    entry, sizeof entry, 
    "%s,%7d,%11.2f,%12u,%11.2f,%12.2f,%12.2f,%12.2f,%8d,%8d\n", 
    date, count, elapsed, bytes, ttime / count, count / elapsed, bytes / elapsed, 
    ttime / elapsed, code, failed 
  );

  /* open the log and write to file */
  if ((fd = open( my.logfile, O_WRONLY | O_APPEND, 0644 )) < 0) {
    NOTIFY(ERROR, "Unable to write to file: %s", my.logfile);
    return;
  }

  write(fd, entry, strlen(entry));
  close(fd);
 
  return;
}  

/* marks the siege.log with a user defined 
   message.  checks for the existence of a
   log and creates one if not found.      */
void
mark_log_file(char *message)
{
  int  fd;
  char entry[512];

  /* if the file does NOT exist then create it.  */
  if (!file_exists(my.logfile)) {
    if (!create_logfile(my.logfile)) {
      NOTIFY(ERROR, "unable to create log file: %s", my.logfile);
      return;
    }
  }

  /* create the log file entry */
  snprintf(entry, sizeof entry, "**** %s ****\n", message);

  if ((fd = open( my.logfile, O_WRONLY | O_APPEND, 0644 )) < 0) {
    NOTIFY(ERROR, "Unable to write to file: %s", my.logfile);
  }

  write(fd, entry, strlen(entry));
  close(fd);

  return; 
}

/**
 * returns TRUE if the file exists,
 */
BOOLEAN
file_exists(char *file)
{
  int  fd;

  /* open the file read only  */
  if((fd = open(file, O_RDONLY)) < 0){
  /* the file does NOT exist  */
    close(fd);
    return FALSE;
  } else {
  /* party on Garth... */
    close(fd);
    return TRUE;
  }

  return FALSE;
}

/**
 * return TRUE upon the successful
 * creation of the file, FALSE if not.  The
 * function adds a header at the top of the
 * file, format is comma separated text for
 * spreadsheet import.
 */
BOOLEAN
create_logfile(const char *file)
{
  int   fd;
  char *head = (char*)"      Date & Time,  Trans,  Elap Time,  Data Trans,  "
               "Resp Time,  Trans Rate,  Throughput,  Concurrent,    OKAY,   Failed\n"; 
 
  if((fd = open(file, O_CREAT | O_WRONLY, 0644)) < 0){
    return FALSE;
  }

  /* write the header to the file */
  write(fd, head, strlen(head));
  close(fd);
  
  return TRUE;
} 

