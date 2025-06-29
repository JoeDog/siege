#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <uuid.h>

char *
uuid_header(char *input) 
{
  const char *token = "{uuid}";
  char uuid_str[37];

  const char *pos = strstr(input, token);
  if (!pos) {
    printf("Token not found\n");
    return input;
  }

  generate_uuid(uuid_str, 10);
  size_t prefix_len = pos - input;
  size_t suffix_len = strlen(pos + strlen(token));
  size_t total_len = prefix_len + strlen(uuid_str) + suffix_len + 1;

  char *result = malloc(total_len);
  if (!result) {
    return input;
  }

  memcpy(result, input, prefix_len);
  memcpy(result + prefix_len, uuid_str, strlen(uuid_str));
  memcpy(result + prefix_len + strlen(uuid_str),
         pos + strlen(token),
         suffix_len);
  result[total_len - 1] = '\0';
  return result;
}

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

