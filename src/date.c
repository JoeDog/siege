/**
 * Date calculations for siege
 *
 * Copyright (C) 2007-2014 by
 * Jeffrey Fulmer - <jeff@joedog.org>, et al. 
 *
 * Copyright (C) 1998 - 2006, Daniel Stenberg, <daniel@haxx.se>, et al.  
 * (modified - use the original: http://curl.haxx.se/)
 *
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
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h> 
#include <stdlib.h> 
#include <date.h>
#include <util.h>

#if  TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif/*TIME_WITH_SYS_TIME*/

#include <setup.h>
#include <joedog/boolean.h>

#define MAX_TIME_LEN 64

enum assume {
  DATE_MDAY,
  DATE_YEAR,
  DATE_TIME
};
 
const char * const wday[]    = {
  "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

const char * const weekday[] = {
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
};    

const char * const month[]   = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};

struct tzinfo {
  const char *name;
  int offset; /* +/- in minutes */
};

#define tDAYZONE -60          /* offset for daylight savings time */
static const struct tzinfo tz[]= {
  {"GMT",    0},              /* Greenwich Mean */
  {"UTC",    0},              /* Universal (Coordinated) */
  {"WET",    0},              /* Western European */
  {"BST",    0 tDAYZONE},     /* British Summer */
  {"WAT",   60},              /* West Africa */
  {"AST",   240},             /* Atlantic Standard */
  {"ADT",   240 tDAYZONE},    /* Atlantic Daylight */
  {"EST",   300},             /* Eastern Standard */
  {"EDT",   300 tDAYZONE},    /* Eastern Daylight */
  {"CST",   360},             /* Central Standard */
  {"CDT",   360 tDAYZONE},    /* Central Daylight */
  {"MST",   420},             /* Mountain Standard */
  {"MDT",   420 tDAYZONE},    /* Mountain Daylight */
  {"PST",   480},             /* Pacific Standard */
  {"PDT",   480 tDAYZONE},    /* Pacific Daylight */
  {"YST",   540},             /* Yukon Standard */
  {"YDT",   540 tDAYZONE},    /* Yukon Daylight */
  {"HST",   600},             /* Hawaii Standard */
  {"HDT",   600 tDAYZONE},    /* Hawaii Daylight */
  {"CAT",   600},             /* Central Alaska */
  {"AHST",  600},             /* Alaska-Hawaii Standard */
  {"NT",    660},             /* Nome */
  {"IDLW",  720},             /* International Date Line West */
  {"CET",   -60},             /* Central European */
  {"MET",   -60},             /* Middle European */
  {"MEWT",  -60},             /* Middle European Winter */
  {"MEST",  -60 tDAYZONE},    /* Middle European Summer */
  {"CEST",  -60 tDAYZONE},    /* Central European Summer */
  {"MESZ",  -60 tDAYZONE},    /* Middle European Summer */
  {"FWT",   -60},             /* French Winter */
  {"FST",   -60 tDAYZONE},    /* French Summer */
  {"EET",  -120},             /* Eastern Europe, USSR Zone 1 */
  {"WAST", -420},             /* West Australian Standard */
  {"WADT", -420 tDAYZONE},    /* West Australian Daylight */
  {"CCT",  -480},             /* China Coast, USSR Zone 7 */
  {"JST",  -540},             /* Japan Standard, USSR Zone 8 */
  {"EAST", -600},             /* Eastern Australian Standard */
  {"EADT", -600 tDAYZONE},    /* Eastern Australian Daylight */
  {"GST",  -600},             /* Guam Standard, USSR Zone 9 */
  {"NZT",  -720},             /* New Zealand */
  {"NZST", -720},             /* New Zealand Standard */
  {"NZDT", -720 tDAYZONE},    /* New Zealand Daylight */
  {"IDLE", -720},             /* International Date Line East */
};

char * 
timetostr(const time_t *T)
{
  char *line;
  struct tm *tm;

  tm   = gmtime(T);
  line = xmalloc(MAX_TIME_LEN);

  snprintf(
    line, MAX_TIME_LEN,
    "If-Modified-Since: %s, %d %s %d %d:%d:%d GMT\015\012",
    wday[tm->tm_wday],
    tm->tm_mday,
    month[tm->tm_mon],
    tm->tm_year,
    tm->tm_hour,
    tm->tm_min,
    tm->tm_sec
  );

  return line;
}

