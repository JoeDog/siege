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
#include <client.h>
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
  { "concurrent",   required_argument, NULL, 'c' },
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

  memset(name, 0, sizeof name);
  strncpy(name, program_name, strlen(program_name));

  if(my.debug){
    fprintf(stderr,"%s %s: debugging enabled\n\n%s\n", uppercase(name, strlen(name)), version_string, copyright);
  } else {
    if(b == TRUE){
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
  puts("  -c, --concurrent=NUM      CONCURRENT users, default is 10");
  puts("  -i, --internet            INTERNET user simulation, hits URLs randomly." );
  puts("  -b, --benchmark           BENCHMARK: no delays between requests." );
  puts("  -t, --time=NUMm           TIMED testing where \"m\" is modifier S, M, or H" );
  puts("                            ex: --time=1H, one hour test." );
  puts("  -r, --reps=NUM            REPS, number of times to run the test." );
  puts("  -f, --file=FILE           FILE, select a specific URLS FILE." );
  printf("  -R, --rc=FILE             RC, specify an %src file\n",program_name);
  puts("  -l, --log[=FILE]          LOG to FILE. If FILE is not specified, the");
  printf("                            default is used: PREFIX/var/%s.log\n", program_name);
  puts("  -m, --mark=\"text\"         MARK, mark the log file with a string." );
  puts("  -d, --delay=NUM           Time DELAY, random delay before each requst");
  puts("                            between .001 and NUM. (NOT COUNTED IN STATS)");
  puts("  -H, --header=\"text\"       Add a header to request (can be many)" ); 
  puts("  -A, --user-agent=\"text\"   Sets User-Agent in request" ); 
  puts("  -T, --content-type=\"text\" Sets Content-Type in request" ); 
  puts("");
  puts(copyright);
  /**
   * our work is done, exit nicely
   */
  exit( EXIT_SUCCESS );
}

/* Check the command line for the presence of the -R or --RC switch.  We
 * need to do this seperately from the other command line switches because
 * the options are initialized from the .siegerc file before the command line
 * switches are parsed. The argument index is reset before leaving the
 * function. */
void 
parse_rc_cmdline(int argc, char *argv[])
{
  int a = 0;
  strcpy(my.rc, "");
  
  while( a > -1 ){
    a = getopt_long(argc, argv, "VhvqCDgl::ibr:t:f:d:c:m:H:R:A:T:", long_options, (int*)0);
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
  while ((c = getopt_long(argc, argv, "VhvqCDgl::ibr:t:f:d:c:m:H:R:A:T:", long_options, (int *)0)) != EOF) {
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
      case 'l':
        my.logging = TRUE;
        if (optarg) {
          my.logfile[strlen(optarg)] = '\0';
          strncpy(my.logfile, optarg, strlen(optarg));
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
        memset(my.file, 0, sizeof(my.file));
        if(optarg == NULL) break; /*paranoia*/
        strncpy(my.file, optarg, strlen(optarg));
        break;
      case 'A':
        strncpy(my.uagent, optarg, 255);
        break;
      case 'T':
        strncpy(my.conttype, optarg, 255);
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

int 
main(int argc, char *argv[])
{
  int            x; 
  int            j = 0;
  int            result;
  DATA           D    = new_data();
  ARRAY          urls = new_array();
  CREW           crew;  
  HASH           cookies;
  LINES          *lines;   
  CLIENT         *client; 
  pthread_t      cease; 
  pthread_t      timer;  
  pthread_attr_t scope_attr; 
  void *statusp;
  sigset_t sigs;

  sigemptyset(&sigs);
  sigaddset(&sigs, SIGHUP);
  sigaddset(&sigs, SIGINT);
  sigaddset(&sigs, SIGALRM);
  sigaddset(&sigs, SIGTERM);
  sigaddset(&sigs, SIGPIPE);
  sigprocmask(SIG_BLOCK, &sigs, NULL);

  lines = xcalloc(1, sizeof *lines);
  lines->index   = 0;
  lines->line    = NULL;

  memset(&my, 0, sizeof(struct CONFIG));

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

  if (my.url != NULL) {
    my.length = 1; 
  } else { 
    my.length = read_cfg_file(lines, my.file); 
  }

  if (my.length == 0) { 
    display_help();
  }

  /* memory allocation for threads and clients */
  client = xcalloc(my.cusers, sizeof(CLIENT));
  if ((crew = new_crew(my.cusers, my.cusers, FALSE)) == NULL) {
    NOTIFY(FATAL, "unable to allocate memory for %d simulated browser", my.cusers);  
  }

  /** 
   * determine the source of the url(s),
   * command line or file, and add them
   * to the urls struct.
   */

  if (my.url != NULL) {
    URL tmp = new_url(my.url);
    url_set_ID(tmp, 0);
    if (my.get && url_get_method(tmp) != POST && url_get_method(tmp) != PUT) {
      url_set_method(tmp, my.method); 
    }
    array_npush(urls, tmp, URLSIZE); // from cmd line
  } else { 
    for (x = 0; x < my.length; x++) {
      URL tmp = new_url(lines->line[x]);
      url_set_ID(tmp, x);
      array_npush(urls, tmp, URLSIZE);
    }
  } 

  /**
   * display information about the siege
   * to the user and prepare for verbose 
   * output if necessary.
   */
  if (!my.get && !my.quiet) {
    fprintf(stderr, "** "); 
    display_version(FALSE);
    fprintf(stderr, "** Preparing %d concurrent users for battle.\n", my.cusers);
    fprintf(stderr, "The server is now under siege...");
    if (my.verbose) { fprintf(stderr, "\n"); }
  }

  /**
   * record start time before spawning threads
   * as the threads begin hitting the server as
   * soon as they are created.
   */
  data_set_start(D);

  /**
   * for each concurrent user, spawn a thread and
   * loop until condition or pthread_cancel from the
   * handler thread.
   */
  pthread_attr_init(&scope_attr);
  pthread_attr_setscope(&scope_attr, PTHREAD_SCOPE_SYSTEM);
#if defined(_AIX)
  /* AIX, for whatever reason, defies the pthreads standard and  *
   * creates threads detached by default. (see pthread.h on AIX) */
  pthread_attr_setdetachstate(&scope_attr, PTHREAD_CREATE_JOINABLE);
#endif

  /** 
   * invoke OpenSSL's thread safety
   */
#ifdef HAVE_SSL
  SSL_thread_setup();
#endif

  /**
   * create the signal handler and timer;  the
   * signal handler thread (cease) responds to
   * ctrl-C (sigterm) and the timer thread sends
   * sigterm to cease on time out.
   */
  if ((result = pthread_create(&cease, NULL, (void*)sig_handler, (void*)crew)) < 0) {
    NOTIFY(FATAL, "failed to create handler: %d\n", result);
  }
  if (my.secs > 0) {
    if ((result = pthread_create(&timer, NULL, (void*)siege_timer, (void*)cease)) < 0) {
      NOTIFY(FATAL, "failed to create handler: %d\n", result);
    } 
  }

  /**
   * loop until my.cusers and create a corresponding thread...
   */  
  cookies = load_cookies(my.cookies);
  for (x = 0; x < my.cusers && crew_get_shutdown(crew) != TRUE; x++) {
    char tmp[4096];
    snprintf(tmp, 4096, "%d", x);
    client[x].id              = x; 
    client[x].bytes           = 0;
    client[x].time            = 0.0;
    client[x].hits            = 0;
    client[x].code            = 0;
    client[x].ok200           = 0;   
    client[x].fail            = 0; 
    if (cookies != NULL) {
      if (hash_get(cookies, tmp) != NULL) {
        client[x].cookies = hoh_get(cookies, tmp);
      } 
    }
    if (my.reps > 0 ) {
      /** 
       * Traditional -r/--reps where each user
       * loops through every URL in the file. 
       */
      client[x].urls = urls;
    } else {
      /**
       * -r once/--reps=once where each URL
       * in the file is hit only once...
       */
      int   len = (array_length(urls)/my.cusers); 
      ARRAY tmp = new_array();
      for ( ; j < ((x+1) * len) && j < (int)array_length(urls); j++) {
        URL u = array_get(urls, j);
        if (u != NULL && url_get_hostname(u) != NULL && strlen(url_get_hostname(u)) > 1) {
          array_npush(tmp, array_get(urls, j), URLSIZE);    
        }
      } 
      client[x].urls = tmp;
    }
    client[x].auth.www        = 0;
    client[x].auth.proxy      = 0;
    client[x].auth.type.www   = BASIC;
    client[x].auth.type.proxy = BASIC;
    client[x].rand_r_SEED     = urandom();
    result = crew_add(crew, (void*)start_routine, &(client[x]));
    if (result == FALSE) { 
      my.verbose = FALSE;
      fprintf(stderr, "Unable to spawn additional threads; you may need to\n");
      fprintf(stderr, "upgrade your libraries or tune your system in order\n"); 
      fprintf(stderr, "to exceed %d users.\n", my.cusers);
      NOTIFY(FATAL, "system resources exhausted"); 
    }
  } /* end of for pthread_create */
#if 0
  if (cookies != NULL) {
    int i, j;
    char **keys = hash_get_keys(cookies);
    for (i = 0; i < hash_get_entries(cookies) || i < my.cusers; i ++){
      char **k;
      HASH hash = hoh_get(cookies, keys[i]);
      k = hash_get_keys(hash);
      printf("%s HAS %d ENTRIES\n", keys[i], hash_get_entries(hash));
      for (j = 0; j < hash_get_entries(hash); j ++){
        char *tmp = (char*)hash_get(hash, k[j]);
        printf("%s: %s => %s\n", keys[i], k[j], (tmp==NULL)?"NULL":tmp);
      }
      hash_destroy(hash);
    }
  }
#endif

  crew_join(crew, TRUE, &statusp);

#ifdef HAVE_SSL
  SSL_thread_cleanup();
#endif

  /**
   * collect all the data from all the threads that
   * were spawned by the run.
   */
  for (x = 0; x < ((crew_get_total(crew) > my.cusers || 
                    crew_get_total(crew)==0 ) ? my.cusers : crew_get_total(crew)); x++) {
    data_increment_count(D, client[x].hits);
    data_increment_bytes(D, client[x].bytes);
    data_increment_total(D, client[x].time);
    data_increment_code (D, client[x].code);
    data_increment_ok200(D, client[x].ok200);
    data_increment_fail (D, client[x].fail);
    data_set_highest    (D, client[x].himark);
    data_set_lowest     (D, client[x].lomark);
    client[x].rand_r_SEED = urandom();
  } /* end of stats accumulation */
  
  /**
   * record stop time
   */
  data_set_stop(D);

  /**
   * cleanup crew
   */ 
  crew_destroy(crew);

  for (x = 0; x < my.cusers; x++) {
    // XXX: TODO
    //digest_challenge_destroy(client[x].auth.wwwchlg);
    //digest_credential_destroy(client[x].auth.wwwcred);
    //digest_challenge_destroy(client[x].auth.proxychlg);
    //digest_credential_destroy(client[x].auth.proxycred);
  }
  array_destroy(my.lurl);
  xfree(client);

  if (my.get) {
    if (data_get_ok200(D) > 0) {
      exit(EXIT_SUCCESS);
    } else {
      if (!my.quiet) echo("[done]\n");
      exit(EXIT_FAILURE);
    }
  }

  /**
   * take a short nap  for  cosmetic  effect
   * this does NOT affect performance stats.
   */
  pthread_usleep_np(10000);
  if (my.verbose)
    fprintf(stderr, "done.\n");
  else
    fprintf(stderr, "\b      done.\n");

  /**
   * prepare and print statistics.
   */
  if (my.failures > 0 && my.failed >= my.failures) {
    fprintf(stderr, "%s aborted due to excessive socket failure; you\n", program_name);
    fprintf(stderr, "can change the failure threshold in $HOME/.%src\n", program_name);
  }
  fprintf(stderr, "\nTransactions:\t\t%12u hits\n",        data_get_count(D));
  fprintf(stderr, "Availability:\t\t%12.2f %%\n",          data_get_count(D)==0 ? 0 :
                                                           (double)data_get_count(D) /
                                                           (data_get_count(D)+my.failed)
                                                           *100
  );
  fprintf(stderr, "Elapsed time:\t\t%12.2f secs\n",        data_get_elapsed(D));
  fprintf(stderr, "Data transferred:\t%12.2f MB\n",        data_get_megabytes(D)); /*%12llu*/
  fprintf(stderr, "Response time:\t\t%12.2f secs\n",       data_get_response_time(D));
  fprintf(stderr, "Transaction rate:\t%12.2f trans/sec\n", data_get_transaction_rate(D));
  fprintf(stderr, "Throughput:\t\t%12.2f MB/sec\n",        data_get_throughput(D));
  fprintf(stderr, "Concurrency:\t\t%12.2f\n",              data_get_concurrency(D));
  fprintf(stderr, "Successful transactions:%12u\n",        data_get_code(D)); 
  if (my.debug) {
    fprintf(stderr, "HTTP OK received:\t%12u\n",             data_get_ok200(D));
  }
  fprintf(stderr, "Failed transactions:\t%12u\n",          my.failed);
  fprintf(stderr, "Longest transaction:\t%12.2f\n",        data_get_highest(D));
  fprintf(stderr, "Shortest transaction:\t%12.2f\n",       data_get_lowest(D));
  fprintf(stderr, " \n");
  if(my.mark)    mark_log_file(my.markstr);
  if(my.logging) log_transaction(D);

  my.cookies = cookies_destroy(my.cookies);
  data_destroy(D);
  if (my.url == NULL) {
    for (x = 0; x < my.length; x++)
      xfree(lines->line[x]);
    xfree(lines->line);
    xfree(lines);
  } else {
    xfree(lines->line);
    xfree(lines);
  }

  /** 
   * I should probably take a deeper look 
   * at cookie content to free it but at 
   * this point we're two lines from exit
   */
  xfree (my.url);
  
  exit(EXIT_SUCCESS);  
} /* end of int main **/
