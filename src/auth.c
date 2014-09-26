/**
 * HTTP Authentication
 *
 * Copyright (C) 2002-2014 by
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <base64.h>
#include <md5.h>
#include <auth.h>
#include <array.h>
#include <util.h>
#include <pthread.h>
#include <setup.h>
#include <joedog/joedog.h>

struct AUTH_T {
  ARRAY   creds;
  BOOLEAN okay;
  struct {
    char *encode;
  } basic;
  struct {
    char *encode;
  } digest;
  struct {
    BOOLEAN required;   /* boolean, TRUE == use a proxy server.    */
    char *hostname;     /* hostname for the proxy server.          */
    int  port;          /* port number for proxysrv                */
    char *encode;       /* base64 encoded username and password    */
  } proxy;
  pthread_mutex_t lock;
}; 

struct DIGEST_CRED {
  char *username;
  char *password;
  char *cnonce_value;
  char *h_a1;
  char nc[9];
  unsigned int nc_value;
};

struct DIGEST_CHLG {
 char *realm;
 char *domain;
 char *nonce;
 char *opaque;
 char *stale;
 char *algorithm;
 char *qop;
};

typedef enum {
  REALM,
  DOMAIN,
  NONCE,
  OPAQUE,
  STALE,
  ALGORITHM,
  QOP,
  UNKNOWN
} KEY_HEADER_E;

size_t AUTHSIZE = sizeof(struct AUTH_T);

private BOOLEAN       __basic_header(AUTH this, SCHEME scheme, CREDS creds);
private DCHLG *       __digest_challenge(const char *challenge);
private DCRED *       __digest_credentials(CREDS creds, size_t *randseed);
private KEY_HEADER_E  __get_keyval(const char *key);
private char *        __get_random_string(size_t length, unsigned int *randseed);
private char *        __get_h_a1(const DCHLG *chlg, DCRED *cred, const char *nonce_value);
private char *        __get_md5_str(const char *buf);
private BOOLEAN       __str_list_contains(const char *str, const char *pattern, size_t pattern_len);

AUTH
new_auth()
{
  AUTH this;

  this = calloc(AUTHSIZE, 1);
  this->creds  = new_array();
  this->basic.encode  = xstrdup("");
  this->digest.encode = xstrdup("");
  this->proxy.encode  = xstrdup("");
  return this;
}

AUTH
auth_destroy(AUTH this)
{
  this->creds = array_destroy(this->creds);
  xfree(this);
  return NULL;
}

void
auth_add(AUTH this, CREDS creds) 
{
  array_npush(this->creds, creds, CREDSIZE);
  return;
}

void
auth_display(AUTH this, SCHEME scheme)
{
  size_t i;
  //XXX: Needs to be reformatted for siege -C
  for (i = 0; i < array_length(this->creds); i++) {
    CREDS tmp = array_get(this->creds, i);
    if (creds_get_scheme(tmp) == scheme) {
      printf("credentials:  %s:%s:%s\n", creds_get_username(tmp), creds_get_password(tmp), creds_get_realm(tmp));
    }
  } 
}

char *
auth_get_basic_header(AUTH this, SCHEME scheme)
{
  if (scheme == PROXY) {
    return this->proxy.encode;
  } else {
    return this->basic.encode;
  }  
}

BOOLEAN
auth_set_basic_header(AUTH this, SCHEME scheme, char *realm) 
{
  size_t i;
  for (i = 0; i < array_length(this->creds); i++) {
    CREDS tmp = array_get(this->creds, i);
    if (strmatch(creds_get_realm(tmp), realm)) {
      if (creds_get_scheme(tmp) == HTTP || creds_get_scheme(tmp) == HTTPS) {
        return __basic_header(this, scheme, tmp); 
      }
    }
  }
  /**
   * didn't match a realm, trying 'any'
   */ 
  for (i = 0; i < array_length(this->creds); i++) {
    CREDS tmp = array_get(this->creds, i);
    if (strmatch(creds_get_realm(tmp), "any")) {
      if (creds_get_scheme(tmp) == HTTP || creds_get_scheme(tmp) == HTTPS) {
        return __basic_header(this, scheme, tmp); 
      }
    }
  }
  return FALSE; 
}

