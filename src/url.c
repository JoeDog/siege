/**
 * URL Support
 *
 * Copyright (C) 2013-2015 
 * Jeffrey Fulmer - <jeff@joedog.org>, et al.
 * Copyright (C) 1999 by
 * Jeffrey Fulmer - <jeff@joedog.org>.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *--
 */
#include <stdio.h>
#include <stdlib.h>
#include <setup.h>
#include <url.h>
#include <load.h>
#include <date.h>
#include <util.h>
#include <joedog/joedog.h>
#include <joedog/boolean.h>
#include <joedog/defs.h>

struct URL_T
{
  int       ID;
  char *    url;
  SCHEME    scheme;
  METHOD    method;
  char *    username;
  char *    password;
  char *    hostname;
  int       port;
  char *    path;
  char *    file;
  char *    params;
  BOOLEAN   hasparams;
  char *    query;
  char *    frag;
  char *    request;
  size_t    postlen;
  char *    postdata;
  char *    posttemp;
  char *    conttype;
  time_t    expires;
  time_t    modified;
  BOOLEAN   cached;
  char *    etag;
  char *    realm;
};

size_t URLSIZE = sizeof(struct URL_T);

private void    __url_parse(URL this, char *url);
private void    __parse_post_data(URL this, char *datap);
private char *  __url_set_absolute(URL this, char *url);
private BOOLEAN __url_has_scheme (char *url);
private BOOLEAN __url_has_credentials(char *url);
private int     __url_default_port(URL this);
private char *  __url_set_scheme(URL this, char *url);
private char *  __url_set_password(URL this, char *str);
private char *  __url_set_username(URL this, char *str);
private char *  __url_set_hostname(URL this, char *str);
private char *  __url_set_port(URL this, char *str);
private char *  __url_set_path(URL this, char *str);
private char *  __url_set_file(URL this, char *str);
private char *  __url_set_parameters(URL this, char *str);
private char *  __url_set_query(URL this, char *str);
private char *  __url_set_fragment(URL this, char *str);
private char *  __url_escape(const char *s);
private METHOD  __url_has_method(const char *url);


URL
new_url(char *str)
{
  URL this;

  this = xcalloc(sizeof(struct URL_T), 1);
  this->ID = 0;
  this->hasparams = FALSE;
  __url_parse(this, str); 
  return this;
}

URL
url_destroy(URL this)
{
  if (this!=NULL) {
    xfree(this->username);
    xfree(this->password);
    xfree(this->hostname);
    xfree(this->path);
    xfree(this->file);
    xfree(this->query);
    xfree(this->frag);
    xfree(this->request);
    xfree(this->conttype);
    xfree(this->postdata);
    xfree(this->etag);
    xfree(this);
  }
  return NULL;  
}

/**
 * URL setters 
 */

void 
url_set_ID(URL this, int ID)
{
  this->ID = ID;
  return;
}

/**
 * if we don't have a hostname at 
 * construction, we can use this 
 * method to add one...
 */
void 
url_set_hostname(URL this, char *hostname) 
{
  size_t len;

  if (empty(hostname)) return;

  xfree(this->hostname);
  len = strlen(hostname)+1;
  this->hostname = xmalloc(len);
  memset(this->hostname, '\0', len);
  strncpy(this->hostname, hostname, len);
  return;
}

void
url_set_last_modified(URL this, char *date) 
{
  this->modified = strtotime(date);
  return;
}

void
url_set_etag(URL this, char *etag)
{
  size_t len;

  if (empty(etag)) return;

  len = strlen(etag)+1;
  this->etag = xmalloc(len);
  memset(this->etag, '\0', len);
  strncpy(this->etag, etag, len);
  return;
}

void 
url_set_conttype(URL this, char *type) {
  this->conttype = xstrdup(type);
  return;
}

void
url_set_method(URL this, METHOD method) {
  this->method = method;
}

