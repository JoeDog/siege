/**
 * Siege, http regression tester / benchmark utility
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
 *
 */
#define  INTERN  1

#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#ifdef  HAVE_PTHREAD_H
# include <pthread.h>
#endif/*HAVE_PTHREAD_H*/

/*LOCAL HEADERS*/
#include <setup.h>
#include <array.h>
#include <handler.h>
#include <timer.h>
#include <browser.h>
#include <util.h>
#include <log.h>
#include <init.h>
#include <cfg.h>
#include <url.h>
#include <ssl.h>
#include <cookies.h>
#include <crew.h>
#include <data.h>
#include <version.h>
#include <memory.h>
#include <notify.h>
#include <sys/resource.h>
#ifdef __CYGWIN__
# include <getopt.h>
#else
# include <joedog/getopt.h>
#endif 

/**
 * long options, std options struct
 */
static struct option long_options[] =
{
  { "version",      no_argument,       NULL, 'V' },
  { "help",         no_argument,       NULL, 'h' },
  { "verbose",      no_argument,       NULL, 'v' },
  { "quiet",        no_argument,       NULL, 'q' },
  { "config",       no_argument,       NULL, 'C' },
  { "debug",        no_argument,       NULL, 'D' },
  { "get",          no_argument,       NULL, 'g' },
  { "print",        no_argument,       NULL, 'p' },
  { "concurrent",   required_argument, NULL, 'c' },
  { "no-parser",    no_argument,       NULL, 'N' },
  { "no-follow",    no_argument,       NULL, 'F' },
  { "internet",     no_argument,       NULL, 'i' },
  { "benchmark",    no_argument,       NULL, 'b' },
  { "reps",         required_argument, NULL, 'r' },
  { "time",         required_argument, NULL, 't' },
  { "delay",        required_argument, NULL, 'd' },
  { "log",          optional_argument, NULL, 'l' },
  { "file",         required_argument, NULL, 'f' },
  { "rc",           required_argument, NULL, 'R' }, 
  { "mark",         required_argument, NULL, 'm' },
  { "header",       required_argument, NULL, 'H' },
  { "user-agent",   required_argument, NULL, 'A' },
  { "content-type", required_argument, NULL, 'T' },
  { "json-output",  no_argument,       NULL, 'j' },
  {0, 0, 0, 0}
};

/**
 * display_version   
 * displays the version number and exits on boolean false. 
 * continue running? TRUE=yes, FALSE=no
 * return void
 */

void 
display_version(BOOLEAN b)
{
  /**
   * version_string is defined in version.c 
   * adding it to a separate file allows us
   * to parse it in configure.  
   */
  char name[128]; 

  xstrncpy(name, program_name, sizeof(name));

  if (my.debug) {
    fprintf(stderr,"%s %s: debugging enabled\n\n%s\n", uppercase(name, strlen(name)), version_string, copyright);
  } else {
    if (b == TRUE) {
      fprintf(stderr,"%s %s\n\n%s\n", uppercase(name, strlen(name)), version_string, copyright);
      exit(EXIT_SUCCESS);
    } else {
      fprintf(stderr,"%s %s\n", uppercase(name, strlen(name)), version_string);
    }
  }
}  /* end of display version */

/**
 * display_help 
 * displays the help section to STDOUT and exits
 */ 
