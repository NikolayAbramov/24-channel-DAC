#include "clock-arch.h"
#include "stm32f0xx.h"

/*---------------------------------------------------------------------------*/
clock_time_t clock_time(void)
{
	uint32_t time;
	time = (uint32_t)SysTick->VAL/(SystemCoreClock/1000);
  return time;
}
/*---------------------------------------------------------------------------*/