void
url_set_postdata(URL this, char *postdata, size_t postlen)
{
  this->postlen = postlen;
  this->postdata = malloc(this->postlen);
  memcpy(this->postdata, postdata, this->postlen);
  return;
}


/**
 * URL getters
 */

public int
url_get_ID(URL this) 
{
  return this->ID;
}

public char *
url_get_absolute(URL this)
{
  return this->url;
}

public SCHEME
url_get_scheme(URL this)
{
  return this->scheme;
}

public char *
url_get_display(URL this)
{
  if (my.fullurl)
    return url_get_absolute(this);

  if (this->method == GET)
    return url_get_request(this);

  return url_get_absolute(this);
}

public char *
url_get_scheme_name(URL this)
{
  switch (this->scheme) {
    case HTTP:
    return "http";
    case HTTPS:
    return "https";
    case FTP:
    return "ftp";
    case PROXY:
    return "proxy";
    case UNSUPPORTED:
    default:
    return "unsupported";
  }
  return "unsupported";
}

public char *
url_get_username(URL this) 
{
  return this->username;
}

public char *
url_get_password(URL this) 
{
  return this->password;
}

public char *
url_get_hostname(URL this)
{
  return this->hostname;
}

public int
url_get_port(URL this)
{
  return this->port;
}

public char *
url_get_path(URL this)
{
  return this->path;
}

public char *
url_get_file(URL this) 
{
  return this->file;
}

public char *
url_get_request(URL this)
{
  return this->request;
}

public char *
url_get_parameters(URL this)
{
  return this->params;
}

public char *
url_get_query(URL this) 
{
  return this->query;
}

public char *
url_get_fragment(URL this)
{
  return this->frag;
}

public size_t
url_get_postlen(URL this) {
  return this->postlen;
}

public char *
url_get_postdata(URL this) {
  return this->postdata;
}

public char *
url_get_posttemp(URL this) {
  return this->posttemp;
}

public char *
url_get_conttype(URL this) {

  if (this->conttype == NULL) {
    if (! empty(my.conttype)) {
      this->conttype = xstrdup(my.conttype);
    } else {
      this->conttype = xstrdup("application/x-www-form-urlencoded");
    }
  }
  return this->conttype;
}

public METHOD 
url_get_method(URL this) {
  return this->method;
}

public char *
url_get_method_name(URL this) {
  switch (this->method){
    case POST:
      return "POST";
    case PUT:
      return "PUT";
    case DELETE:
      return "DELETE";
    case HEAD:
     return "HEAD";
    case GET:
    default:
      return "GET";
  }
  return "GET";
}

char *
url_get_if_modified_since(URL this)
{
  if (this->cached == FALSE){
    return NULL;
  }

  return timetostr(&this->modified);
}

char *
url_get_etag(URL this)
{
  char   *tag;
  size_t len;

  if (empty(this->etag)) return NULL;

  len = strlen(this->etag) + 18;
  tag = xmalloc(len);
  memset(tag, '\0', len);

  snprintf(tag, len, "If-None-Match: %s\015\012", this->etag);
  return tag;
}

public char *
url_get_realm(URL this)
{
  return (this->realm!=NULL)?this->realm:"";
}

public void
url_set_realm(URL this, char *realm)
{
  this->realm = xstrdup(realm);
}

void
url_set_username(URL this, char *username) 
{
  size_t len = strlen(username);

  this->username = malloc(len+1);
  memset(this->username, '\0', len+1);
  memcpy(this->username, username, len);
  return;
}

void
url_set_password(URL this, char *password) 
{
  size_t len = strlen(password);

  this->password = malloc(len+1);
  memset(this->password, '\0', len+1);
  memcpy(this->password, password, len);
  return;
}

