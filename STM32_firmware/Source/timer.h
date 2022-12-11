#include "inttypes.h"

//Reload value for 1 ms period
#define SYSTICK_RVR_VAL 8000

void delay(uint32_t);
void delay_long(uint32_t);

struct timer {
  uint32_t start;
  uint32_t interval;
	uint8_t run;
};

void timer_set(struct timer *t, uint32_t interval);
void timer_restart(struct timer *t);
uint32_t timer_expired(struct timer *t);