//digest_generate_authorization(C->auth.wwwchlg, C->auth.wwwcred, "GET", fullpath);
char *
auth_get_digest_header(AUTH this, SCHEME scheme, DCHLG *chlg, DCRED *cred, const char *method, const char *uri)
{
  size_t len;
  char  *cnonce      = NULL;
  char  *nonce_count = NULL;
  char  *qop         = NULL;
  char  *response    = NULL;
  char  *request_digest = NULL;
  char  *h_a1 = NULL;
  char  *h_a2 = NULL;
  char  *opaque = NULL;
  char  *result, *tmp;

  if (NULL != chlg->qop) {
    nonce_count = xstrcat(", nc=", cred->nc, NULL);
    cnonce = xstrcat(", cnonce=\"", cred->cnonce_value, "\"", NULL);

    if (NULL == (h_a1 = __get_h_a1(chlg, cred, chlg->nonce))) {
      fprintf(stderr, "error calling __get_h_a1\n");
      return NULL;
    }

    if (__str_list_contains(chlg->qop, "auth", 4)) {
      qop = xstrdup(", qop=auth");
      tmp = xstrcat(method, ":", uri, NULL);
      h_a2 = __get_md5_str(tmp);
      xfree(tmp);

      tmp = xstrcat(h_a1,":",chlg->nonce,":",cred->nc,":",cred->cnonce_value,":auth:",h_a2,NULL);
      request_digest = __get_md5_str(tmp);
      xfree(tmp);
      response = xstrcat(", response=\"", request_digest, "\"", NULL);
    } else {
      fprintf(stderr, "error quality of protection not supported: %s\n", chlg->qop);
      return NULL;
    }
  } else {
    if (NULL == (h_a1 = __get_h_a1(chlg, cred, ""))) {
      NOTIFY(ERROR, "__get_h_a1\n");
      return NULL;
    }
    tmp = xstrcat(method, ":", uri, NULL);
    h_a2 = __get_md5_str(tmp);
    xfree(tmp);
    tmp = xstrcat(h_a1, ":", chlg->nonce, ":", h_a2, NULL);
    request_digest = __get_md5_str(tmp);
    xfree(tmp);
    response = xstrcat(" response=\"", request_digest, "\"", NULL);
  }
  if (NULL != chlg->opaque)
    opaque = xstrcat(", opaque=\"", chlg->opaque, "\"", NULL);

  result = xstrcat (
    "Digest username=\"", cred->username, "\", realm=\"", chlg->realm, "\", nonce=\"", chlg->nonce, 
    "\", uri=\"", uri, "\", algorithm=", chlg->algorithm, response, opaque ? opaque : "", qop ? qop : "", 
    nonce_count ? nonce_count : "", cnonce ? cnonce : "", NULL
  );

  (cred->nc_value)++;
  snprintf(cred->nc, sizeof(cred->nc), "%.8x", cred->nc_value);

  if (0 == strcasecmp("MD5", chlg->algorithm))
    xfree(h_a1);

  xfree(nonce_count);
  xfree(cnonce);
  xfree(qop);
  xfree(response);
  xfree(request_digest);
  xfree(h_a2);
  xfree(opaque);

  len = strlen(result)+32;

  if (scheme == PROXY) {
    this->proxy.encode = xmalloc(len);
    memset(this->proxy.encode, '\0', len);
    snprintf(this->proxy.encode, len, "Proxy-Authorization: %s\015\012", result);
    xfree(result);
    return this->proxy.encode;
  } else {
    this->digest.encode = xmalloc(len);
    memset(this->digest.encode, '\0', len);
    snprintf(this->digest.encode, len, "Authorization: %s\015\012", result);
    xfree(result);
    return this->digest.encode;
  }  
}

