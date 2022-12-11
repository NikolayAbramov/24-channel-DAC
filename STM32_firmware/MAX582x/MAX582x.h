#include <inttypes.h>

int8_t max582x_write(uint8_t, uint16_t);
int8_t max582x_read(uint8_t, uint16_t *);

void max582_set_addr(uint8_t);

int8_t max582x_init_ref(void);
int8_t max582x_init(void);

int8_t max582x_code_load(uint8_t, uint16_t);
int8_t max582x_read_code(uint8_t, uint16_t*);
