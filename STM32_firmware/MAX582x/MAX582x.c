#include "stm32f0xx.h"
#include "Source/I2C.h"
#include "string.h"
#include "MAX582x.h"

#define ADDR_FRAME 0x10
//Config commands
#define WDOG 0b10

#define REF 0x20
	#define REF_EX 0
	#define REF_2_5V 1
	#define REF_2V 2
	#define REF_4_1V 3
	#define REF_PWR_ALWEYS 0x04

#define SW_GATE_CLR 0x30
#define SW_GATE_SET 0x31
#define WD_REFRESH  0x32
#define WD_RESET 		0x33
#define SW_CLEAR 		0x34
#define SW_RESET 		0x35

#define POWER 				0x40
	#define POWER_NORM		0
	#define POWER_1K_PD		0x40
	#define POWER_100K_PD	0x80
	#define POWER_HIZ_PD	0xC0

#define CONFIG 			0x50
	#define CONFIG_WDOG_DIS 0
	#define CONFIG_WDOG_GATE	0x40
	#define CONFIG_WDOG_CLR 	0x80
	#define CONFIG_WDOG_HOLD 	0xC0
	#define CONFIG_GATE_ENB 	0x20
	#define CONFIG_LDAC_ENB 	0x10
	#define CONFIG_CLEAR_ENB 	0x08

#define DEFAULT 		0x60
	#define	DEFATLT_MZ  		0
	#define DEFAULT_ZERO 		0x20 
	#define DEFAULT_MID 		0x40
	#define DEFAULT_FULL 		0x60
	#define DEFAULT_RETURN 	0x80

#define DATA_FILL_H 0x96
#define DATA_FILL_L 0x30
#define DATA_FILL 0x9630

//DAC commands
#define RETURN 						0x70
#define CODE 							0x80
#define LOAD 							0x90
#define CODE_LOAD_ALL 		0xA0
#define CODE_LOAD 				0xB0
#define CODE_ALL 					0xC0
#define LOAD_ALL 					0xC1
#define CODE_ALL_LOAD_ALL 0xC2
#define RETURN_ALL 				0xC3

#define DAC_DATA_OFFS 4

//Readback
#define READ_DAC    0x90
#define READ_CODE   0x80
#define READ_RETURN 0x70

//Set slave address
void max582_set_addr(uint8_t addr)
{
	addr = (addr | ADDR_FRAME)<<1;
	i2c_set_slave_addr(addr);
}

//Write and verify data
//data lenght is 3 bytes
int8_t max582x_write(uint8_t addr, uint16_t data)
{
	uint8_t buf[3];
	
	buf[0] = addr;
	buf[1] = ((uint8_t*)&data)[1];
	buf[2] = ((uint8_t*)&data)[0];
	
	if( i2c_tx(buf, 3, 0)==0 )
		return(0);
	if( i2c_rx(buf,3, 1)==0 )
		return(0);
	//Verify
	if( (buf[0]==addr) && (buf[1]==((uint8_t*)&data)[1]) && (buf[2]==((uint8_t*)&data)[0]) )
		return(1);
	else
		return(0);
}
//-------------------------------------------------------------------------------------------
int8_t max582x_read(uint8_t addr, uint16_t *data)
{
	if( i2c_tx(&addr, 1, 0)==0 )
		return(0);
	if( i2c_rx_word(data, 1)==0 )
		return(0);
	return 1;
}
//--------------------------------------------------------------------------------------------
int8_t max582x_init_ref(){
	if( max582x_write( REF|REF_PWR_ALWEYS|REF_4_1V, 0 )==0 )
		return(0);
	
	if( max582x_write( SW_CLEAR, DATA_FILL )==0 )
		return(0);
	
	return(1);
}

int8_t max582x_init()
{
	if( max582x_write(REF|REF_EX, 0)==0 )
		return(0);
	
	if( max582x_write(SW_CLEAR, DATA_FILL)==0 )
		return(0);
	
	return(1);
}
//---------------------------------------------------------------------------------
//Load DAC data
//dac - channet 0-7
int8_t max582x_code_load(uint8_t dac, uint16_t data)
{
	if( max582x_write( CODE_LOAD | dac, data<<DAC_DATA_OFFS )==0 )
		return(0);
	return(1);
}
//---------------------------------------------------------------------------------
//Read DAC data
int8_t max582x_read_code(uint8_t dac, uint16_t *data)
{
	if( max582x_read( READ_CODE | dac, data) )
	{
		*data = *data>>DAC_DATA_OFFS;
		return(1);
	}
	return(0);		
}
