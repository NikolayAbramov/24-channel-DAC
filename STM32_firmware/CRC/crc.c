#include "stm32f0xx.h"
#include "crc.h"

uint32_t crc_calc(uint8_t * buffer, uint32_t len) {
	uint32_t i, len32, rest;
	
	len32 = len>>2;
	CRC->CR |= CRC_CR_RESET;
	for(i = 0; i < len32; i++) {
		CRC->DR = ((uint32_t*)buffer)[i];
	}
	rest = len-(len32<<2);
	
	i = len-rest;
	switch (rest){
		case 1:
			CRC->DR = (uint32_t)buffer[i]<<24;
			break;
		case 2:
			CRC->DR = (uint32_t)(*(uint16_t*)(&buffer[i]))<<16;
			break;
		case 3:
			CRC->DR = ((uint32_t)(*(uint16_t*)(&buffer[i]))<<16)|((uint32_t)buffer[i+2]<<8);
	}
return (CRC->DR);
}
