#include <inttypes.h>
//Answer data type
#define SCPI_NODATA 0
#define SCPI_IDN 		1
#define SCPI_VOLT 	2
#define SCPI_TRUE 	3
#define SCPI_FALSE 	4	
#define SCPI_FORMAT 5
#define SCPI_DATA   6
#define SCPI_KEEPALIVE 7

int8_t scpi_strcmp(const char *, char**);
int8_t parse_data(uint8_t *);

void handle_idn(char *, uint32_t *);
//---------------------------------------------------
void scpi_parse_volt(char * , uint8_t *, uint32_t *);
void scpi_get_volt(uint8_t);
//----------------------------------------------------
void handle_data(char*, uint32_t *);
int8_t parse_float_list(char *);
void scpi_get_data(void);
//---------------------------------------------------
void handle_format(char *, uint32_t *);
int8_t parse_format(char *);
void scpi_get_format(void);
//---------------------------------------------------
void send_keepalive(void);
//--------------------------------------------------
void scpi_ans(uint32_t, uint8_t);
//---------------------------------------------------
int8_t isquery(char *);
