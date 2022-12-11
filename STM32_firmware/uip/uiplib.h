#include "uip.h"

#define fill(str) fill_fmt(str)

unsigned char h2int(char);

uint8_t parse_list(char *, char, uint8_t *, int8_t, uint8_t);

uint16_t htoi(char *);

//uint16_t fill(uint16_t , char* );

void fill_fmt(char* , ...);

uint8_t url_strcmp(const char *, uint8_t **);

uint8_t find_key_value(char *, char *, uint8_t, char *);

uint8_t get_num_of_conns(uint8_t);
