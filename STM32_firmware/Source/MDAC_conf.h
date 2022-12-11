#include <inttypes.h>
//Number of DAC ICs
#define DAC_N 3 
//Number of channels per IC
#define DAC_NCHAN 8
#define NCHAN 24

#define DAC_MAX 4095
#define DAC_MID 2048
#define DAC_REF 4.096
#define DAC_RES 2e-3

//I2C slave addresses of ICs
extern uint8_t DAC_addr[DAC_N];
//DATA buffer
extern uint16_t DAC_buffer[NCHAN];

extern uint8_t global_flags;
#define DAC_ERROR 1
#define DAC_FAULT 2
#define CHANGE_IP 4
#define CHANGE_MAC 8

#define VOLT(ch) DAC_RES*(float)((int16_t)(DAC_buffer[ch])-DAC_MID)