void
url_dump(URL this) 
{
  printf("URL ID:    %d\n", this->ID);
  printf("Abolute:   %s\n", this->url);
  printf("Scheme:    %s\n", url_get_scheme_name(this));
  printf("Method:    %s\n", url_get_method_name(this));
  printf("Username:  %s\n", url_get_username(this));
  printf("Password:  %s\n", url_get_password(this));
  printf("Hostname:  %s\n", url_get_hostname(this));
  printf("Port:      %d\n", url_get_port(this));
  printf("Path:      %s\n", url_get_path(this));
  printf("File:      %s\n", url_get_file(this));
  printf("Request:   %s\n", url_get_request(this));
  if (this->hasparams==TRUE)
    printf("Params:   %s\n", url_get_parameters(this));
  printf("Query:     %s\n", url_get_query(this));
  printf("Fragment:  %s\n", url_get_fragment(this));
  printf("Post Len:  %d\n", (int)url_get_postlen(this));
  printf("Post Data: %s\n", url_get_postdata(this));
  printf("Cont Type: %s\n", url_get_conttype(this));
  //time_t    expires;
  //time_t    modified;
  //BOOLEAN   cached;
  //char *    etag;
  //char *    realm;
  return;
}

URL
url_normalize(URL req, char *location)
{
  URL    ret;
  char * url;
  size_t len = strlen(url_get_absolute(req)) + strlen(location) + 32;

  if (strchr(location, ':') != NULL) {
    // it's very likely normalized
    ret = new_url(location);
    // but we better test it...
    if (strlen(url_get_hostname(ret)) > 1) {
      return ret;
    }
  }

  if (strchr(location, '.') != NULL) {
    // it's *maybe* host/path
    ret = new_url(location);
    // so we better test it...
    if (strchr(url_get_hostname(ret), '.') != NULL) {
      return ret;
    }
  }

  // XXX: 8/20/2014 - YES. Yes, I do.
  if (strstr(location, "localhost") != NULL) {
    ret = new_url(location);
    if (strlen(url_get_hostname(ret)) == 9) {
      // we found and correctly parsed localhost
      return ret;
    }
  }

  /**
   * If we got this far we better construct it...
   */
  url = (char*)malloc(len);
  memset(url, '\0', len);

  if (location[0] == '/') {
    if (strlen(location) > 1 && location[1] == '/') {
      /* starts with // so we should use base protocol */
      snprintf(url, len, "%s:%s", url_get_scheme_name(req), location);
    } else {
      snprintf(url, len, "%s://%s:%d%s", url_get_scheme_name(req), url_get_hostname(req), url_get_port(req), location);
    }
  } else {
    snprintf(url, len, "%s://%s:%d/%s", url_get_scheme_name(req), url_get_hostname(req), url_get_port(req), location);
  }
  ret = new_url(url);
  free(url);
  return ret;
}

private void
__url_parse(URL this, char *url)
{
  char   *ptr = NULL;
  char   *esc = NULL;
  char   *post;  

  /**
   * URL escaping is in its infancy so we're
   * going to make it a configurable option. 
   * see: url-escaping in siegerc.
   */
  esc = __url_escape(url);
  if (my.escape) {
    ptr = __url_set_absolute(this, esc);
  } else {
    ptr = __url_set_absolute(this, url);
  }
  ptr = __url_set_scheme(this, ptr);
  
  post = strstr(this->url, " POST");
  if (! post) {
    post = strstr(this->url, " PUT");
  }

  if (post != NULL){
    if (!strncasecmp(post," PUT", 4)) {
      this->method = PUT;
      post += 4;
    } else {
      this->method = POST;
      post += 5;
    }
    __parse_post_data(this, post);
  } else {
    this->method = GET;
    this->postdata   = NULL;
    this->posttemp   = NULL;
    this->postlen    = 0;
  }

  if (__url_has_credentials(ptr)) {
    ptr = __url_set_username(this, ptr);
    ptr = __url_set_password(this, ptr);
  }

  ptr = __url_set_hostname(this, ptr);
  ptr = __url_set_port(this, ptr);
  ptr = __url_set_path(this, ptr);
  ptr = __url_set_file(this, ptr); 
  ptr = __url_set_parameters(this, ptr);
  ptr = __url_set_query(this, ptr);
  ptr = __url_set_fragment(this, ptr);
  return;
}

