#include "uip.h"
#include "uiplib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/*-----------------------------------------------------------------------------------
unsigned char uiplib_ipaddrconv(char *addrstr, unsigned char *ipaddr)
{
  unsigned char tmp;
  char c;
  unsigned char i, j;

  tmp = 0;
  
  for(i = 0; i < 4; ++i) {
    j = 0;
    do {
      c = *addrstr;
      ++j;
      if(j > 4) {
	return 0;
      }
      if(c == '.' || c == 0) {
	*ipaddr = tmp;
	++ipaddr;
	tmp = 0;
      } else if(c >= '0' && c <= '9') {
	tmp = (tmp * 10) + (c - '0');
      } else {
	return 0;
      }
      ++addrstr;
    } while(c != '.' && c != 0);
  }
  return 1;
}*/
//----------------------------------------------------------------------------
//Convert a single hex digit character to its integer value
unsigned char h2int(char c)
{
        if (c >= '0' && c <='9'){
                return((unsigned char)c - '0');
        }
        if (c >= 'a' && c <='f'){
                return((unsigned char)c - 'a' + 10);
        }
        if (c >= 'A' && c <='F'){
                return((unsigned char)c - 'A' + 10);
        }
        return(0);
}
//Converts a hex string to integer
uint16_t htoi(char *str)
{
		uint8_t i, len;
		uint16_t n=0, n1=1;
		
		len = strlen(str);
		for(i = 0; i<len; i++)
		{				
			n = n + (uint16_t)h2int(str[len-i-1])*n1;
			n1 = n1*16;
		}
		return(n);		
}
//-------------------------------------------------------------------------------------------------------------
//Universal parser string data with delimiters
//mode = 2|10|16
uint8_t parse_list(char *str, char delimiter, uint8_t *buf, int8_t mode, uint8_t nbytes)
{
	uint8_t i=0;//digit counter
	uint8_t j=0;//byte counter	
	int16_t n;
	char strbuf[4];
	char *str_begin;
	str_begin = str;
	//validate
	while(1){
		if( ((*str=='0' || *str=='1')&&(mode==2)) || (( *str>='0' && *str<='9' )&&(mode!=2)) || ( ( ( *str>='A' && *str<='F' )||( *str>='a' && *str<='f' ) ) && (mode==16) ) ){
			i++;
			if( ( i>1 && mode==2 ) || (i>3) || (i>2 && mode==16) )
				return(0);
			str++;
			continue;
		}			
		if( *str == delimiter ){
			j++;
			if(j >=nbytes || i==0 )
				return(0);	
			str++;
			i=0;	
			continue;	
		}
		if( (*str == '\0')||(*str == '\n') ){
			if(i==0)
				return(0);
			j++;
			if(j==nbytes)
				break;
		}			
		return(0);	
	}
	//Than parse
	str = str_begin;
	i=0;
	j=0;						
	while(1){
		if( (*str == delimiter) || (*str=='\0') || (*str == '\n') ){
			strbuf[i] = '\0';
			if(mode==16){
				n = htoi(strbuf);
			}else{
				n = (uint16_t)atoi(strbuf);
				if( n>255 )
					return(0);
			}
			buf[j] = (uint8_t)n;
			if(*str == delimiter){
				j++;
				i=0;
				str++;
				continue;
			}
			break;									
		}
		strbuf[i] = *str;
		i++;
		str++;
	}
	return(1);	
}
//-------------------------------------------------------------------------------------------------------
/*uint16_t fill(uint16_t len, char* string)
{
	uint16_t size;
	size = strlen(string);
	memcpy( ((uint8_t*)uip_appdata) + len, string, size);
	len+=size;
	return(len);	
}*/
//-----------------------------------------------------------------------------------
void fill_fmt(char* fmt, ...)
{
	va_list args;
  va_start(args, fmt);
	uip_slen+= vsprintf((char*)uip_appdata + uip_slen, fmt, args );
}
/*-----------------------------------------------------------------------------------*/
uint8_t url_strcmp(const char *str, uint8_t **buf)
{
	uint8_t len;
	len = strlen(str);
	if( !strncmp(str, (char*)(*buf), len) )
	{
		(*buf)+=len;
		return(1);
	}
	return(0);
}
//------------------------------------------------------------------------------------
// search for a string of the form key=value in
// a string that looks like q?xyz=abc&uvw=defgh HTTP/1.1\r\n
//
// The returned value is stored in strbuf. You must allocate
// enough storage for strbuf, maxlen is the size of strbuf.
// I.e the value it is declared with: strbuf[5]-> maxlen=5
uint8_t find_key_value(char *str, char *strbuf, uint8_t maxlen, char *key)
{
        uint8_t found=0;
        uint8_t i=0;
        char *kp;
        kp=key;
        while(*str &&  *str!=' ' && *str!='\r' && found==0){
                if (*str == *kp){
                        // At the beginning of the key we must check
                        // if this is the start of the key otherwise we will
                        // match on 'foobar' when only looking for 'bar', by andras tucsni
                        if (kp==key &&  ! ( *(str-1) == '?' || *(str-1) == '&' ) ) goto NEXT;
                        kp++;
                        if (*kp == '\0'){
                                str++;
                                kp=key;
                                if (*str == '='){
                                        found=1;
                                }
                        }
                }else{
                        kp=key;
                }
NEXT:
                str++;
        }
        if (found==1){
                // copy the value to a buffer and terminate it with '\0'
                while(*str &&  *str!=' ' && *str!='\r' && *str!='&' && i<maxlen-1){
                        *strbuf=*str;
                        i++;
                        str++;
                        strbuf++;
                }
                *strbuf='\0';
        }
        // return 1 if found (the string in strbuf might still be an empty string)
        // otherwise return 0 
        return(1);
}
//Get number of connections with a specific flag
uint8_t get_num_of_conns(uint8_t tcpstateflag)
{
	uint8_t i, n=0;	
	for(i = 0; i < UIP_CONNS; i++) {
		if(uip_conns[i].tcpstateflags == tcpstateflag)
			n++;
	}
	return n;
}
