#include "stm32f0xx.h"
#include "flash.h"
#include "crc/crc.h"

flash_data_TypeDef *flash_data = (flash_data_TypeDef*)FLASH_DATA_BASE;

void wait_for_ready(void);
void wait_for_eop(void);

void watit_for_ready(void) {
	while(FLASH->SR & FLASH_SR_BSY);
	if (FLASH->SR & FLASH_SR_EOP) {
		FLASH->SR |= FLASH_SR_EOP;
	}
}

void wait_for_eop(){
	while (!(FLASH->SR & FLASH_SR_EOP));
	FLASH->SR = FLASH_SR_EOP;
}

void flash_unlock(void) {
	FLASH->KEYR = FLASH_KEY1;
	FLASH->KEYR = FLASH_KEY2;	
}

void flash_lock() {
	FLASH->CR |= FLASH_CR_LOCK;
}

void flash_erase_page(uint32_t address) {
	watit_for_ready();
	FLASH->CR|= FLASH_CR_PER; 
	FLASH->AR = address; 
	FLASH->CR|= FLASH_CR_STRT; 
	wait_for_eop();
	FLASH->CR&= ~FLASH_CR_PER; 
}

//Write len - number of bytes. Must be even
void flash_write(uint32_t address, uint16_t *data, uint32_t len) 
{
	uint8_t i;
	
	flash_unlock();
	
	flash_erase_page(address);
	
	watit_for_ready();
	
	FLASH->CR |= FLASH_CR_PG;
	len>>=1;
	
	for(i = 0; i<len; i++ ){
		*(__IO uint16_t*)address = data[i];
		wait_for_eop();
		address+=2;
	}

	FLASH->CR &= ~(FLASH_CR_PG);
	
	flash_lock();
}