void 
display_help()
{
  /**
   * call display_version, but do not exit 
   */
  display_version(FALSE); 
  printf("Usage: %s [options]\n", program_name);
  printf("       %s [options] URL\n", program_name);
  printf("       %s -g URL\n", program_name);
  printf("Options:\n"                    );
  puts("  -V, --version             VERSION, prints the version number.");
  puts("  -h, --help                HELP, prints this section.");
  puts("  -C, --config              CONFIGURATION, show the current config.");
  puts("  -v, --verbose             VERBOSE, prints notification to screen.");
  puts("  -q, --quiet               QUIET turns verbose off and suppresses output.");
  puts("  -g, --get                 GET, pull down HTTP headers and display the");
  puts("                            transaction. Great for application debugging.");
  puts("  -p, --print               PRINT, like GET only it prints the entire page.");
  puts("  -c, --concurrent=NUM      CONCURRENT users, default is 10");
  puts("  -r, --reps=NUM            REPS, number of times to run the test." );
  puts("  -t, --time=NUMm           TIMED testing where \"m\" is modifier S, M, or H");
  puts("                            ex: --time=1H, one hour test." );
  puts("  -d, --delay=NUM           Time DELAY, random delay before each request");
  puts("  -b, --benchmark           BENCHMARK: no delays between requests." );
  puts("  -i, --internet            INTERNET user simulation, hits URLs randomly.");
  puts("  -f, --file=FILE           FILE, select a specific URLS FILE." );
  printf("  -R, --rc=FILE             RC, specify an %src file\n",program_name);
  puts("  -l, --log[=FILE]          LOG to FILE. If FILE is not specified, the");
  printf("                            default is used: PREFIX/var/%s.log\n", program_name);
  puts("  -m, --mark=\"text\"         MARK, mark the log file with a string." );
  puts("                            between .001 and NUM. (NOT COUNTED IN STATS)");
  puts("  -H, --header=\"text\"       Add a header to request (can be many)" ); 
  puts("  -A, --user-agent=\"text\"   Sets User-Agent in request" ); 
  puts("  -T, --content-type=\"text\" Sets Content-Type in request" ); 
  puts("  -j, --json-output         JSON OUTPUT, print final stats to stdout as JSON");
  puts("      --no-parser           NO PARSER, turn off the HTML page parser");
  puts("      --no-follow           NO FOLLOW, do not follow HTTP redirects");
  puts("");
  puts(copyright);
  /**
   * our work is done, exit nicely
   */
  exit(EXIT_SUCCESS);
}

/* Check the command line for the presence of the -R or --RC switch.  We
 * need to do this separately from the other command line switches because
 * the options are initialized from the .siegerc file before the command line
 * switches are parsed. The argument index is reset before leaving the
 * function. */
void 
parse_rc_cmdline(int argc, char *argv[])
{
  int a = 0;
  strcpy(my.rc, "");
  
  while( a > -1 ){
    a = getopt_long(argc, argv, "VhvqCDNFpgl::ibr:t:f:d:c:m:H:R:A:T:j", long_options, (int*)0);
    if(a == 'R'){
      strcpy(my.rc, optarg);
      a = -1;
    }
  }
  optind = 0;
} 

/**
 * parses command line arguments and assigns
 * values to run time variables. relies on GNU
 * getopts included with this distribution.  
 */ 
void 
parse_cmdline(int argc, char *argv[])
{
  int c = 0;
  int nargs;
  while ((c = getopt_long(argc, argv, "VhvqCDNFpgl::ibr:t:f:d:c:m:H:R:A:T:j", long_options, (int *)0)) != EOF) {
  switch (c) {
      case 'V':
        display_version(TRUE);
        break;
      case 'h':
        display_help();
        exit(EXIT_SUCCESS);
      case 'D':
        my.debug = TRUE;
        break;
      case 'C':
        my.config = TRUE;
        my.get    = FALSE;
        break;
      case 'c':
        my.cusers  = atoi(optarg);
        break;
      case 'i':
        my.internet = TRUE;
        break;
      case 'b':
        my.bench    = TRUE;
        break;
      case 'd':
	/* XXX range checking? use strtol? */
        my.delay   = atof(optarg);
	if(my.delay < 0){
	  my.delay = 0; 
	}
        break;
      case 'g':
        my.get = TRUE;
        break;
      case 'p':
        my.print  = TRUE;
        my.cusers = 1;
        my.reps   = 1;
        break;
      case 'l':
        my.logging = TRUE;
        if (optarg) {
          if (strlen(optarg) > sizeof(my.logfile)) {
            fprintf(stderr, "ERROR: -l/--logfile is limited to %ld in length", sizeof(my.logfile));
            exit(1);
          }
          xstrncpy(my.logfile, optarg, strlen(optarg)+1);
        } 
        break;
      case 'm':
        my.mark    = TRUE;
        my.markstr = optarg;
        my.logging = TRUE; 
        break;
      case 'q':
        my.quiet   = TRUE;
        break;
      case 'v':
        my.verbose = TRUE;
        break;
      case 'r':
        if(strmatch(optarg, "once")){
           my.reps = -1;
        } else {
          my.reps = atoi(optarg);
        }
        break;
      case 't':
        parse_time(optarg);
        break;
      case 'f':
        if(optarg == NULL) break; /*paranoia*/
        xstrncpy(my.file, optarg, strlen(optarg)+1);
        break;
      case 'A':
        strncpy(my.uagent, optarg, 255);
        break;
      case 'T':
        strncpy(my.conttype, optarg, 255);
        break;
      case 'N':
        my.parser = FALSE;
        break;
      case 'F':
        my.follow = FALSE;
        break;
      case 'R':  
        /**
         * processed above 
         */
        break; 
      case 'H':
        {
          if(!strchr(optarg,':')) NOTIFY(FATAL, "no ':' in http-header");
          if((strlen(optarg) + strlen(my.extra) + 3) > 2048)
              NOTIFY(FATAL, "header is too large");
          strcat(my.extra,optarg);
          strcat(my.extra,"\015\012");
        }
        break;
      case 'j':
        my.json_output = TRUE;
        break;

    } /* end of switch( c )           */
  }   /* end of while c = getopt_long */
  nargs = argc - optind;
  if (nargs)
    my.url = xstrdup(argv[argc-1]); 
  if (my.get && my.url==NULL) {
    puts("ERROR: -g/--get requires a commandline URL");
    exit(1);
  }
  return;
} /* end of parse_cmdline */

