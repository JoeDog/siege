#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#ifdef HAVE_LIBUUID
#include <uuid/uuid.h>
void generate_uuid(char *uuid_str, unsigned int id) {
  (void)id;
  uuid_t uuid;

  srand((unsigned int) time(NULL));
  uuid_generate(uuid);
  uuid_unparse_lower(uuid, uuid_str);
}
#else
/**
 *UUIDv4 ID in std C library
 */
void generate_uuid(char *uuid_str, unsigned int id) {
  int i; 
  unsigned int seed = (unsigned int)(time(NULL) ^ id);
  uint8_t uuid[16];
  const char *hex = "0123456789abcdef";

  for (i = 0; i < 16; i++) {
    uuid[i] = rand_r(&seed) % 256;
  }

  // UUID version 4 and variant bits
  uuid[6] = (uuid[6] & 0x0F) | 0x40;
  uuid[8] = (uuid[8] & 0x3F) | 0x80;

  int pos = 0;
  for (i = 0; i < 16; i++) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      uuid_str[pos++] = '-';
    }
    uuid_str[pos++] = hex[(uuid[i] >> 4) & 0xF];
    uuid_str[pos++] = hex[uuid[i] & 0xF];
  }
  uuid_str[pos] = '\0';
}
#endif

