#include "scpi.h"
#include "string_functions.h"
#include "MAX582x/MAX582x.h"
#include "MDAC_conf.h"
#include "MAX582x/dac_functions.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "uiplib.h"

//Data format
uint8_t format;
#define ASCII 0
#define BIN 	1

//Command buffer size
#define COMMAND_BUF_SIZE 8

//----------------------------------------------------------
void handle_idn(char *buf, uint32_t *data_type)
{
	switch( isquery(buf) ){
		case 1:
			*data_type = SCPI_IDN;
			break;
		case 2:
			*data_type = SCPI_FALSE;
			break;
	}
}
//------------------------------------------------------------

void scpi_parse_volt(char *buf, uint8_t *ch, uint32_t *data_type)
{
	char* start;
	uint8_t chan, query=0, chip;
	uint16_t data;
	float volt;
	
	buf = rm_spaces(buf);
	
	start = buf;
	
	while(1){
		if(*buf=='\0')
			goto parse_volt_false;
		if(*buf == '?' || *buf==','){
			switch(isquery(buf)){
				case 0:
					break;
				case 1:
					query=1;
					break;
				case 2:
					goto parse_volt_false;
			}
			*buf='\0';
			break;
		}
		buf++;
	}
	
	if(isuint(start)){
		chan = atoi(start);
		if( chan>NCHAN )
			goto parse_volt_false;
	}
	
	if(query){
		*ch = chan;
		*data_type = SCPI_VOLT;
		return;
	}
	
	buf++;
	
	if(isfloat(buf)){
		volt = atof(buf);
		chip = chan/DAC_NCHAN;
		data=dac_volt_to_data(volt);
		
		max582_set_addr(DAC_addr[chip]);
		if(! max582x_code_load( chan-chip*DAC_NCHAN, data) )
		{
			global_flags |= DAC_ERROR;
			goto parse_volt_false;
		}
		global_flags &= ~DAC_ERROR;
		DAC_buffer[chan]=data;
		*data_type=SCPI_TRUE;
		return;
	}
	parse_volt_false:
	*data_type = SCPI_FALSE;
	return;
}

void scpi_get_volt(uint8_t ch)
{
	fill_fmt("%.3f\n", VOLT(ch) );
}

//------------------------------------------------------------
void handle_data(char* buf, uint32_t *data_type)
{
	switch( isquery(buf) ){
		case 1:
			*data_type = SCPI_DATA;
			break;
		case 2:
			*data_type = SCPI_FALSE;
			break;
		case 0: 
			if( parse_data((uint8_t*)buf) )	
				*data_type = SCPI_TRUE;
			else
				*data_type = SCPI_FALSE;
	}
}	

int8_t parse_data(uint8_t *buf)
{
	if(format==ASCII){
		if( parse_float_list( (char*)buf ) )
			return(1);
		return(0);
	}
	return(0);
}

int8_t parse_float_list(char *buf)
{
	char* substr;
	float volt;
	uint8_t ch=0;
	uint16_t data;

	substr = strtok(buf, ",");
	while(substr!=NULL)
	{
		if(ch<NCHAN){
			if(isfloat(substr)){
				volt = atof(substr);
				data = dac_volt_to_data(volt);
				DAC_buffer[ch]=data;
				ch++;
			}
			substr = strtok (NULL, ",");
		}else{
			return(0);
		}
	}
	if(ch==NCHAN){
		if(dac_load_all())
			return(1);
		else
			return(0);
	}
	return(0);
}

void scpi_get_data()
{
	uint8_t i;
	
	dac_read_all();
	
	for(i=0;i<NCHAN;i++)
	{
		fill_fmt("%.3f,", VOLT(i) );
	}
	fill_fmt("\n");
}
//---------------------------------------------------------
void handle_format(char *buf, uint32_t *data_type)
{
	buf = rm_spaces(buf);
					
	switch( isquery(buf) ){
		case 1:
			*data_type = SCPI_FORMAT;
			break;
		case 2:
			*data_type = SCPI_FALSE;
			break;
		case 0: 
			if( parse_format(buf) )
				*data_type = SCPI_TRUE;
			else
				*data_type = SCPI_FALSE;
	}
}

int8_t parse_format(char *buf)
{
	buf = rm_spaces(buf);
	
	if( scpi_strcmp("ascii", &buf)){
		if(*rm_spaces(buf)=='\0'){
			format=ASCII;
			return(1);
		}
		return(0);
	}
	if( scpi_strcmp("bin", &buf)){
		if(*rm_spaces(buf)=='\0'){
			format=BIN;
			return(1);
		}
	}
	return(0);	
}

void scpi_get_format()
{
	switch(format){
		case ASCII:
			fill("ASCII\n");
			break;
		case BIN:
			fill("BIN\n");
			break;
	}
}
//--------------------------------------------------------
int8_t scpi_strcmp(const char *str, char **buf)
{
	uint8_t len;
	char combuf[COMMAND_BUF_SIZE]; 
	
	len = strlen(str);
	memcpy(combuf, *buf, len);
	
	to_lower_case(combuf,'\n', len);
	
	if( !strncmp(str, combuf, len) )
	{
		(*buf)+=len;
		return(1);
	}
	return(0);
}
//0 - no query
//1 - valid query
//2 - invalid query
int8_t isquery(char *str)
{
	str = rm_spaces(str);
	if(*str =='?'){
		if( *rm_spaces(str+1) == '\0')
			return(1);
		return(2);
	}
	return(0);
}

//-----------------------------------------
//Send TCP keep-alive packet with
void send_keepalive(){
	uip_slen = 1;
	*(uint8_t*)uip_appdata = 0;
}

void scpi_ans(uint32_t data_type, uint8_t chan)
{
	switch(data_type){
		case SCPI_IDN:
			fill("Multichannel DAC, s/n 1\n");
			break;
		case SCPI_TRUE:
			fill("True\n");
			break;
		case SCPI_FALSE:
			fill("False\n");
			break;
		case SCPI_VOLT:
			scpi_get_volt(chan);
			break;
		case SCPI_FORMAT:
			scpi_get_format();
			break;
		case SCPI_DATA:
			scpi_get_data();
			break;
		case SCPI_KEEPALIVE:
			send_keepalive();
		}
}
