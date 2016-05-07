/**
 * HTTP Authentication
 *
 * Copyright (C) 2002-2016 by
 * Jeffrey Fulmer - <jeff@joedog.org>, et al.
 * This file is distributed as part of Siege
 *
 * Additional copyright information is noted below
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
#include <memory.h>
#include <notify.h>

#ifdef HAVE_SSL
#include <openssl/des.h>
#include <openssl/md4.h>
#include <openssl/opensslv.h>
# if OPENSSL_VERSION_NUMBER < 0x00907001L
#  define DES_key_schedule des_key_schedule
#  define DES_cblock des_cblock
#  define DES_set_odd_parity des_set_odd_parity
#  define DES_set_key des_set_key
#  define DES_ecb_encrypt des_ecb_encrypt
#  define DESKEY(x) x
#  define DESKEYARG(x) x
# else
#  define DESKEYARG(x) *x
#  define DESKEY(x) &x
# endif
#endif

typedef enum 
{
  TYPE_N = 0,
  TYPE_1 = 1,
  TYPE_2 = 2,
  TYPE_3 = 3,
  TYPE_L = 4
} STATE;

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
    STATE state;
    char *encode;
    BOOLEAN  ready;
    unsigned char nonce[8];
  } ntlm;
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

/**
 * Flag bits definitions available at on
 * http://davenport.sourceforge.net/ntlm.html
 */
#define NTLMFLAG_NEGOTIATE_OEM                   (1<<1)
#define NTLMFLAG_NEGOTIATE_NTLM_KEY              (1<<9)
#define BASE64_LENGTH(len) (4 * (((len) + 2) / 3))

size_t AUTHSIZE = sizeof(struct AUTH_T);

private BOOLEAN       __basic_header(AUTH this, SCHEME scheme, CREDS creds);
private BOOLEAN       __ntlm_header(AUTH this, SCHEME scheme, const char *header, CREDS creds);
private DCHLG *       __digest_challenge(const char *challenge);
private DCRED *       __digest_credentials(CREDS creds, unsigned int *randseed);
private KEY_HEADER_E  __get_keyval(const char *key);
private char *        __get_random_string(size_t length, unsigned int *randseed);
private char *        __get_h_a1(const DCHLG *chlg, DCRED *cred, const char *nonce_value);
private char *        __get_md5_str(const char *buf);
private BOOLEAN       __str_list_contains(const char *str, const char *pattern, size_t pattern_len);
#ifdef HAVE_SSL
private void          __mkhash(const char *pass, unsigned char *nonce, unsigned char *lmresp, unsigned char *ntresp);
#endif/*HAVE_SSL*/

AUTH
new_auth()
{
  AUTH this;

  this = calloc(AUTHSIZE, 1);
  this->creds  = new_array();
  this->basic.encode  = xstrdup("");
  this->digest.encode = xstrdup("");
  this->ntlm.encode   = xstrdup("");
  this->ntlm.state    = TYPE_N;
  this->proxy.encode  = xstrdup("");
  return this;
}

AUTH
auth_destroy(AUTH this)
{
  this->creds = array_destroy(this->creds);
  xfree(this->basic.encode);
  xfree(this->digest.encode);
  xfree(this->ntlm.encode);
  xfree(this->proxy.encode);
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
    if (realm == NULL) break;
    if (strmatch(creds_get_realm(tmp), realm)) {
      if (creds_get_scheme(tmp) == scheme) {
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
      if (creds_get_scheme(tmp) == scheme) {
        return __basic_header(this, scheme, tmp); 
      }
    }
  }
  return FALSE; 
}

BOOLEAN
auth_set_ntlm_header(AUTH this, SCHEME scheme, char *header, char *realm) 
{
  size_t i;
  for (i = 0; i < array_length(this->creds); i++) {
    CREDS tmp = array_get(this->creds, i);
    if (realm == NULL) break;
    if (strmatch(creds_get_realm(tmp), realm)) {
      if (creds_get_scheme(tmp) == HTTP || creds_get_scheme(tmp) == HTTPS) {
        return __ntlm_header(this, scheme, header, tmp);
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
        return __ntlm_header(this, scheme, header, tmp);
      }
    }
  }
  return FALSE;
}