private void
__parse_post_data(URL this, char *datap)
{
  for (; isspace((unsigned int)*datap); datap++) {
    /* Advance past white space */
  }
  if (*datap == '<') {
    datap++;
    load_file(this, datap);
    this->file = xmalloc(strlen(datap)+1);
    memset(this->file, '\0', strlen(datap)+1);
    memcpy(this->file, datap, strlen(datap));
    return;
  } else {
    this->postdata = xstrdup(datap);
    this->postlen  = strlen(this->postdata);
    if (! empty(my.conttype)) {
      this->conttype = xstrdup(my.conttype);
    } else {
      this->conttype = xstrdup("application/x-www-form-urlencoded");
    }
    return;
  }

  return;
}

/**
 * assign the full url to this->url
 */
private char *
__url_set_absolute(URL this, char *url)
{
  size_t len;
  char   *slash;
  //char *ptr = url;

  if (empty(url)) return NULL;

  len = strlen(url)+5;
  if (!__url_has_scheme(url)) {
    this->url = xmalloc(len+7);
    memset(this->url, '\0', len+7);
    slash = strstr(url, "/");
    if (slash) {
      snprintf(this->url, len+7, "http://%s", url);
    } else {
      snprintf(this->url, len+7, "http://%s/", url);
    }
  } else {
    this->url = xmalloc(len);
    memset(this->url, '\0', len);
    snprintf(this->url, len, "%s", url);
  }
  return this->url;
}

#define SCHEME_CHAR(ch) (isalnum (ch) || (ch) == '-' || (ch) == '+')
/**
 * stolen from wget:url.c
 */
private BOOLEAN
__url_has_scheme (char *url)
{
  const char *p = url;

  /* The first char must be a scheme char. */
  if (!*p || !SCHEME_CHAR (*p))
    return FALSE;
  ++p;
  /* Followed by 0 or more scheme chars. */
  while (*p && SCHEME_CHAR (*p))
    ++p;
  /* Terminated by ':'. */
  return *p == ':';
}

private BOOLEAN
__url_has_credentials(char *url)
{
  /** 
   * if there's an @ before /?#; then we have creds 
   */
  const char *p = (const char *)strpbrk (url, "@/?#;");
  if (!p || *p != '@')
    return FALSE;
  return TRUE;
}

private int
__url_default_port(URL this)
{
  switch(this->scheme){
    case FTP:
     return 21;
    case HTTP:
      return 80;
    case HTTPS:
      return 443;
    case UNSUPPORTED:
    default:
      return 80;
  }
}

/**
 * set the scheme, i.e., http/https
 * <SCHEME>://<username>:<password>@<hostname>:<port>/<path>;<params>?<query>#<frag>
 */
private char *
__url_set_scheme(URL this, char *url)
{
  if(!strncasecmp(this->url, "http:", 5)){
    this->scheme = HTTP;
    return url+7;
  }
  if(!strncasecmp(this->url, "https:", 6)){
    this->scheme = HTTPS;
    return url+8;
  }
  if(!strncasecmp(this->url, "ftp:", 4)){
    this->scheme = FTP;
    return url+6;
  }
  this->scheme = UNSUPPORTED;
  return url;
}

/**
 * set the username
 * <scheme>://<USERNAME>:<password>@<hostname>:<port>/<path>;<params>?<query>#<frag>
 */
private char *
__url_set_username(URL this, char *str)
{
  int i;
  char *a;
  char *s;

  a = strchr(str, '@');
  s = strchr(str, '/');

  if((!a) || (s && (a >= s))){
    return str;
  }

  for(i = 0; str[i] && str[i] != ':' && str[i] != '@' && str[i] != '/'; i++);

  if(str[i] != '@' && str[i] != ':'){
    return str;
  }

  this->username = malloc(i+1);
  memcpy(this->username, str, i + 1);
  this->username[i] = '\0';
  str += i + 1;

  return str;
}

