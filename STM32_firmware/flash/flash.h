#include "stm32f0xx.h"
#include "Source/net_interface.h"

#define FLASH_DATA_BASE ((uint32_t)(FLASH_BANK1_END-0x3FFU))

void flash_unlock(void);

void flash_lock(void);

void flash_erase_page(uint32_t);

void flash_write(uint32_t, uint16_t *, uint32_t);

//User data to store
/*
typedef struct {
	uint8_t mac[6];
	uint8_t ip[4];
} flash_user_data_TypeDef;
*/
typedef net_addr_TypeDef flash_user_data_TypeDef;
//Must contain even number of bytes
typedef struct {
	flash_user_data_TypeDef user_data;
	uint32_t checksum;
} flash_data_TypeDef;

extern flash_data_TypeDef *flash_data;