BOOLEAN 
auth_set_digest_header(AUTH this, DCHLG **chlg, DCRED **cred, size_t *rand, char *realm, char *str) {
  size_t  i;
  for (i = 0; i < array_length(this->creds); i++) {
    CREDS tmp = array_get(this->creds, i);
    if (strmatch(creds_get_realm(tmp), realm)) {
      *chlg = __digest_challenge(str);
      *cred = __digest_credentials(tmp, rand);
      if (*cred == NULL || *chlg == NULL) return FALSE;
      return TRUE;
    }
  }
  /**
   * didn't match a realm, trying 'any'
   */ 
  for (i = 0; i < array_length(this->creds); i++) {
    CREDS tmp = array_get(this->creds, i);
    if (strmatch(creds_get_realm(tmp), "any")) {
      *chlg = __digest_challenge(str);
      *cred = __digest_credentials(tmp, rand);
      if (*cred == NULL || *chlg == NULL) return FALSE;
      return TRUE;
    }
  }
  return FALSE; 
}

BOOLEAN
auth_get_proxy_required(AUTH this)
{
  return this->proxy.required;
}

char *
auth_get_proxy_host(AUTH this) 
{
  return this->proxy.hostname;
}

int
auth_get_proxy_port(AUTH this)
{
  return this->proxy.port;
}

void
auth_set_proxy_required(AUTH this, BOOLEAN required)
{
  this->proxy.required = required;
}

void
auth_set_proxy_host(AUTH this, char *host)
{
  this->proxy.hostname = xstrdup(host);
  this->proxy.required = TRUE;
}

void
auth_set_proxy_port(AUTH this, int port)
{
  this->proxy.port = port;
}

char *
auth_get_ftp_username(AUTH this, char *realm) 
{
  size_t  i;
  for (i = 0; i < array_length(this->creds); i++) {
    CREDS tmp = array_get(this->creds, i);
    if (strmatch(creds_get_realm(tmp), realm)) {
      if (creds_get_scheme(tmp) == FTP) {
        return creds_get_username(tmp); 
      }
    }
  }
  /**
   * didn't match a realm, trying 'any'
   */ 
  for (i = 0; i < array_length(this->creds); i++) {
    CREDS tmp = array_get(this->creds, i);
    if (strmatch(creds_get_realm(tmp), "any")) {
      if (creds_get_scheme(tmp) == FTP) {
        return creds_get_username(tmp); 
      }
    }
  }
  return ""; 
}

char *
auth_get_ftp_password(AUTH this, char *realm)
{
  size_t i; 
  for (i = 0; i < array_length(this->creds); i++) {
    CREDS tmp = array_get(this->creds, i);
    if (strmatch(creds_get_realm(tmp), realm)) {
      if (creds_get_scheme(tmp) == FTP) {
        return creds_get_password(tmp);          
      }
    }
  }
  /**
   * didn't match a realm, trying 'any'
   */
  for (i = 0; i < array_length(this->creds); i++) {
    CREDS tmp = array_get(this->creds, i);
    if (strmatch(creds_get_realm(tmp), "any")) {
      if (creds_get_scheme(tmp) == FTP) {
        return creds_get_password(tmp);          
      }
    }
  }
  return "";   
}