private void
__signal_setup()
{
  sigset_t sigs;

  sigemptyset(&sigs);
  sigaddset(&sigs, SIGHUP);
  sigaddset(&sigs, SIGINT);
  sigaddset(&sigs, SIGALRM);
  sigaddset(&sigs, SIGTERM);
  sigaddset(&sigs, SIGPIPE);
  sigprocmask(SIG_BLOCK, &sigs, NULL);
}

private void
__config_setup(int argc, char *argv[])
{
  
  memset(&my, '\0', sizeof(struct CONFIG));

  parse_rc_cmdline(argc, argv); 
  if (init_config() < 0) { 
    exit(EXIT_FAILURE); 
  } 
  parse_cmdline(argc, argv);
  ds_module_check(); 
  
  if (my.config) {
    show_config(TRUE);    
  }

  /** 
   * Let's tap the brakes and make sure the user knows what they're doing...
   */ 
  if (my.cusers > my.limit) {
    printf("\n");
    printf("================================================================\n");
    printf("WARNING: The number of users is capped at %d.%sTo increase this\n", my.limit, (my.limit>999)?" ":"  ");
    printf("         limit, search your .siegerc file for 'limit' and change\n");
    printf("         its value. Make sure you read the instructions there...\n");
    printf("================================================================\n");
    sleep(10);
    my.cusers = my.limit;
  }
}

private LINES *
__urls_setup() 
{
  LINES * lines;

  lines          = xcalloc(1, sizeof(LINES));
  lines->index   = 0;
  lines->line    = NULL;

  if (my.url != NULL) {
    my.length = 1; 
  } else { 
    my.length = read_cfg_file(lines, my.file); 
  }

  if (my.length == 0) { 
    display_help();
  }

  return lines;
}