/**
 * set the password
 * <scheme>://<username>:<PASSWORD>@<hostname>:<port>/<path>;<params>?<query>#<frag>
 */
private char *
__url_set_password(URL this, char *str)
{
  int i;
  char *a;
  char *s;
  a = strchr(str, '@');
  s = strchr(str, '/');

  if((!a) || (s && (a >= s)) ){
    return str;
  }
  /**
   * XXX: as the original author (Zachary Beane <xach@xach.com>) notes:
   * this code breaks if user has an '@' or a '/' in their password. 
   */
  for(i = 0 ; str[i] != '@'; i++);
  this->password = xmalloc(i+1);

  memcpy(this->password, str, i);
  this->password[i] = '\0';

  str += i + 1;

  return str;
}

/**
 * set the hostname
 * <scheme>://<username>:<password>@<HOSTNAME>:<port>/<path>;<params>?<query>#<frag>
 */
private char *
__url_set_hostname(URL this, char *str)
{
  int i;

  /* skip to end, slash, or port colon */
  for(i = 0; str[i] && str[i] != '/' && str[i] != ':'; i++);

  this->hostname = xmalloc(i + 1);
  memcpy(this->hostname, str, i);

  this->hostname[i] = '\0';

  /* if there's a port */
  if(str[i] == ':'){
    str += i + 1;
  } else {
    str += i;
  }
  return str;
}

/**
 * set the port
 * <scheme>://<username>:<password>@<hostname>:<PORT>/<path>;<params>?<query>#<frag>
 */
private char *
__url_set_port(URL this, char *str)
{
  char *portstr;
  int i;

  this->port = __url_default_port(this);

   for(i = 0; isdigit(str[i]); i++);

   if(i == 0) return str;


   portstr = malloc(i + 1);
   memcpy(portstr, str, i + 1);
   portstr[i] = '\0';

   this->port = atoi(portstr);
   xfree(portstr);

   str += i;
   return str;
}

/**
 * set the path
 * <scheme>://<username>:<password>@<hostname>:<port>/<PATH>;<params>?<query>#<frag>
 */
private char *
__url_set_path(URL this, char *str)
{
  int  i;    // capture the lenght of the path
  int  j;    // capture the length of the request (sans frag)

  for(i = strlen(str); i > 0 && str[i] != '/'; i--);
  for(j = 0; str[j] && (str[j] != '#' && !isspace(str[j])); j++);


  if(str[i] != '/'){
    this->path    = xmalloc(2);
    this->request = xmalloc(2);
    strncpy(this->path,    "/", 2);
    strncpy(this->request, "/", 2);
    this->path[1]    = '\0';
    this->request[1] = '\0';
  } else {
    this->path    = xmalloc(i+2);
    this->request = xmalloc(j+2);
    memcpy(this->path, str, i+1);
    memcpy(this->request, str, j+1);
    this->path[i] = '/';
    this->path[i + 1]    = '\0';
    if (this->request[j]=='#') {
      this->request[j]   = '\0'; // lop the #
    } else {
      this->request[j+1] = '\0'; // no frag
    }
  }
  trim(this->request);
  str += i + 1;
  return str;
}

/**
 * set the file
 * <scheme>://<username>:<password>@<hostname>:<port>/<FILE>;<params>?<query>#<frag>
 */
private char *
__url_set_file(URL this, char *str)
{
  int   i;

  if (str==NULL) return NULL;
  if (this->file != NULL && strlen(this->file) > 1) return str;

  for(i = 0; str[i] && (str[i] != ';' && str[i] != '?' && !isspace(str[i])); i++);
  this->file = xmalloc(i+1);
  memset(this->file, '\0', i+1);
  memcpy(this->file, str, i);
  trim(this->file);

  /* if there are params or a query string */
  if (str[i] == ';') { 
    this->hasparams = TRUE;
    str += i + 1;
  } else if(str[i] == '?') {
    str += i + 1;
  } else {
    str += i;
  }
  return str;
}