private BOOLEAN
__basic_header(AUTH this, SCHEME scheme, CREDS creds)
{
  char    buf[256];
  char   *hdr;
  size_t  len;
  BOOLEAN ret = TRUE;

  memset(buf, '\0', sizeof(buf));
  pthread_mutex_lock(&(this->lock));
  snprintf(buf, sizeof buf,"%s:%s",creds_get_username(creds), creds_get_password(creds));
  if (scheme==PROXY) {
    xfree(this->proxy.encode);
    if ((base64_encode(buf, strlen(buf), &hdr) < 0)) {
      ret = FALSE;
    } else {
      len = strlen(hdr)+32;
      this->proxy.encode = xmalloc(len);
      memset(this->proxy.encode, '\0', len); 
      snprintf(this->proxy.encode, len, "Proxy-Authorization: Basic %s\015\012", hdr);
    } 
  } else {
    xfree(this->basic.encode);
    if ((base64_encode(buf, strlen(buf), &hdr) < 0 )) {
      ret = FALSE;
    } else {
      len = strlen(hdr)+32;
      this->basic.encode = xmalloc(len);
      memset(this->basic.encode, '\0', len); 
      snprintf(this->basic.encode, len, "Authorization: Basic %s\015\012", hdr);
    }
  }
  pthread_mutex_unlock(&(this->lock));
  return ret; 
}

typedef struct
{
  const char *keyname;
  KEY_HEADER_E keyval;
} KEYPARSER;

static const KEYPARSER keyparser[] = 
{
  { "realm",     REALM     },
  { "domain",    DOMAIN    },
  { "nonce",     NONCE     },
  { "opaque",    OPAQUE    },
  { "stale",     STALE     },
  { "algorithm", ALGORITHM },
  { "qop",       QOP       },
  { NULL,        UNKNOWN   }
};


private KEY_HEADER_E
__get_keyval(const char *key)
{
  int i;

  for (i = 0; keyparser[i].keyname; i++) {
    if (!strcasecmp(key, keyparser[i].keyname)) {
      return keyparser[i].keyval;
    }
  }
  return UNKNOWN;
}

private char *
__get_random_string(size_t length, unsigned int *randseed)
{
  const unsigned char b64_alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ./";
  unsigned char *result;
  size_t i;

  result = xmalloc(sizeof(unsigned char) * (length + 1));

  for(i = 0; i < length; i++)
    result[i] = (int) (255.0 * (pthread_rand_np(randseed) / (RAND_MAX + 1.0)));
  for (i = 0; i < length; i++)
    result[i] = b64_alphabet[(result[i] % ((sizeof(b64_alphabet) - 1) / sizeof(unsigned char)))];

  result[length] = '\0';

  return (char *) result;
}


#define DIGEST_CNONCE_SIZE 16

private DCRED *
__digest_credentials(CREDS creds, size_t *randseed)
{
  DCRED *result;

  result = xcalloc(1, sizeof(struct DIGEST_CRED));
  result->username = xstrdup(creds_get_username(creds));
  result->password = xstrdup(creds_get_password(creds));
  /* Generate a pseudo random cnonce */
  result->cnonce_value = __get_random_string(DIGEST_CNONCE_SIZE, randseed);
  result->nc_value = 1U;
  snprintf(result->nc, sizeof(result->nc), "%.8x", result->nc_value);
  result->h_a1 = NULL;

  return result;
}

