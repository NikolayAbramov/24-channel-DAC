//ENC28J60 Wiring
#define ENC28J60_PORT_SPI   GPIOB
#define ENC28J60_PORT_CS   GPIOA
#define ENC28J60_CS 11
#define ENC28J60_SO 4
#define ENC28J60_SI 5
#define ENC28J60_SCK 3

//set CS to 0 = active
#define ENC28J60_CS_ACTIVE ENC28J60_PORT_CS->BSRR|=1<<(ENC28J60_CS+16)
//set CS to 1 = passive
#define ENC28J60_CS_PASSIVE ENC28J60_PORT_CS->BSRR|=1<<ENC28J60_CS

//IP & MAC reset button
#define IP_RESET_BUTTON (GPIOA->IDR & (uint32_t)0x1)

void hardware_init(void);
