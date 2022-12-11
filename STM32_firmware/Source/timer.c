#include "stm32f0xx.h"
#include "timer.h"

uint32_t ms_timer=0;
//Sofrware overflow flag
uint32_t ovf=0;

//Short stupid waiting
void delay(uint32_t ticks)
{
	uint32_t start_time;

	start_time = (uint32_t)SysTick->VAL;
	while( 1 )
	{
		if(ovf)
		{
			ovf = 0;
			//if overflow
			if( start_time <= ticks )
				ticks -= start_time;
			else
				break;
			start_time = SYSTICK_RVR_VAL;
		}
			
		if( (start_time - (uint32_t)SysTick->VAL) >= ticks )
			break;
	}
}
//Long stupid waiting
void delay_long(uint32_t ms_delay)
{
	uint32_t start_time;
	
	start_time = ms_timer;
	
	while( (ms_timer-start_time) < ms_delay);
		
}
//ms timer
void timer_set(struct timer *t, uint32_t interval)
{
  t->interval = interval;
  t->start = ms_timer;
	t->run = 1;
}

void timer_restart(struct timer *t)
{
  t->start = ms_timer;
}

uint32_t timer_expired(struct timer *t)
{
  return ((ms_timer - t->start) >= t->interval) && (t->run);
}

//SysTick IRQ
void SysTick_Handler(){
	ms_timer +=1;
	//SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
}
