/**
 * Hash Table
 *
 * Copyright (C) 2003-2015 by
 * Jeffrey Fulmer - <jeff@joedog.org>, et al. 
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef  HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif/*HAVE_SYS_TYPES_H*/
#include <hash.h>
#include <joedog/joedog.h>
#include <joedog/defs.h>

typedef struct NODE
{
  char  *key;
  char  *val;
  HASH   hash;
  struct NODE *next;
} NODE;

struct HASH_T
{
  int   size;
  int   entries;
  int   index;
  NODE  **table;
};

#define xfree(x) free(x)
#define xmalloc(x) malloc(x)

/** 
 * local prototypes
 */
private BOOLEAN      __lookup(HASH this, char *key);
private void         __resize(HASH this); 
private unsigned int __genkey(int size, char *str);
private u_int32_t    fnv_32_buf(void *buf, size_t len, u_int32_t hval); 

/**
 * allocs size and space for the 
 * hash table. 
 */
HASH 
new_hash()
{
  HASH this;
  int  size = 10240;

  this = calloc(sizeof(*this),1);
  this->size    = size;
  this->entries = 0;
  this->index   = 0;
  while (this->size < size) {
    this->size <<= 1;
  }
  this->table = (NODE**)calloc(this->size * sizeof(NODE*), 1);
  return this;
}

void
hash_reset(HASH this, ssize_t size)
{
  this->size    = 2;
  this->entries = 0;

  while (this->size < size) {
    this->size <<= 1;
  }

  this->table = (NODE**)calloc(this->size * sizeof(NODE*), 1);
  return;
}

/**
 * redoubles the size of the hash table. 
 * This is a local function called by hash_add 
 * which dynamically resizes the table as 
 * necessary.
 */
private void
__resize(HASH this) 
{
  NODE *tmp;
  NODE *last_node; 
  NODE **last_table;
  int  x, hash, size;
 
  size       = this->size; 
  last_table = this->table;

  hash_reset(this, size*2);

  x = 0;
  while (x < size) {
    last_node = last_table[x]; 
    while (last_node != NULL) {
      tmp       = last_node;
      last_node = last_node->next;
      hash      = __genkey(this->size, (char*)tmp->key);
      tmp->next = this->table[hash];
      this->table[hash] = tmp;
      this->entries++;
    } 
    x++;
  } 
  return;
}

/**
 * add a key value pair to the hash table.
 * This function tests the size of the table
 * and dynamically resizes it as necessary.
 * len is the size of void pointer.
 */
void
hash_add(HASH this, char *key, char *val)
{
  int  x;
  NODE *node;

  if (__lookup(this, key) == TRUE)
    return;

  if (this->entries >= this->size/4)
    __resize(this);

  x = __genkey(this->size, key);
  node           = xmalloc(sizeof(NODE));
  node->key      = strdup(key);
  node->val      = strdup(val);
  node->next     = this->table[x]; 
  node->hash     = new_hash();
  this->table[x] = node;
  this->entries++;
  return;
}

void 
hoh_add(HASH this, char *key, char *k, char *v) 
{
  int  x;
  NODE *node;

  if (__lookup(this, key) == FALSE) {
    // we don't have a corresponding hash
    if (this->entries >= this->size/4)
      __resize(this);
    x = __genkey(this->size, key); 
    node           = xmalloc(sizeof(NODE));
    node->key      = strdup(key);
    node->val      = strdup("HOH");
    node->hash     = new_hash();
    node->next     = this->table[x]; 
    hash_add(node->hash, k, v);
    this->table[x] = node;
    this->entries++;
  } else {
    // we already have a hash; we can just add an entry
    x = __genkey(this->size, key);
    for (node = this->table[x]; node != NULL; node = node->next) {
      if (!strcmp(node->key, key)) {
        hash_add(node->hash, k, v);
      } 
    }
  } 
  return;
}

/**
 * returns a void NODE->val element
 * in the table corresponding to key.
 */
char *
hash_get(HASH this, char *key)
{
  int  x;
  NODE *node;

  x = __genkey(this->size, key);
  for (node = this->table[x]; node != NULL; node = node->next) {
    if (!strcmp(node->key, key)) {
      return(node->val);
    }
  }

 return NULL;
} 

HASH
hoh_get(HASH this, char *key) 
{
  int  x;
  NODE *node;
  
  x = __genkey(this->size, key);
  for (node = this->table[x]; node != NULL; node = node->next) {
    if (!strcmp(node->key, key)) {
      return(node->hash);
    }
  }
  return NULL;
}

BOOLEAN 
hash_lookup(HASH this, char *key) 
{
  return __lookup(this, key);
}


char **
hash_get_keys(HASH this)
{
  int x; 
  int i = 0;
  NODE *node;
  char **keys;

  keys = (char**)malloc(sizeof( char*) * this->entries);
  for (x = 0; x < this->size; x ++) {
    for(node = this->table[x]; node != NULL; node = node->next){
      size_t len = strlen(node->key)+1;
      keys[i] = (char*)malloc(len);
      memset(keys[i], '\0', len);
      memcpy(keys[i], (char*)node->key, strlen(node->key));
      i++;
    }
  }
  return keys;
}

void 
hash_free_keys(HASH this, char **keys)
{
  int x;
  for (x = 0; x < this->entries; x ++)
    if (keys[x] != NULL) {
      char *tmp = keys[x];
      xfree(tmp);
    }
  xfree(keys);

  return;
}

/**
 * destroy the hash table and free
 * memory which was allocated to it.
 */
