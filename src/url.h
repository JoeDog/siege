/**
 * URL Support
 *
 * Copyright (C) 2013-2016 by
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
#ifndef __URL_H
#define __URL_H
#include <stdlib.h>
#include <joedog/defs.h>
#include <joedog/boolean.h>

/** 
 * a URL object 
 */
typedef struct URL_T *URL;

/**
 * For memory allocation; URLSIZE 
 * provides the object size 
 */
extern size_t  URLSIZE;   

/**
 * HTTP method 
 */
typedef enum {
  NOMETHOD =  0,
  HEAD     =  1,
  GET      =  2,
  POST     =  3,
  PUT      =  4,
  DELETE   =  5,
  TRACE    =  6,
  OPTIONS  =  7,
  CONNECT  =  8,
  PATCH    =  9,
} METHOD;
  
/**
 * enum SCHEME
 */
typedef enum {
  UNSUPPORTED = 0,
  HTTP        = 1,
  HTTPS       = 2,
  FTP         = 3,
  PROXY       = 4
} SCHEME;


/* Constructor / destructor */
URL      new_url(char *str);
URL      url_destroy(URL this);
void     url_dump(URL this);

void     url_set_ID(URL this, int id);
void     url_set_scheme(URL this, SCHEME scheme);
void     url_set_hostname(URL this, char *hostname);
void     url_set_redirect(URL this, BOOLEAN redir);
void     url_set_conttype(URL this, char *type);
void     url_set_postdata(URL this, char *postdata, size_t postlen);
void     url_set_method(URL this, METHOD method);

int      url_get_ID(URL this);
METHOD   url_get_method(URL this);
char *   url_get_method_name(URL this) ;
BOOLEAN  url_is_redirect(URL this);

/* <scheme>://<username>:<password>@<hostname>:<port>/<path>;<params>?<query>#<frag> */
char *   url_get_absolute(URL this);

/* <SCHEME>://<username>:<password>@<hostname>:<port>/<path>;<params>?<query>#<frag> */
SCHEME   url_get_scheme(URL this);
char *   url_get_scheme_name(URL this);

/* <scheme>://<USERNAME>:<password>@<hostname>:<port>/<path>;<params>?<query>#<frag> */
char *   url_get_username(URL this);

/* <scheme>://<username>:<PASSWORD>@<hostname>:<port>/<path>;<params>?<query>#<frag> */
char *   url_get_password(URL this);

/* <scheme>://<username>:<password>@<HOSTNAME>:<port>/<path>;<params>?<query>#<frag> */
char *   url_get_hostname(URL this);

/* <scheme>://<username>:<password>@<hostname>:<PORT>/<path>;<params>?<query>#<frag> */
int      url_get_port(URL this);

/* <scheme>://<username>:<password>@<hostname>:<port>/<PATH>;<params>?<query>#<frag> */
char *   url_get_path(URL this);

/* <scheme>://<username>:<password>@<hostname>:<port>/<FILE>;<params>?<query>#<frag> */
char *   url_get_file(URL this);
char *   url_get_request(URL this); // "<PATH><FILE>"

/* <scheme>://<username>:<password>@<hostname>:<port>/<file>;<PARAMS>?<query>#<frag> */
char *   url_get_parameters(URL this);

/* <scheme>://<username>:<password>@<hostname>:<port>/<path>;<params>?<QUERY>#<frag> */
char *   url_get_query(URL this);

/* <scheme>://<username>:<password>@<hostname>:<port>/<path>;<params>?<query>#<FRAG> */
char *   url_get_fragment(URL this);


/* 
 *  Make a decision about what to display.  Will show absolute url when fullurl
 *  is set to ture.  Otherwise we check the HTTP method to display the submitted
 *  URI or the respective line provided from the urls file.
 */
char *   url_get_display(URL this);

/**
 * POST method getters
 * <scheme>://<username>:<password>@<hostname>:<port>/<path> POST <params>?<query>#<frag> 
 */
size_t   url_get_postlen(URL this);   
char *   url_get_postdata(URL this);  
char *   url_get_posttemp(URL this); 
char *   url_get_conttype(URL this);  
char *   url_get_if_modified_since(URL this);
char *   url_get_etag(URL this);
char *   url_get_realm(URL this);
void     url_set_realm(URL this, char *realm);
void     url_set_username(URL this, char *username);
void     url_set_password(URL this, char *password);
URL      url_normalize(URL req, char *location);
char *   url_normalize_string(URL req, char *location);


#endif/*__URL_H*/