int 
main(int argc, char *argv[])
{
  int       i, j;
  int       result   = 0;
  void  *   status   = NULL;
  LINES *   lines    = NULL;
  CREW      crew     = NULL;
  DATA      data     = NULL;
  HASH      cookies  = NULL;
  ARRAY     urls     = new_array();
  ARRAY     browsers = new_array();
  pthread_t cease; 
  pthread_t timer;  
  pthread_attr_t scope_attr;
 
  __signal_setup();
  __config_setup(argc, argv);
  lines = __urls_setup();

  pthread_attr_init(&scope_attr);
  pthread_attr_setscope(&scope_attr, PTHREAD_SCOPE_SYSTEM);
#if defined(_AIX)
  /**
   * AIX, for whatever reason, defies the pthreads standard and  
   * creates threads detached by default. (see pthread.h on AIX) 
   */
  pthread_attr_setdetachstate(&scope_attr, PTHREAD_CREATE_JOINABLE);
#endif

#ifdef HAVE_SSL
  SSL_thread_setup();
#endif

  if (my.url != NULL) {
    URL tmp = new_url(my.url);
    url_set_ID(tmp, 0);
    if (my.get && url_get_method(tmp) != POST && url_get_method(tmp) != PUT) {
      url_set_method(tmp, my.method); 
    }
    array_npush(urls, tmp, URLSIZE); // from cmd line
  } else { 
    for (i = 0; i < my.length; i++) {
      URL tmp = new_url(lines->line[i]);
      url_set_ID(tmp, i);
      array_npush(urls, tmp, URLSIZE);
    }
  } 

  cookies = load_cookies(my.cookies);
  for (i = 0; i < my.cusers; i++) {
    char    tmp[4096];
    BROWSER B = new_browser(i);
    memset(tmp, '\0', sizeof(tmp));
    snprintf(tmp, 4096, "%d", i);
    if (cookies != NULL) {
      if (hash_get(cookies, tmp) != NULL) {
        browser_set_cookies(B, (HASH)hash_get(cookies, tmp));
      } 
    }
    if (my.reps > 0 ) {
      browser_set_urls(B, urls);
    } else {
      /**
       * Scenario: -r once/--reps=once 
       */
      int n_urls = array_length(urls);
      int per_user = n_urls / my.cusers;
      int remainder = n_urls % my.cusers;
      int begin_url = i * per_user + ((i < remainder) ? i : remainder);
      int end_url = (i + 1) * per_user + ((i < remainder) ? (i + 1) : remainder);
      ARRAY url_slice = new_array();
      for (j = begin_url; j < end_url && j < n_urls; j++) {
        URL u = array_get(urls, j);
        if (u != NULL && url_get_hostname(u) != NULL && strlen(url_get_hostname(u)) > 1) {
          array_npush(url_slice, u, URLSIZE);
        }
      }
      browser_set_urls(B, url_slice);
    }
    array_npush(browsers, B, BROWSERSIZE);
  }

  if ((crew = new_crew(my.cusers, my.cusers, FALSE)) == NULL) {
    NOTIFY(FATAL, "unable to allocate memory for %d simulated browser", my.cusers);  
  } 

  if ((result = pthread_create(&cease, NULL, (void*)sig_handler, (void*)crew)) < 0) {
    NOTIFY(FATAL, "failed to create handler: %d\n", result);
  }
  if (my.secs > 0) {
    if ((result = pthread_create(&timer, NULL, (void*)siege_timer, (void*)cease)) < 0) {
      NOTIFY(FATAL, "failed to create handler: %d\n", result);
    } 
  }

  /**
   * Display information about the siege to the user
   * and prepare for verbose output if necessary.
   */
  if (!my.get && !my.quiet) {
    fprintf(stderr, "** "); 
    display_version(FALSE);
    fprintf(stderr, "** Preparing %d concurrent users for battle.\n", my.cusers);
    fprintf(stderr, "The server is now under siege...");
    if (my.verbose) { fprintf(stderr, "\n"); }
  } 

  data = new_data();
  data_set_start(data);
  for (i = 0; i < my.cusers && crew_get_shutdown(crew) != TRUE; i++) {
    BROWSER B = (BROWSER)array_get(browsers, i);
    result = crew_add(crew, (void*)start, B);
    if (result == FALSE) { 
      my.verbose = FALSE;
      fprintf(stderr, "Unable to spawn additional threads; you may need to\n");
      fprintf(stderr, "upgrade your libraries or tune your system in order\n"); 
      fprintf(stderr, "to exceed %d users.\n", my.cusers);
      NOTIFY(FATAL, "system resources exhausted"); 
    }
  } 
  crew_join(crew, TRUE, &status);
  data_set_stop(data); 

#ifdef HAVE_SSL
  SSL_thread_cleanup();
#endif

  for (i = 0; i < ((crew_get_total(crew) > my.cusers || 
                    crew_get_total(crew) == 0) ? my.cusers : crew_get_total(crew)); i++) {
    BROWSER B = (BROWSER)array_get(browsers, i);
    data_increment_count(data, browser_get_hits(B));
    data_increment_bytes(data, browser_get_bytes(B));
    data_increment_total(data, browser_get_time(B));
    data_increment_code (data, browser_get_code(B));
    data_increment_okay (data, browser_get_okay(B));
    data_increment_fail (data, browser_get_fail(B));
    data_set_highest    (data, browser_get_himark(B));
    data_set_lowest     (data, browser_get_lomark(B));
  } crew_destroy(crew);

  pthread_usleep_np(10000);

  if (! my.quiet && ! my.get) {
    if (my.failures > 0 && my.failed >= my.failures) {
      fprintf(stderr, "%s aborted due to excessive socket failure; you\n", program_name);
      fprintf(stderr, "can change the failure threshold in $HOME/.%src\n", program_name);
    }
    fprintf(stderr, "\nTransactions:\t\t%12u hits\n",        data_get_count(data));
    fprintf(stderr, "Availability:\t\t%12.2f %%\n",          data_get_count(data)==0 ? 0 :
                                                             (double)data_get_count(data) /
                                                             (data_get_count(data)+my.failed)*100
    );
    fprintf(stderr, "Elapsed time:\t\t%12.2f secs\n",        data_get_elapsed(data));
    fprintf(stderr, "Data transferred:\t%12.2f MB\n",        data_get_megabytes(data)); /*%12llu*/
    fprintf(stderr, "Response time:\t\t%12.2f secs\n",       data_get_response_time(data));
    fprintf(stderr, "Transaction rate:\t%12.2f trans/sec\n", data_get_transaction_rate(data));
    fprintf(stderr, "Throughput:\t\t%12.2f MB/sec\n",        data_get_throughput(data));
    fprintf(stderr, "Concurrency:\t\t%12.2f\n",              data_get_concurrency(data));
    fprintf(stderr, "Successful transactions:%12u\n",        data_get_code(data)); 
    if (my.debug) {
      fprintf(stderr, "HTTP OK received:\t%12u\n",             data_get_okay(data));
    }
    fprintf(stderr, "Failed transactions:\t%12u\n",          my.failed);
    fprintf(stderr, "Longest transaction:\t%12.2f\n",        data_get_highest(data));
    fprintf(stderr, "Shortest transaction:\t%12.2f\n",       data_get_lowest(data));
    fprintf(stderr, " \n");
  }

  if (my.json_output) {
    fprintf(stderr, "\n");
    printf("{\n");
    printf("\t\"transactions\":\t\t\t%12u,\n", data_get_count(data));

    double availability;
    if (data_get_count(data) == 0) {
      availability = 0;
    } else {
      availability = (double)data_get_count(data) / (data_get_count(data) + my.failed) * 100;
    }

    printf("\t\"availability\":\t\t\t%12.2f,\n", availability);
    printf("\t\"elapsed_time\":\t\t\t%12.2f,\n", data_get_elapsed(data));
    printf("\t\"data_transferred\":\t\t%12.2f,\n", data_get_megabytes(data)); /*%12llu*/
    printf("\t\"response_time\":\t\t%12.2f,\n", data_get_response_time(data));
    printf("\t\"transaction_rate\":\t\t%12.2f,\n", data_get_transaction_rate(data));
    printf("\t\"throughput\":\t\t\t%12.2f,\n", data_get_throughput(data));
    printf("\t\"concurrency\":\t\t\t%12.2f,\n", data_get_concurrency(data));
    printf("\t\"successful_transactions\":\t%12u,\n", data_get_code(data));

    if (my.debug) {
      printf("\t\"http_ok_received\":\t\t%12u,\n", data_get_okay(data));
    }

    printf("\t\"failed_transactions\":\t\t%12u,\n", my.failed);
    printf("\t\"longest_transaction\":\t\t%12.2f,\n", data_get_highest(data));
    printf("\t\"shortest_transaction\":\t\t%12.2f\n", data_get_lowest(data));
    puts("}");
  }

  if (my.mark)    mark_log_file(my.markstr);
  if (my.logging) {
    log_transaction(data);
    if (my.failures > 0 && my.failed >= my.failures) {
      mark_log_file("siege aborted due to excessive socket failure.");
    }
  }

  /**
   * Let's clean up after ourselves....
   */
  data       = data_destroy(data);
  urls       = array_destroyer(urls, (void*)url_destroy);
  browsers   = array_destroyer(browsers, (void*)browser_destroy);
  cookies    = hash_destroy(cookies);
  my.cookies = cookies_destroy(my.cookies);

  if (my.url == NULL) {
    for (i = 0; i < my.length; i++)
      xfree(lines->line[i]);
    xfree(lines->line);
    xfree(lines);
  } else {
    xfree(lines->line);
    xfree(lines);
  }

  exit(EXIT_SUCCESS);  
} /* end of int main **/
