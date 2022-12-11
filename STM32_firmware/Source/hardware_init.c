#include "stm32f0xx.h"
#include "timer.h"
#include "hardware_init.h"

void hardware_init()
{
	//Enable clock for ports A and B
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	//-------------------------------------------------------------------------------------------------
	//Output
	GPIOA->MODER |= GPIO_MODER_MODER3_0;
	GPIOA->MODER &= ~GPIO_MODER_MODER3_1;
	//IP reset button pullup
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR0_0;
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR0_1;
	
	//Push-pull
	GPIOA->OTYPER &= ~GPIO_OTYPER_OT_3;
	//Low speed
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDR_OSPEEDR3_0;
	//Enable clock for TIM14
	RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
	
	//ENC28J60 SPI
	//Alternative func mode for SPI pins SCK and MOSI and MISO
	ENC28J60_PORT_SPI->MODER |= ( 1<<(ENC28J60_SO*2+1) | 1<<(ENC28J60_SI*2+1) | 1<<(ENC28J60_SCK*2+1) );
	ENC28J60_PORT_SPI->MODER &= ~( 1<<ENC28J60_SO*2 | 1<<ENC28J60_SI*2 | 1<<ENC28J60_SCK*2 );
	//CS pin
	ENC28J60_PORT_CS->MODER |= ( 1<<ENC28J60_CS*2 );
	ENC28J60_PORT_CS->MODER &= ~( 1<<(ENC28J60_CS*2+1) );
	//MISO input with pullup
	//ENC28J60_PORT->PUPDR |= 1<<(ENC28J60_SO*2);
	//ENC28J60_PORT->PUPDR &= ~ ( 1<<(ENC28J60_SO*2+1) );
	//Push pull mode for SPI pins SCK , MOSI and CS
	ENC28J60_PORT_SPI->OTYPER &= ~( 1<<ENC28J60_SI | 1<<ENC28J60_SCK);
	ENC28J60_PORT_CS->OTYPER &= ~(1<<ENC28J60_CS);
	
	//CRC
	RCC->AHBENR |= RCC_AHBENR_CRCEN;
	
	//Timer
	TIM14->PSC = 8000;
	TIM14->ARR = 1000;
	TIM14->DIER |= TIM_DIER_UIE;
	//-------------------------------------------------------------------------------------------------
	//Run SysTick
	SysTick_Config( SYSTICK_RVR_VAL );
	//SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	
	//-------------------------------------------------------------------------------------------------
	//Interrupts
	NVIC_EnableIRQ(TIM14_IRQn);
	NVIC_EnableIRQ(SysTick_IRQn);
	TIM14->CR1 |= TIM_CR1_CEN;
}
