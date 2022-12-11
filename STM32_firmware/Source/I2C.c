#include "stm32f0xx.h"
#include "I2C.h"

#define I2C_USED I2C1
#define I2C_PORT GPIOB
#define I2C_AF 1UL //Port alternate function id for I2C
#define I2C_SCL_PIN 6
#define I2C_SDA_PIN 7
#define I2C_CLOCK_EN ( RCC->APB1ENR |= RCC_APB1ENR_I2C1EN )

#define TIMIGR_PRESC 28
#define TIMIGR_SCLDEL 20
#define TIMIGR_SDADEL 16
#define TIMIGR_SCLH 8
#define TIMIGR_SCLL 0

#define I2C_CR2_NBYTES_OFFS 16

void i2c_set_numbytes(uint8_t);

void i2c_init()
{
//-------------------------------------------------------------------------------------------------
	//I2C1 init
	//Open drain, pull up
	I2C_PORT->OTYPER |= (1<<I2C_SDA_PIN | 1<<I2C_SCL_PIN );
	I2C_PORT->PUPDR |=  (1<<I2C_SDA_PIN*2 | 1<<I2C_SCL_PIN*2 );
	I2C_PORT->PUPDR &= ~( 1<<(I2C_SDA_PIN*2+1) | 1<<(I2C_SCL_PIN*2+1) );
	//Alternate function for GPIO pins
	I2C_PORT->AFR[(I2C_SCL_PIN/8) | (I2C_SDA_PIN/8)] |= (I2C_AF<<I2C_SCL_PIN*4 | I2C_AF<<I2C_SDA_PIN*4);
	I2C_PORT->MODER |= ( 1<<(I2C_SDA_PIN*2+1) | 1<<(I2C_SCL_PIN*2+1) );
	I2C_PORT->MODER &= ~( 1<<I2C_SDA_PIN*2 | 1<<I2C_SCL_PIN*2 );
	
	//Enable clock
	I2C_CLOCK_EN;
	//I2C timing crap for 100 kHz
	I2C_USED->TIMINGR |= 1<<TIMIGR_PRESC | 0x13<<TIMIGR_SCLL | 0xF<<TIMIGR_SCLH 
								| 0x2<<TIMIGR_SDADEL | 0x4<<TIMIGR_SCLDEL;
	//Enable I2C
	I2C_USED->CR1 |= I2C_CR1_PE;
	
	//Autoend
	//I2C_USED->CR2 |= I2C_CR2_AUTOEND;
}

void i2c_set_slave_addr(uint8_t addr)
{
	//Slave address
	I2C_USED->CR2 &= ~I2C_CR2_SADD;
	I2C_USED->CR2 |= (uint32_t)addr;
}

void i2c_set_numbytes(uint8_t numbytes)
{
	//Number of bytes
	I2C_USED->CR2 &= ~I2C_CR2_NBYTES;
	I2C_USED->CR2 |= ((uint32_t)numbytes) << I2C_CR2_NBYTES_OFFS;
}
//Transmit data
//stop - send stop
int8_t i2c_tx(uint8_t *data, uint8_t len, uint8_t stop)
{
	i2c_set_numbytes(len);
	//Write mode
	I2C_USED->CR2 &= ~I2C_CR2_RD_WRN;
	//Start communication
	I2C_USED->ICR |= I2C_ICR_NACKCF;
	I2C_USED->CR2 |= I2C_CR2_START;
	
	while(len)
  {
		while((I2C_USED->ISR & I2C_ISR_TXIS) == 0)
		{
			if(I2C_USED->ISR & I2C_ISR_NACKF)
				return(0);// Communication fault
		}
		*(uint8_t *)&(I2C1->TXDR) = *data;
    len--;
    data++;
  }
	while((I2C_USED->ISR & I2C_ISR_TC)==0)
		{
			if(I2C_USED->ISR & I2C_ISR_NACKF)
				return(0);// Communication fault
		}
	if(stop)
	{
		I2C_USED->CR2 |= I2C_CR2_STOP;
	}
	return(1);
}
//Receive data
int8_t i2c_rx(uint8_t *data, uint8_t len, uint8_t stop)
{
	i2c_set_numbytes(len);
	//Read mode
	I2C_USED->CR2 |= I2C_CR2_RD_WRN; 
	//Start communication
	I2C_USED->ICR |= I2C_ICR_NACKCF;
	I2C_USED->CR2 |= I2C_CR2_START;
	
	if(I2C_USED->ISR & I2C_ISR_NACKF)
			return(0);// Communication fault
	
	while(len)
	{
		while((I2C_USED->ISR & I2C_ISR_RXNE)==0);
		*data = *(uint8_t *)&(I2C1->RXDR);
		data++;
		len--;
	}
	while((I2C_USED->ISR & I2C_ISR_TC)==0);
	if(stop)
	{
		I2C_USED->CR2 |= I2C_CR2_STOP;
	}
	return(1);
}

int8_t i2c_rx_word(uint16_t *data, uint8_t stop)
{
	i2c_set_numbytes(2);
	//Read mode
	I2C_USED->CR2 |= I2C_CR2_RD_WRN; 
	//Start communication
	I2C_USED->ICR |= I2C_ICR_NACKCF;
	I2C_USED->CR2 |= I2C_CR2_START;
	
	if(I2C_USED->ISR & I2C_ISR_NACKF)
			return(0);// Communication fault
	
	while( (I2C_USED->ISR & I2C_ISR_RXNE) == 0);
	((uint8_t*)data)[1] = *(uint8_t *)&(I2C1->RXDR);
	while( (I2C_USED->ISR & I2C_ISR_RXNE) == 0);
	((uint8_t*)data)[0] = *(uint8_t *)&(I2C1->RXDR);

	while((I2C_USED->ISR & I2C_ISR_TC)==0);
	if(stop)
	{
		I2C_USED->CR2 |= I2C_CR2_STOP;
	}
	return(1);
}