void 
hash_destroy(HASH this)
{
  int x;
  NODE *t1, *t2;

  if (this == NULL) return;

  for (x = 0; x < this->size; x++) {
    t1 = this->table[x];
    while (t1 != NULL) {
      t2 = t1->next;
      if (t1->key != NULL)
        xfree(t1->key);
      if (t1->val != NULL)
        xfree(t1->val);
      if (t1->hash != NULL) 
        hash_destroy(t1->hash);
      xfree(t1);
      t1 = t2;      
    } 
    this->table[x] = NULL;
  }
  if (this->table != NULL) {
    xfree(this->table);
    memset(this, 0, sizeof(struct HASH_T));
  } 
  xfree(this);
  return;
}

int
hash_get_entries(HASH this)
{
  return this->entries;
}

/**
 * returns int cache key for the table
 */
private unsigned int
fnv_32_buf(void *buf, size_t len, unsigned int hval) {
  unsigned char *bp = (unsigned char *)buf;   /* start of buffer */
  unsigned char *be = bp + len;               /* beyond end of buffer */

  /**
   * FNV-1a hash each octet in the buffer
   */
  while (bp < be) {
    hval ^= (u_int32_t)*bp++;
    hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
  }
  return hval;
}

/**
 * returns int hash key for the table
 */
#define FNV1_32_INIT ((unsigned int)2166136261LL)

private unsigned int
__genkey(int size, char *str) {
  unsigned int hash;
  void *data = str;

  hash = fnv_32_buf(data, strlen(str), FNV1_32_INIT);
  hash %= size;
  return hash;
}


/**
 * returns TRUE if key is present in the table
 * and FALSE if it is not.
 */
private BOOLEAN
__lookup(HASH this, char *key)
{
  int  x;
  NODE *node;

  if (key == NULL) { return FALSE; }
  x = __genkey(this->size, key);
  for (node = this->table[x]; node != NULL; node = node->next) {
    if (!strcmp(node->key, key)) {
      return TRUE;
    }
  }
  return FALSE;
}

#if 0
int 
main()
{
  int    i, j;
  char **keys;
  HASH HOH = new_hash();
  hoh_add(HOH,
    "139804567041792",
    "1", "haha=Whoo+hoo%21; domain=.armstrong.com; path=/; expires=1449261008"
  );
  hoh_add(HOH, 
    "139804567041792",
    "2", "exes=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX; domain=.armstrong.com; path=/; expires=1449253423"
  );
  hoh_add(HOH,
    "139804577531648", 
    "3", "haha=Whoo+hoo%21; domain=.armstrong.com; path=/; expires=1449261009"
  );
  hoh_add(HOH, 
    "139804577531648", 
    "4", "exes=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX; domain=.armstrong.com; path=/; expires=1449253423"
  );
  hoh_add(HOH,
    "139804588021504",
    "5", "haha=Whoo+hoo%21; domain=.armstrong.com; path=/; expires=1449261009"
  );
  hoh_add(HOH,
    "139804588021504",
    "6", "exes=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX; domain=.armstrong.com; path=/; expires=1449253423"
  );

  keys = hash_get_keys(HOH);
  for (i = 0; i < hash_get_entries(HOH); i ++){
    char **k;
    HASH hash = hoh_get(HOH, keys[i]);
    k = hash_get_keys(hash);
    for (j = 0; j < hash_get_entries(hash); j ++){
      char *tmp = (char*)hash_get(hash, k[j]);
      printf("%s: %s => %s\n", keys[i], k[j], (tmp==NULL)?"NULL":tmp);
    }
    hash_destroy(hash);
  }
}
#endif

#if 0
int
main()
{
  HASH H = new_hash();
  int  i = 0;
  int  j;
  char **keys;
  int  len = 24;
  char *simpsons[][2] = {
    {"Homer",          "D'oh!!!"},
    {"Homey",          "Whoo hoo!"},
    {"Homer J.",       "Why, you little!"},
    {"Marge",          "Hrrrrmph"},
    {"Bart",           "Aye Caramba!"},
    {"Lisa",           "I'll be in my room"},
    {"Maggie",         "(pacifier suck)"},
    {"Ned",            "Okily Dokily!"},
    {"Moe",            "WhaaaAAAAT?!"},
    {"Comic Book Guy", "Worst. Episode. Ever!"},
    {"Waylon",         "That's Homer Simpson from sector 7G"},
    {"Barnie",         "Brrraaarrp!"},
    {"Krusty",         "Hey! Hey! Kids!"},
    {"Frink",          "Glavin!"},
    {"Burns",          "Release the hounds!"},
    {"Nelson",         "Hah! Haw!"},
    {"Dr. Nick",       "Hi, everybody!"},
    {"Edna",           "Hah!"},
    {"Helen",          "Think of the children!"},
    {"Snake",          "Dude!"},
    {"Chalmers",       "SKINNER!"},
    {"Agnes",          "SEYMOUR!"},
    {"Sea Cap'n",      "Yargh!"},
    {"Sideshow Bob",   "Hello, Bart!"},
  };

  for (i = 0; i < len; i++) {
    hash_add(H, simpsons[i][0], simpsons[i][1]);
  }

  keys = hash_get_keys(H);
  for (j = 0; j < 100000; j++) {
    for (i = 0; i < hash_get_entries(H); i ++){
      char *tmp = (char*)hash_get(H, keys[i]);
      printf("%16s => %s\n", keys[i], (tmp==NULL)?"NULL":tmp);
    }
  }
  hash_free_keys(H, keys);
  hash_destroy(H);
  exit(0); 
}
#endif

