#include "MAX582x.h"
#include "Source/MDAC_conf.h"
#include "dac_functions.h"

int8_t dac_read_all(void)
{
	uint8_t i, chip, err=0;	
	
	for(i=0; i<NCHAN; i++)
		{
			chip = i/DAC_NCHAN;
			max582_set_addr(DAC_addr[chip]);
			if(!max582x_read_code(i-chip*DAC_NCHAN, &DAC_buffer[i]))
				err = 1;
		}
	if(err)	
		return(0);
	else
		return(1);
}

int8_t dac_load_all(void)
{
	uint8_t i, chip, err = 0;	
	
	for(i=0; i<NCHAN; i++)
	{
		chip = i/DAC_NCHAN;
		max582_set_addr(DAC_addr[chip]);
		
		if(!max582x_code_load(i-chip*DAC_NCHAN, DAC_buffer[i]))
			err = 1;
	}
	if(err)	
		return(0);
	else
		return(1);
}

uint16_t dac_volt_to_data(float volt)
{
	int32_t data;
	if (volt>=0)
		data = (int32_t)(volt/DAC_RES+0.5)+DAC_MID;
	else
		data = (int32_t)(volt/DAC_RES-0.5)+DAC_MID;
	
	if(data<0)
		data = 0;
	if(data>DAC_MAX)
		data = DAC_MAX;
	
	return( (uint16_t)data );
}