private DCHLG *
__digest_challenge(const char *challenge)
{
  DCHLG *result;
  char  *key; 
  char  *val;
  const char *beg; 
  const char *end;
  KEY_HEADER_E keyval;

  result = xcalloc(1, sizeof(struct DIGEST_CHLG));

  for (beg = end = challenge; !isspace(*end) && *end; ++end);

  if (strncasecmp("Digest", beg, end - beg)) {
    fprintf(stderr, "no Digest keyword in challenge [%s]\n", challenge);
    return NULL;
  }

  for (beg = end; isspace(*beg); ++beg);

  while (*beg != '\0') {

    /* find key */
    while (isspace(*beg))
      beg++;

    end = beg;
    while (*end != '=' && *end != ',' && *end != '\0' && !isspace(*end))
      end++;

    key = xmalloc((1 + end - beg) * sizeof(char));
    memcpy(key, beg, end - beg);
    key[end - beg] = '\0';

    beg = end;
    while (isspace(*beg))
      beg++;

    /* find value */
    val = NULL;
    if (*beg == '=') {
      beg++;
      while (isspace(*beg))
        beg++;

      if (*beg == '\"') {     /* quoted string */
        beg++;
        end = beg;
        while (*end != '\"' && *end != '\0') {
          if (*end == '\\' && end[1] != '\0') {
            end++;      /* escaped char */
          }
          end++;
        }
        val = xmalloc((1 + end - beg) * sizeof(char));
        memcpy(val, beg, end - beg);
        val[end - beg] = '\0';
        beg = end;
        if (*beg != '\0') {
          beg++;
        }
      }
      else {              /* token */
        end = beg;
        while (*end != ',' && *end != '\0' && !isspace(*end))
          end++;

        val = xmalloc((1 + end - beg) * sizeof(char));
        memcpy(val, beg, end - beg);
        val[end - beg] = '\0';
        beg = end;
      }
    }

    while (*beg != ',' && *beg != '\0')
      beg++;

    if (*beg != '\0') {
      beg++;
    }
 
    keyval = __get_keyval(key);
    switch (keyval) {
      case REALM:
      result->realm = val;
      break;
      case DOMAIN:
      result->domain = val;
      break;
      case NONCE:
      result->nonce = val;
      break;
      case OPAQUE:
      result->opaque = val;
      break;
      case STALE:
      result->stale = val;
      break;
      case ALGORITHM:
      result->algorithm = val;
      break;
      case QOP:
      result->qop = val;
      break;
      default:
      fprintf(stderr, "unknown key [%s]\n", key);
      xfree(val);
      break;
    }
    xfree(key);
  }

  return result;
}

private char *
__get_md5_str(const char *buf)
{
  const char *hex = "0123456789abcdef";
  struct md5_ctx ctx;
  unsigned char hash[16];
  char *r, *result;
  size_t length;
  int i;

  length = strlen(buf);
  result = xmalloc(33 * sizeof(char));
  md5_init_ctx(&ctx);
  md5_process_bytes(buf, length, &ctx);
  md5_finish_ctx(&ctx, hash);

  for (i = 0, r = result; i < 16; i++) {
    *r++ = hex[hash[i] >> 4];
    *r++ = hex[hash[i] & 0xF];
  }
  *r = '\0';

  return result;
}


private char *
__get_h_a1(const DCHLG *chlg, DCRED *cred, const char *nonce_value)
{
  char *h_usrepa, *result, *tmp;

  if (0 == strcasecmp("MD5", chlg->algorithm)) {
    tmp = xstrcat(cred->username, ":", chlg->realm, ":", cred->password, NULL);
    h_usrepa = __get_md5_str(tmp);
    xfree(tmp);
    result = h_usrepa;
  }
  else if (0 == strcasecmp("MD5-sess", chlg->algorithm)) {
    if ((NULL == cred->h_a1)) {
      tmp = xstrcat(cred->username, ":", chlg->realm, ":", cred->password, NULL);
      h_usrepa = __get_md5_str(tmp);
      xfree(tmp);
      tmp = xstrcat(h_usrepa, ":", nonce_value, ":", cred->cnonce_value, NULL);
      result = __get_md5_str(tmp);
      xfree(tmp);
      cred->h_a1 = result;
    }
    else {
      return cred->h_a1;
    }
  }
  else {
    fprintf(stderr, "invalid call to %s algorithm is [%s]\n", __FUNCTION__, chlg->algorithm);
    return NULL;
  }

  return result;
}

private BOOLEAN
__str_list_contains(const char *str, const char *pattern, size_t pattern_len)
{
  const char *ptr;

  ptr = str;
  do {
    if (0 == strncmp(ptr, pattern, pattern_len)
        && ((',' == ptr[pattern_len]) || ('\0' == ptr[pattern_len]))) {
      return TRUE;
    }

    if (NULL != (ptr = strchr(ptr, ','))) ptr++;
  }
  while (NULL != ptr);

  return FALSE;
}

