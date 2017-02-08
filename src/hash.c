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
#include <hash.h>
#include <memory.h>
#include <joedog/defs.h>

typedef struct NODE
{
  char  *key;
  void  *val;
  struct NODE *next;
} NODE;

struct HASH_T
{
  int    size;
  int    entries;
  int    index;
  NODE   **table;
  method free; 
};

size_t HASHSIZE = sizeof(struct HASH_T);

/** 
 * local prototypes
 */
private BOOLEAN      __lookup(HASH this, char *key);
private void         __resize(HASH this); 
private unsigned int __genkey(int size, char *str);
private u_int32_t    fnv_32_buf(void *buf, size_t len, u_int32_t hval); 

/**
 * Constructs an empty hash map with an initial 
 * capacity of 10240 entries.
 */
HASH 
new_hash()
{
  HASH this;
  int  size = 10240;
  this = calloc(HASHSIZE,1);
  this->size    = size;
  this->entries = 0;
  this->index   = 0;
  while (this->size < size) {
    this->size <<= 1;
  }
  this->table  = (NODE**)calloc(this->size * sizeof(NODE*), 1);
  this->free   = NULL;
  return this;
}

/**
 * Returns the number of key-value mappings in this hash.
 */
int
hash_size(HASH this)
{
  return this->entries;
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
 * add a key value pair to the hash table.
 * This function tests the size of the table
 * and dynamically resizes it as necessary.
 * len is the size of void pointer.
 */
void
hash_add(HASH this, char *key, void *val)
{
  size_t len = 0;
  if (__lookup(this, key) == TRUE)
    return; 

  len = strlen(val);
  hash_nadd(this, key, val, len);
  return;
}

void
hash_nadd(HASH this, char *key, void *val, size_t len)
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
  node->val      = xmalloc(len+1);
  memset(node->val,  '\0', len+1);
  memcpy(node->val,  val, len);
  node->next     = this->table[x];
  this->table[x] = node;
  this->entries++;
  return;
}


/**
 * returns a void NODE->val element
 * in the table corresponding to key.
 */
void *
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

/**
 * Removes and element from the hash; if
 * a function wasn't assigned, then it uses
 * free. You can assign an alternative method
 * using hash_remover or assign it in advance
 * with hash_set_destroyer
 */
void
hash_remove(HASH this, char *key)
{
  int   x  = 0;
  NODE *n1 = NULL;
  NODE *n2 = NULL;

  if (__lookup(this, key) == FALSE)
    return;

  if (this->free == NULL) {
    this->free = free;
  }

  x  = __genkey(this->size, key);
  n1 = this->table[x];
  while (n1 != NULL) {
    n2 = n1->next;
    if (n1->key != NULL) {
      xfree(n1->key);
      this->entries--;
    }
    if (n1->val != NULL)
      this->free(n1->val);
    xfree(n1);
    n1 = n2;
  }
  this->table[x] = NULL;
}

void
hash_remover(HASH this, char *key, method m)
{
  this->free = m;
  hash_remove(this, key);
}

void
hash_set_destroyer(HASH this, method m)
{
  if (this == NULL) return;

  this->free = m;
}


BOOLEAN 
hash_contains(HASH this, char *key) 
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

  if (this == NULL || this->entries == 0) return NULL;


  keys = (char**)malloc(sizeof(char*) * this->entries);
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
 * Destroy the hash table and free
 * memory which was allocated to it.
 */
HASH
hash_destroy(HASH this)
{
  int x;
  NODE *t1, *t2;

  if (this == NULL) {
    return this;
  } 

  if (this->free == NULL) {
    this->free = free;
  }

  for (x = 0; x < this->size; x++) {
    t1 = this->table[x];
    while (t1 != NULL) {
      t2 = t1->next;
      if (t1->key != NULL)
        xfree(t1->key);
      if (t1->val != NULL)
        this->free(t1->val);
      xfree(t1);
      t1 = t2;      
    } 
    this->table[x] = NULL;
  }
  if (this->table != NULL) {
    xfree(this->table);
    memset(this, '\0', sizeof(struct HASH_T));
  } 
  xfree(this);
  return NULL;
}

HASH
hash_destroyer(HASH this, method m)
{
  this->free = m;
  return hash_destroy(this);
}

int
hash_get_entries(HASH this)
{
  return this->entries;
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
 * Fowler/Noll/Vo hash
 *
 * The basis of this hash algorithm was taken from an idea sent
 * as reviewer comments to the IEEE POSIX P1003.2 committee by:
 *
 * Phong Vo (http://www.research.att.com/info/kpv/)
 * Glenn Fowler (http://www.research.att.com/~gsf/)
 *
 * In a subsequent ballot round:
 *
 * Landon Curt Noll (http://www.isthe.com/chongo/)
 *
 * improved on their algorithm.  Some people tried this hash
 * and found that it worked rather well.  In an EMail message
 * to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.
 *
 * FNV hashes are designed to be fast while maintaining a low
 * collision rate. The FNV speed allows one to quickly hash lots
 * of data while maintaining a reasonable collision rate.  
 *
 * See: http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *
 * This code is in the public domain.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
private unsigned int
fnv_32_buf(void *buf, size_t len, unsigned int hval) {
  unsigned char *bp = (unsigned char *)buf; /* start of buffer */
  unsigned char *be = bp + len;             /* beyond end of buffer */

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