char *
timestamp()
{
  char *line;
  time_t ltime;
  struct tm *tm;

  ltime = time(NULL);
  tm    = localtime(&ltime);
  line  = xmalloc(MAX_TIME_LEN);

  strftime(line, 64, "[%a, %F %T] ", tm);
  return line;
}

time_t
adjust(time_t tvalue, int secs)
{
  struct tm *tp;
  time_t ret;

  if((ret = (tvalue != (time_t)-1))){
    tp = localtime(&tvalue);
    if(secs > INT_MAX - tp->tm_sec){
      ret = (time_t)-1;
    } else {
      tp->tm_sec  += secs;
      ret = mktime(tp);
    }
  }
  return ret;
}

/**
 * Copyright (C) 1998 - 2006, Daniel Stenberg, <daniel@haxx.se>, et al.  
 * (modified - use the original: http://curl.haxx.se/)
 */
static int checkday(char *check, size_t len)
{
  int i;
  const char * const *what;
  BOOLEAN found = FALSE;
  if(len > 3)
    what = &weekday[0];
  else
    what = &wday[0];
  for(i=0; i<7; i++) {
    if(strmatch(check, (char*)what[0])) {
      found=TRUE;
      break;
    }
    what++;
  }
  return found?i:-1;
}

/**
 * Copyright (C) 1998 - 2006, Daniel Stenberg, <daniel@haxx.se>, et al.  
 * (modified - use the original: http://curl.haxx.se/)
 */
static int checkmonth(char *check)
{
  int i;
  const char * const *what;
  BOOLEAN found = FALSE; 
  
  what = &month[0];
  for(i = 0; i < 12; i++){
    if(strmatch(check, (char*)what[0])) {
      found=TRUE;
      break;
    } 
    what++;
  }
  return found ? i : -1; /* return the offset or -1, no real offset is -1 */
}

/**
 * Copyright (C) 1998 - 2006, Daniel Stenberg, <daniel@haxx.se>, et al.  
 * (modified - use the original: http://curl.haxx.se/)
 */
static int checktz(char *check)
{
  unsigned int i;
  const struct tzinfo *what;
  BOOLEAN found = FALSE;

  what = tz;
  for(i=0; i< sizeof(tz)/sizeof(tz[0]); i++) {
    if(strmatch(check, (char*)what->name)) {
      found=TRUE;
      break;
    }
    what++;
  }
  return found ? what->offset*60 : -1;
}

/**
 * Copyright (C) 1998 - 2006, Daniel Stenberg, <daniel@haxx.se>, et al.  
 */
static void skip(const char **date) {
  /* skip everything that aren't letters or digits */
  while(**date && !isalnum((unsigned char)**date))
    (*date)++;
}

