#ifndef PARSER_H
#define PARSER_H

#include <array.h>
#include <url.h>

/**
 * PARSER object
 */
typedef struct PARSER_T *PARSER;
extern  size_t PARSERSIZE;

PARSER  new_parser(URL base, char *page);
PARSER  parser_destroy(PARSER this);
void    parser_reset(PARSER this, URL base);
ARRAY   parser_get(PARSER this);

#endif/*PARSER_H*/