/**
 * set the parameters
 * <scheme>://<username>:<password>@<hostname>:<port>/<path>;<PARAMS>?<query>#<frag>
 */
private char *
__url_set_parameters(URL this, char *str)
{
  int i;

  if (str==NULL) return NULL;
  if (this->params != NULL && strlen(this->params) > 1) return str;

  if (this->hasparams == FALSE) {
    this->params = "";
    return str;
  }
  
  for (i = 0; str[i] && (str[i] != '?' && !isspace(str[i])); i++);

  this->params = xmalloc(i+1);
  memset(this->params, '\0', i+1);
  memcpy(this->params, str, i);

  /* if there is a query string */
  if(str[i] == '?'){
    str += i + 1;
  } else {
    str += i;
  }
  return str;
}

/**
 * set the query
 * <scheme>://<username>:<password>@<hostname>:<port>/<path>;<params>?<QUERY>#<frag>
 */
private char *
__url_set_query(URL this, char *str)
{
  int   i;

  if (str==NULL) {
    this->query = xstrcat("");
    return NULL;
  }

  if (this->query != NULL && strlen(this->query) > 1) return str;
  
  for(i = 0; str[i] && (str[i] != '#' && !isspace(str[i])); i++);

  this->query = xmalloc(i+1);
  memset(this->query, '\0', i+1);
  memcpy(this->query, str, i);

  /* if there are params or a query string */
  if(str[i] == '#'){
    str += i + 1;
  } else {
    str += i;
  }
  return str;
}

/**
 * set the fragment (not used by siege)
 * <scheme>://<username>:<password>@<hostname>:<port>/<path>;<params>?<query>#<FRAG>
 */
private char *
__url_set_fragment(URL this, char *str)
{
  int   i;

  if (str==NULL) return NULL;
  if (this->frag != NULL && strlen(this->frag) > 1) return str;

  for(i = 0; str[i] && !isspace(str[i]); i++);

  this->frag = xmalloc(i+1);
  memcpy(this->frag, str, i);

  str += i + 1;
  return str;
}

/**
 * The following functions provide url encoding. They
 * were lifted from wget:
 * Copyright (C) 1995, 1996, 1997, 1998, 2000, 2001
 * Free Software Foundation, Inc.
 */
enum {
  /* rfc1738 reserved chars, preserved from encoding.  */
  urlchr_reserved = 1,

  /* rfc1738 unsafe chars, plus some more.  */
  urlchr_unsafe   = 2
};

#define urlchr_test(c, mask) (urlchr_table[(unsigned char)(c)] & (mask))
#define URL_RESERVED_CHAR(c) urlchr_test(c, urlchr_reserved)
#define URL_UNSAFE_CHAR(c) urlchr_test(c, urlchr_unsafe)

/* Shorthands for the table: */
#define R  urlchr_reserved
#define U  urlchr_unsafe
#define RU R|U

static const unsigned char urlchr_table[256] =
{
  U,  U,  U,  U,   U,  U,  U,  U,   /* NUL SOH STX ETX  EOT ENQ ACK BEL */
  U,  U,  U,  U,   U,  U,  U,  U,   /* BS  HT  LF  VT   FF  CR  SO  SI  */
  U,  U,  U,  U,   U,  U,  U,  U,   /* DLE DC1 DC2 DC3  DC4 NAK SYN ETB */
  U,  U,  U,  U,   U,  U,  U,  U,   /* CAN EM  SUB ESC  FS  GS  RS  US  */
  U,  0,  U, RU,   0,  U,  R,  0,   /* SP  !   "   #    $   %   &   '   */
  0,  0,  0,  R,   0,  0,  0,  R,   /* (   )   *   +    ,   -   .   /   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* 0   1   2   3    4   5   6   7   */
  0,  0, RU,  R,   U,  R,  U,  R,   /* 8   9   :   ;    <   =   >   ?   */
 RU,  0,  0,  0,   0,  0,  0,  0,   /* @   A   B   C    D   E   F   G   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* H   I   J   K    L   M   N   O   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* P   Q   R   S    T   U   V   W   */
  0,  0,  0, RU,   U, RU,  U,  0,   /* X   Y   Z   [    \   ]   ^   _   */
  U,  0,  0,  0,   0,  0,  0,  0,   /* `   a   b   c    d   e   f   g   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* h   i   j   k    l   m   n   o   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* p   q   r   s    t   u   v   w   */
  0,  0,  0,  U,   U,  U,  U,  U,   /* x   y   z   {    |   }   ~   DEL */

  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,

  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
};
#undef R
#undef U
#undef RU