time_t
strtotime(const char *string){
  int     sec   = -1;   /* seconds          */
  int     min   = -1;   /* minutes          */
  int     hour  = -1;   /* hours            */
  int     mday  = -1;   /* day of the month */
  int     mon   = -1;   /* month            */
  int     year  = -1;   /* year             */
  int     wday  = -1;   /* day of the week  */
  int     tzoff = -1;   /* time zone offset */
  int     part  = 0;
  time_t  t     = 0;
  //time_t  now   = 0;
  struct  tm     tm;
  const   char   *date;
  const   char   *indate = string;    /* original pointer */
  enum    assume dignext = DATE_MDAY;
  BOOLEAN found = FALSE;

  /*
   *  Make sure we have a string to parse. 
   */
  if(!(string && *string))
    return(0);

  date = string;

  /**
   * this parser was more or less stolen form libcurl.
   * Copyright (C) 1998 - 2006, Daniel Stenberg, <daniel@haxx.se>, et al. 
   * http://curl.haxx.se/
   */
 
  while(*date && (part < 6)) {
    found=FALSE;

    skip(&date);
 
    if(isalpha((unsigned char)*date)) {
      /* a name coming up */
      char buf[32]="";
      size_t len;
      sscanf(date, "%31[A-Za-z]", buf);
      len = strlen(buf);

      if(wday == -1) {
        wday = checkday(buf, len);
        if(wday != -1)
          found = TRUE;
      }
      if(!found && (mon == -1)) {
        mon = checkmonth(buf);
        if(mon != -1)
          found = TRUE;
      }

      if(!found && (tzoff == -1)) {
        /* this just must be a time zone string */
        tzoff = checktz(buf);
        if(tzoff != -1)
          found = TRUE;
      }

      if(!found)
        return -1; /* bad string */

      date += len;
    }  else if(isdigit((unsigned char)*date)) {
      /* a digit */
      int val;
      char *end;
      if((sec == -1) &&
         (3 == sscanf(date, "%02d:%02d:%02d", &hour, &min, &sec))) {
        /* time stamp! */
        date += 8;
        found = TRUE;
      }
      else {
        val = (int)strtol(date, &end, 10);

        if((tzoff == -1) &&
           ((end - date) == 4) &&
           (val < 1300) &&
           (indate< date) &&
           ((date[-1] == '+' || date[-1] == '-'))) {
          /* four digits and a value less than 1300 and it is preceeded with
             a plus or minus. This is a time zone indication. */
          found = TRUE;
          tzoff = (val/100 * 60 + val%100)*60;

          /* the + and - prefix indicates the local time compared to GMT,
             this we need ther reversed math to get what we want */
          tzoff = date[-1]=='+'?-tzoff:tzoff;
        }

        if(((end - date) == 8) && (year == -1) && (mon == -1) && (mday == -1)) {
          /* 8 digits, no year, month or day yet. This is YYYYMMDD */
          found = TRUE;
          year  = val/10000;
          mon   = (val%10000)/100-1; /* month is 0 - 11 */
          mday  = val%100;
        }

        if(!found && (dignext == DATE_MDAY) && (mday == -1)) {
          if((val > 0) && (val<32)) {
            mday = val;
            found = TRUE;
          }
          dignext = DATE_YEAR;
        }

        if(!found && (dignext == DATE_YEAR) && (year == -1)) {
          year = val;
          found = TRUE;
          if(year < 1900) {
            if (year > 70)
              year += 1900;
            else
              year += 2000;
          }
          if(mday == -1)
            dignext = DATE_MDAY;
        }

        if(!found)
          return -1;

        date = end;
      }
    }
    part++;
  }
  
  if(-1 == sec)
    sec = min = hour = 0; /* no time, make it zero */

  if((-1 == mday) ||
     (-1 == mon) ||
     (-1 == year))
    /* lacks vital info, fail */
    return -1;

  /* Y238 'bug' */
  if(year > 2037)
    return 0x7fffffff;

  tm.tm_sec   = sec;
  tm.tm_min   = min;
  tm.tm_hour  = hour;
  tm.tm_mday  = mday;
  tm.tm_mon   = mon;
  tm.tm_year  = year - 1900;
  tm.tm_wday  = 0;
  tm.tm_yday  = 0;
  tm.tm_isdst = 0;

  t = mktime(&tm);

  /* time zone adjust (cast t to int to compare to negative one) */
  if(-1 != (int)t) {
    struct tm *gmt;
    long delta;
    time_t t2;

#ifdef HAVE_GMTIME_R
    /* thread-safe version */
    struct tm keeptime2;
    gmt = (struct tm *)gmtime_r(&t, &keeptime2);
    if(!gmt)
      return -1; /* illegal date/time */
    t2 = mktime(gmt);
#else
    /* It seems that at least the MSVC version of mktime() doesn't work
       properly if it gets the 'gmt' pointer passed in (which is a pointer
       returned from gmtime() pointing to static memory), so instead we copy
       the tm struct to a local struct and pass a pointer to that struct as
       input to mktime(). */
    struct tm gmt2;
    gmt = gmtime(&t); /* use gmtime_r() if available */
    if(!gmt)
      return -1; /* illegal date/time */
    gmt2 = *gmt;
    t2 = mktime(&gmt2);
#endif

    /* Add the time zone diff (between the given timezone and GMT) 
       and the diff between the local time zone and GMT. */
    delta = (long)((tzoff!=-1?tzoff:0) + (t - t2));

    if((delta>0) && (t + delta < t))
      return -1; /* time_t overflow */

    t += delta;
  }
  //now = time(NULL);
  return t;
}

#if 0
int main()
{
  const char date[] = "Tue, 20-Mar-2007 14:31:38 GMT";
  time_t t          = strtotime(date);
  time_t n = time(NULL);

  printf("%ld => %ld\n", t, n);
  return 0;
}
#endif
