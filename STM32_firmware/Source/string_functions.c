#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "string_functions.h"

uint8_t to_lower_case(char *str, char termchar, uint8_t maxlen)
{
	int i=0;
	
	while( (str[i]!=termchar)&&(i < maxlen) ){
		if( str[i]<='Z' && str[i]>='A')
			str[i] = str[i] -'A'+'a';
		i++;
	}
	return(i);
}

//Checks if str is undigned integer
uint8_t isuint(char *str)
{
	str = rm_spaces(str);
	while( *str != '\0' && isspace( *str )==0 )
	{
		if( !(*str>='0' && *str<='9') )
			return 0;
		str++;			
	}
	str = rm_spaces(str);
	if(*str != '\0')
		return(0);
	return(1);
}
//Checks if str is float
uint8_t isfloat(char *str)
{
	uint8_t point=0;//Decimal point flag
	
	str = rm_spaces(str);
	
	if( !((*str>='0' && *str<='9')||*str=='-'||*str=='+') )
		return(0);
	str++;
	
	while( !(*str=='\0'||*str=='e'||*str=='E'||isspace(*str)) )
	{
		if(*str=='.')
		{
			if(point)
			{
				return(0);
			}else{
				point=1;
			}
		}else	if( !(*str>='0' && *str<='9') )
		{
			return 0;
		}
		str++;			
	}
	
	if(*str=='e'||*str=='E')
	{
		str++;
		if( *str=='-'||*str=='+' )
			str++;
		if(isuint(str))
			return(1);
		return(0);
	}
	
	str = rm_spaces(str);
	if(*str != '\0')
		return(0);
	return(1);	
}
//Remove leading spaces
char* rm_spaces(char* str)
{
	while( isspace( *str ) && *str!='\0')
	{
			str++;
	}
	return(str);
}