enum copy_method { CM_DECODE, CM_ENCODE, CM_PASSTHROUGH };

/** 
 * Decide whether to encode, decode, or pass through the char at P.
 *  This used to be a macro, but it got a little too convoluted.  
 */
static inline enum copy_method
decide_copy_method (const char *p)
{
  if (*p == '%') {
    if (ISXDIGIT (*(p + 1)) && ISXDIGIT (*(p + 2))) {
      /**
       * %xx sequence: decode it, unless it would decode to an
       * unsafe or a reserved char; in that case, leave it as is.
       */ 
      char preempt = X2DIGITS_TO_NUM (*(p + 1), *(p + 2));
      if (URL_UNSAFE_CHAR (preempt) || URL_RESERVED_CHAR (preempt))
        return CM_PASSTHROUGH;
      else
        return CM_DECODE;
    } else {
      return CM_ENCODE;
    }
  }
  else if (URL_UNSAFE_CHAR (*p) && !URL_RESERVED_CHAR (*p))
    return CM_ENCODE;
  else
    return CM_PASSTHROUGH;
}

static METHOD
__url_has_method(const char *url)
{
   unsigned int i = 0;
   const char * r = NULL;
   static const char* const methods[] = {
     " GET", " HEAD", " POST", " PUT", " TRACE", " DELETE", " OPTIONS", " CONNECT"
   };

   for (i = 0; i < sizeof(methods) / sizeof(methods[0]); i++) {
     r = strstr(url, methods[i]);
     if (r != NULL) return i;
   }

   return NOMETHOD;
}

private char *
__url_escape(const char *s)
{
  const char *p1;
  char *newstr, *p2;
  int oldlen, newlen;

  int encode_count = 0;
  int decode_count = 0;

  /** 
   * FIXME: we're not going to escape siege method
   * URLS, i.e., things with PUT or POST but if the
   * path contains spaces they won't be escaped.
   */
  if (__url_has_method(s)!=NOMETHOD) {
    return (char *)s;
  }  

  /* First, pass through the string to see if there's anything to do,
     and to calculate the new length.  */
  for (p1 = s; *p1; p1++) {
    switch (decide_copy_method (p1)) {
      case CM_ENCODE:
        ++encode_count;
        break;
      case CM_DECODE:
        ++decode_count;
        break;
      case CM_PASSTHROUGH:
        break;
    }
  }

  if (!encode_count && !decode_count)
    return (char *)s; /* C const model sucks. */

  oldlen = p1 - s;
  /* Each encoding adds two characters (hex digits), while each
     decoding removes two characters.  */
  newlen = oldlen + 2 * (encode_count - decode_count);
  newstr = xmalloc (newlen + 1);

  p1 = s;
  p2 = newstr;

  while (*p1) {
    switch (decide_copy_method (p1)) {
      case CM_ENCODE: {
        unsigned char c = *p1++;
        *p2++ = '%';
        *p2++ = XNUM_TO_DIGIT (c >> 4);
        *p2++ = XNUM_TO_DIGIT (c & 0xf);
      }
      break;
    case CM_DECODE:
      *p2++ = X2DIGITS_TO_NUM (p1[1], p1[2]);
      p1 += 3;              /* skip %xx */
      break;
    case CM_PASSTHROUGH:
      *p2++ = *p1++;
    }
  }
  *p2 = '\0';
  return newstr;
}