char *
auth_get_ntlm_header(AUTH this, SCHEME scheme)
{
  if (scheme == PROXY) {
    return this->proxy.encode;
  } else {
    return this->ntlm.encode;
  }
}

char *
auth_get_digest_header(AUTH this, SCHEME scheme, DCHLG *chlg, DCRED *cred, const char *method, const char *uri)
{
  size_t len;
  char  *cnonce         = NULL;
  char  *nonce_count    = NULL;
  char  *qop            = NULL;
  char  *response       = NULL;
  char  *request_digest = NULL;
  char  *h_a1           = NULL;
  char  *h_a2           = NULL;
  char  *opaque         = NULL;
  char  *result         = NULL; 
  char  *tmp            = NULL;

  /**
   * The user probably didn't set login credentials. 
   * We'll return "" here and display a message after
   * the authorization failure.
   */
  if (chlg == NULL || cred == NULL) return "";

  if (chlg != NULL && chlg->qop != NULL) {
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
  if (chlg != NULL && chlg->opaque != NULL)
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
auth_set_digest_header(AUTH this, DCHLG **chlg, DCRED **cred, unsigned int *rand, char *realm, char *str) {
  size_t  i;
  for (i = 0; i < array_length(this->creds); i++) {
    CREDS tmp = array_get(this->creds, i);
    if (realm == NULL) break;
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
  if (this == NULL) 
    return FALSE;
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

/**
 * NTLM authentication is based on Daniel Stenberg's 
 * implementation from libcurl and contribution to wget. 
 * See: http://curl.haxx.se and http://www.gnu.org/software/wget/
 * Copyright (c) 1996 - 2015, Daniel Stenberg, daniel@haxx.se.
 * MIT (or Modified BSD)-style license. 
 * Copyright (C) 2011 by Free Software Foundation 
 * Modified for Siege by J. Fulmer
 * Copyright (C) 2015 GNU Public License v3
 */
#define SHORTPAIR(x) (char) ((x) & 0xff), (char) ((x) >> 8)
#define LONGQUARTET(x) ((x) & 0xff), (((x) >> 8)&0xff), \
  (((x) >>16)&0xff), ((x)>>24)

private BOOLEAN
__ntlm_header(AUTH this, SCHEME scheme, const char *header, CREDS creds)
{
#ifndef HAVE_SSL
  NOTIFY(
    ERROR, "NTLM authentication requires libssl: %d, %d, %s, %s", 
    this->okay, scheme, header, creds_get_username(creds)
  ); // this message is mainly intended to silence the compiler
  return FALSE;
#else
  size_t size        = 0;
  size_t final       = 0;
  const char *domstr = "";
  const char *srvstr = ""; 
  size_t domlen      = strlen(domstr);
  size_t srvlen      = strlen(srvstr);
  size_t srvoff; 
  size_t domoff; 
  char *hdr;
  char  tmp[BUFSIZ];
  char  buf[256]; 

  /**
   * The header is head->auth.challenge.www which was set in client.c
   * It includes everything after 'WWW-Authenticate: ' which means it
   * must begin NTLM for this authentication mechanism. Let's check it
   */
  if (strncasecmp(header, "NTLM", 4)) {
    return FALSE;
  }

  NOTIFY( // honestly this is here to silence the compiler...
    DEBUG, "Parsing NTLM header:  %d, %d, %s, %s",
    this->okay, scheme, header, creds_get_username(creds)
  );

  header += 4; // Step past NTLM
  while (*header && ISSPACE(*header)) {
    header++;
  }

  if (*header) {
    /**
     * We got a type-2 message here:
     *
     *  Index   Description         Content
     *    0     NTLMSSP Signature   Null-terminated ASCII "NTLMSSP"
     *                                (0x4e544c4d53535000)
     *    8     NTLM Message Type   long (0x02000000)
     *   12     Target Name         security buffer(*)
     *   20     Flags               long
     *   24     Challenge           8 bytes
     *  (32)    Context (optional)  8 bytes (two consecutive longs)
     *  (40)    Target Information  (optional) security buffer(*)
     *   32 (48) start of data block
     */
    ssize_t size;
    memset(tmp, '\0', strlen(header));
    if ((size = base64_decode(header, &tmp)) < 0) {
      return FALSE;
    }

    if (size >= 48) {
      /* the nonce of interest is index [24 .. 31], 8 bytes */
      memcpy (this->ntlm.nonce, &tmp[24], 8);
    }
    this->ntlm.state = TYPE_2;
  } else {
    if (this->ntlm.state >= TYPE_1) {
      return FALSE; /* this is an error */
    }
    this->ntlm.state = TYPE_1; /* we should send type-1 */
  }

  switch (this->ntlm.state) {
    case TYPE_1:
    case TYPE_N:
    case TYPE_L:
      srvoff = 32;
      domoff = srvoff + srvlen;
      /** 
       * Create and send a type-1 message:
       * Index Description          Content
       *  0    NTLMSSP Signature    Null-terminated ASCII "NTLMSSP"
       *                            (0x4e544c4d53535000)
       *  8    NTLM Message Type    long (0x01000000)
       * 12    Flags                long
       * 16    Supplied Domain      security buffer(*)
       * 24    Supplied Workstation security buffer(*)
       * 32    start of data block
       */
      snprintf (
        buf, sizeof(buf), "NTLMSSP%c"
        "\x01%c%c%c" /* 32-bit type = 1 */
        "%c%c%c%c"   /* 32-bit NTLM flag field */
        "%c%c"       /* domain length */
        "%c%c"       /* domain allocated space */
        "%c%c"       /* domain name offset */
        "%c%c"       /* 2 zeroes */
        "%c%c"       /* host length */
        "%c%c"       /* host allocated space */
        "%c%c"       /* host name offset */
        "%c%c"       /* 2 zeroes */
        "%s"         /* host name */
        "%s",        /* domain string */
        0,           /* trailing zero */
        0,0,0,       /* part of type-1 long */
        LONGQUARTET(
          /* equals 0x0202 */
          NTLMFLAG_NEGOTIATE_OEM|      /*   2 */
          NTLMFLAG_NEGOTIATE_NTLM_KEY  /* 200 */
        ),
        SHORTPAIR(domlen),
        SHORTPAIR(domlen),
        SHORTPAIR(domoff),
        0,0,
        SHORTPAIR(srvlen),
        SHORTPAIR(srvlen),
        SHORTPAIR(srvoff),
        0,0,
        srvstr, domstr
      );
      size   = 32 + srvlen + domlen;
      if ((base64_encode(buf, size, &hdr) < 0 )) {
        return FALSE;
      }

      final  =  strlen(hdr) + 23;
      this->ntlm.encode = xmalloc(final);
      this->ntlm.state = TYPE_2; /* we sent type one */
      memset(this->ntlm.encode, '\0', final);
      snprintf(this->ntlm.encode, final, "Authorization: NTLM %s\015\012", hdr);
      break;
    case  TYPE_2:
      /**
       * We have type-2; need to create a type-3 message:
       *
       * Index    Description            Content
       *   0      NTLMSSP Signature      Null-terminated ASCII "NTLMSSP"
       *                                 (0x4e544c4d53535000)
       *   8      NTLM Message Type      long (0x03000000)
       *  12      LM/LMv2 Response       security buffer(*)
       *  20      NTLM/NTLMv2 Response   security buffer(*)
       *  28      Domain Name            security buffer(*)
       *  36      User Name              security buffer(*)
       *  44      Workstation Name       security buffer(*)
       * (52)     Session Key (optional) security buffer(*)
       * (60)     Flags (optional)       long
       *  52 (64) start of data block
       */
      {
        size_t lmrespoff;
        size_t ntrespoff;
        size_t usroff;
        unsigned char lmresp[0x18]; /* fixed-size */
        unsigned char ntresp[0x18]; /* fixed-size */
        size_t usrlen;
        const  char *usr;

        usr = strchr(creds_get_username(creds), '\\');
        if (!usr) {
          usr = strchr(creds_get_username(creds), '/');
        }

        if (usr) {
          domstr = creds_get_username(creds);
          domlen = (size_t) (usr - domstr);
          usr++;
        } else {
          usr = creds_get_username(creds);
        }
        usrlen = strlen(usr);
        __mkhash(creds_get_password(creds), &this->ntlm.nonce[0], lmresp, ntresp);
        domoff = 64; 
        usroff = domoff + domlen;
        srvoff = usroff + usrlen;
        lmrespoff = srvoff + srvlen;
        ntrespoff = lmrespoff + 0x18;
        size = (size_t) snprintf (
          buf, sizeof(buf),
          "NTLMSSP%c"
          "\x03%c%c%c"     /* type-3, 32 bits */
          "%c%c%c%c"       /* LanManager length + allocated space */
          "%c%c"           /* LanManager offset */
          "%c%c"           /* 2 zeroes */
          "%c%c"           /* NT-response length */
          "%c%c"           /* NT-response allocated space */
          "%c%c"           /* NT-response offset */
          "%c%c"           /* 2 zeroes */
          "%c%c"           /* domain length */
          "%c%c"           /* domain allocated space */
          "%c%c"           /* domain name offset */
          "%c%c"           /* 2 zeroes */
          "%c%c"           /* user length */
          "%c%c"           /* user allocated space */
          "%c%c"           /* user offset */
          "%c%c"           /* 2 zeroes */
          "%c%c"           /* host length */
          "%c%c"           /* host allocated space */
          "%c%c"           /* host offset */
          "%c%c%c%c%c%c"   /* 6 zeroes */
          "\xff\xff"       /* message length */
          "%c%c"           /* 2 zeroes */
          "\x01\x82"       /* flags */
          "%c%c"           /* 2 zeroes */
                           /* domain string */
                           /* user string */
                           /* host string */
                           /* LanManager response */
                           /* NT response */
          ,
          0,               /* zero termination */
          0,0,0,           /* type-3 long, the 24 upper bits */
          SHORTPAIR(0x18), /* LanManager response length, twice */
          SHORTPAIR(0x18),
          SHORTPAIR(lmrespoff),
          0x0, 0x0,
          SHORTPAIR(0x18), 
          SHORTPAIR(0x18),
          SHORTPAIR(ntrespoff),
          0x0, 0x0,
          SHORTPAIR(domlen),
          SHORTPAIR(domlen),
          SHORTPAIR(domoff),
          0x0, 0x0,
          SHORTPAIR(usrlen),
          SHORTPAIR(usrlen),
          SHORTPAIR(usroff),
          0x0, 0x0,
          SHORTPAIR(srvlen),
          SHORTPAIR(srvlen),
          SHORTPAIR(srvoff),
          0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
          0x0, 0x0,
          0x0, 0x0
        );
        
        size=64;
        buf[62]=buf[63]=0;

       if ((size + usrlen + domlen) >= sizeof(buf))
         return FALSE;

       memcpy(&buf[size], domstr, domlen);
       size += domlen;

       memcpy(&buf[size], usr, usrlen);
       size += usrlen;

       /* we append the binary hashes to the end of the blob */
       if (size < (sizeof(buf) - 0x18)) {
         memcpy(&buf[size], lmresp, 0x18);
         size += 0x18;
       }

       if (size < (sizeof(buf) - 0x18)) {
         memcpy(&buf[size], ntresp, 0x18);
         size += 0x18;
       }

       buf[56] = (char) (size & 0xff);
       buf[57] = (char) (size >> 8);
       if ((base64_encode(buf, size, &hdr) < 0)) {
         return FALSE;
       }
       
       this->ntlm.state = TYPE_3; /* we sent a type-3 */
       this->ntlm.ready = TRUE;
       final  =  strlen(hdr) + 23;
       this->ntlm.encode = (char*)xrealloc(this->ntlm.encode, final);
       memset(this->ntlm.encode, '\0', final);
       snprintf(this->ntlm.encode, final, "Authorization: NTLM %s\015\012", hdr);
       break;
     }
     case TYPE_3:
       this->ntlm.ready = TRUE;
       break;
     default: 
       break;
  }
  return TRUE;
#endif/*HAVE_SSL*/
}

/**
 * Copyright (C) 2011 by Free Software Foundation, Inc.
 * Contributed by Daniel Stenberg.
 * This code is part of wget and based on mk_lm_hash from libcurl
 * Modified by J. Fulmer for siege
 */
#ifdef HAVE_SSL
static void
setup_des_key(unsigned char *key_56, DES_key_schedule DESKEYARG(ks))
{
  DES_cblock key;

  key[0] = key_56[0];
  key[1] = ((key_56[0] << 7) & 0xFF) | (key_56[1] >> 1);
  key[2] = ((key_56[1] << 6) & 0xFF) | (key_56[2] >> 2);
  key[3] = ((key_56[2] << 5) & 0xFF) | (key_56[3] >> 3);
  key[4] = ((key_56[3] << 4) & 0xFF) | (key_56[4] >> 4);
  key[5] = ((key_56[4] << 3) & 0xFF) | (key_56[5] >> 5);
  key[6] = ((key_56[5] << 2) & 0xFF) | (key_56[6] >> 6);
  key[7] =  (key_56[6] << 1) & 0xFF;

  DES_set_odd_parity(&key);
  DES_set_key(&key, ks);
}
#endif/*HAVE_SSL*/

/**
 * Copyright (C) 2011 by Free Software Foundation, Inc.
 * Contributed by Daniel Stenberg.
 * This code is part of wget and based on mk_lm_hash from libcurl
 * Modified by J. Fulmer for siege
 */
#ifdef HAVE_SSL
static void
calc_resp(unsigned char *keys, unsigned char *plaintext, unsigned char *results)
{
  DES_key_schedule ks;

  setup_des_key(keys, DESKEY(ks));
  DES_ecb_encrypt((DES_cblock*) plaintext, (DES_cblock*) results, DESKEY(ks), DES_ENCRYPT);

  setup_des_key(keys+7, DESKEY(ks));
  DES_ecb_encrypt((DES_cblock*) plaintext, (DES_cblock*) (results+8), DESKEY(ks), DES_ENCRYPT);

  setup_des_key(keys+14, DESKEY(ks));
  DES_ecb_encrypt((DES_cblock*) plaintext, (DES_cblock*) (results+16), DESKEY(ks), DES_ENCRYPT);
}
#endif/*HAVE_SSL*/

/**
 * Copyright (C) 2011 by Free Software Foundation, Inc.
 * Contributed by Daniel Stenberg.
 * This code is part of wget and based on mk_lm_hash from libcurl
 * Modified by J. Fulmer for siege
 */
#ifdef HAVE_SSL
private void
__mkhash(const char *password, unsigned char *nonce, unsigned char *lmresp, unsigned char *ntresp)
{
  unsigned char *pw;
  unsigned char lmbuffer[21];
  unsigned char ntbuffer[21];
  static const unsigned char magic[] = {
    0x4B, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25
  };
  size_t i, len = strlen(password);

  pw = (unsigned char *) alloca (len < 7 ? 14 : len * 2);

  if (len > 14)
    len = 14;

  for (i=0; i<len; i++)
    pw[i] = (unsigned char) TOUPPER(password[i]);

  for (; i<14; i++)
    pw[i] = 0;
  {
    DES_key_schedule ks;

    setup_des_key(pw, DESKEY(ks));
    DES_ecb_encrypt((DES_cblock *)magic, (DES_cblock *)lmbuffer, DESKEY(ks), DES_ENCRYPT);

    setup_des_key(pw+7, DESKEY(ks));
    DES_ecb_encrypt((DES_cblock *)magic, (DES_cblock *)(lmbuffer+8), DESKEY(ks), DES_ENCRYPT);
    memset(lmbuffer+16, 0, 5);
  }
  calc_resp(lmbuffer, nonce, lmresp);

  {
    MD4_CTX MD4;
    len = strlen(password);

    for (i=0; i<len; i++) {
      pw[2*i]   = (unsigned char) password[i];
      pw[2*i+1] = 0;
    }

    MD4_Init(&MD4);
    MD4_Update(&MD4, pw, 2*len);
    MD4_Final(ntbuffer, &MD4);

    memset(ntbuffer+16, 0, 5);
  }
  calc_resp(ntbuffer, nonce, ntresp);
}
#endif/*HAVE_SSL*/


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
__digest_credentials(CREDS creds, unsigned int *randseed)
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

