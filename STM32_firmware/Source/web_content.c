#include "web_content.h"
#include "string_functions.h"
#include "MAX582x/MAX582x.h"
#include "MDAC_conf.h"
#include "MAX582x/dac_functions.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "uip.h"
#include "uip_arp.h"
#include "uiplib.h"

//Number of rows and columns of DAC data table
#define NROW 8
#define NCOL 3 // NCHAN/NROW


const char* www_pages[4] = {"/","/ip_mac","/favicon.ico","/connections"};

//uint16_t moved_perm_begin(uint8_t *);
//uint16_t moved_perm_end(uint8_t *, uint16_t);
//char *HTTP_10= "HTTP/1.0 ";
//char *Content_Type = "\r\nContent-Type: ";
//char *HTTP200OK = "200 OK"; 

void page_header(void);
void print_dac_data(void);
//Channel being editing on mainpage
uint8_t ch_edit=0;

//--------------------------------------------------------------------
void http200ok()
{
	fill("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n");
}
//--------------------------------------------------------------------
void print_favico()
{
	fill("HTTP/1.0 200 OK\r\nContent-Type: image/x-icon\r\n\r\n");
}
//---------------------------------------------------------------------
/*uint16_t moved_perm_begin(uint8_t *buf)
{
	return fill(buf,0,"HTTP/1.0 301 Moved Permanently\r\nLocation: ");
}

uint16_t moved_perm_end(uint8_t *buf, uint16_t plen)
{
	return fill(buf,plen,"\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"
		"301 Moved Permanently</h1>\n");
}*/

void moved_perm(const char *page)
{
	fill_fmt(
	"HTTP/1.0 301 Moved Permanently\r\n"
	"Location: http://%u.%u.%u.%u%s\r\n"
	"Content-Type: text/html\r\nPragma: no-cache\r\n\r\n"
	"301 Moved Permanently</h1>\n",
	my_net_addr.ip[0],
	my_net_addr.ip[1],
	my_net_addr.ip[2],
	my_net_addr.ip[3],
	page);
}
//----------------------------------------------------------------------
void page_header()
{	
	fill("<h2>Multichannel DAC</h2>");
}
//--------------------------------------------------------------------------
void print_dac_data()
{
	uint8_t i,j,n_ch;
	
	fill("<table border=1 cellpadding=1>");
	
	for(i=0; i<NCOL; i++)
	{
		fill("<col width=40><col width=70>");
	}
	
	for(i=0; i<NROW; i++)
	{	
		j=0;
		n_ch = i;
		fill("<tr>");
		while( (j <= NCOL) && n_ch<NCHAN  )
		{
			fill_fmt("<th>%u</th><td>%.3f</td>", n_ch, VOLT(n_ch));
			n_ch+=NROW;
			j++;
		}
		fill("</tr>\n ");
	}
	fill("</table>" );
}
//-----------------------------------------------------------------------------------
void print_mainpage()
{
    http200ok();
		fill("<!DOCTYPE html><style>form{display: inline-block;}</style>\n");
		page_header();
    fill("\n<pre>");
		//Table
		print_dac_data();
	
		fill_fmt("\n<form action=/ method=get>"
			"<button value=\"Reset\" name=\"Reset\">Reset</button>\n\n"
			"Channel:<input type=text size=2 name=ch value=%u>"
			"\tVoltage:<input type=text size=6 name=volt value=%.3f>V"
			"  <input type=submit value=\"Submit\"></form>", ch_edit, VOLT(ch_edit));
	
		if(global_flags & DAC_FAULT){
			fill("<p><font color=\"red\"> DAC fault</font></p>");
		}else if(global_flags & DAC_ERROR){
			fill("<p><font color=\"red\"> DAC error</font></p>");
		}else{
			fill("<p> </p>");
		}
		
		fill("<hr><a href=\"./ip_mac\">[IP and MAC]</a>\t<a href=\"./connections\">[Connections]</a>\t<a href=\".\">[Refresh]</a>\n"
			"</pre><hr>Nikolay Abramov, 2017. Based on uIP TCP/IP stack developed by Adam Dunkels\n");
			//"<meta http-equiv=\"refresh\" content=\"3\">");
		//fill_fmt("%u", uip_slen);
}
//-----------------------------------------------------------
int8_t  handle_mainpage_action(char *str)
{
		char strbuf[10];
		uint8_t ch, chip;
		uint16_t data;
		float volt;
		
		if(!strncmp(str, "Reset", 5)){
			for(ch=0;ch<NCHAN;ch++){
				DAC_buffer[ch] = DAC_MID;
			}
			if(dac_load_all()){
				global_flags &= ~DAC_ERROR;
				return 1;
			}else{
				global_flags |= DAC_ERROR;
				return 0;
			}
		}
		
    if ( !find_key_value(str, strbuf, 10, "ch") )
			return(0);
			
		if(isuint(strbuf))
			ch = atoi(strbuf);
		else
			return 0;
			
		if(ch>=NCHAN)
			return 0;
		
		ch_edit = ch;
			
		if(!find_key_value(str, strbuf, 10, "volt"))
			return(0);
		
		if(isfloat(strbuf)){
			volt = strtof(strbuf, NULL);
			chip = ch/DAC_NCHAN;
			data = dac_volt_to_data(volt);
			
			max582_set_addr(DAC_addr[chip]);
			if(! max582x_code_load( ch-chip*DAC_NCHAN, data) )
			{
				global_flags |= DAC_ERROR;
				return(0);
			}
			global_flags &= ~DAC_ERROR;
			DAC_buffer[ch]=data;
		}
		else
		{
			return 0;
		}
		return (1);
}
//--------------------------------------------------------------------------------
void print_ip_mac_page()
{	
  http200ok();	
	page_header();
	fill_fmt(
	"\n<h3>Network settings</h3><pre>\n"
	"<form action=/ip_mac method=get>"
	"IP:\t<input type=text size=20 name=ip value=\"%u.%u.%u.%u\">"
	"<br>\n"
	"MAC:\t<input type=text size=20 name=mac value=\"%x.%x.%x.%x.%x.%x\">"
	"<br>\n"
	"<input type=submit value=\"Apply\"></form>"
	"<hr><a href=\".\">[Home]</a>\t<a href=\"./ip_mac\">[Refresh]</a>", 
	((uint8_t*)(&uip_hostaddr[0]))[0],
	((uint8_t*)(&uip_hostaddr[0]))[1],
	((uint8_t*)(&uip_hostaddr[1]))[0],
	((uint8_t*)(&uip_hostaddr[1]))[1],
	uip_ethaddr.addr[0],
	uip_ethaddr.addr[1],
	uip_ethaddr.addr[2],
	uip_ethaddr.addr[3],
	uip_ethaddr.addr[4],
	uip_ethaddr.addr[5]);
}

void edit_ip_mac(char *str, struct net_interface_state *s)
{
	char strbuf[18];
	
	if( find_key_value(str, strbuf, 18, "ip") ){
		if( !parse_list(strbuf, '.', my_net_addr.ip, 10, 4)){
			return;
		}
	}else{
		return;
	}
	
	if( find_key_value(str, strbuf, 18, "mac") ){
		if( !parse_list(strbuf, '.', my_net_addr.mac, 16, 6) ){
			return;
		}
	}else{
		return;
	}
	//If ip or mac changed
	if( memcmp(my_net_addr.ip, uip_hostaddr, 4) ){
			s->flag |= WWW_CHANGE_IP;
	}
	if( memcmp(my_net_addr.mac, uip_ethaddr.addr, 6) ){
		s->flag |= WWW_CHANGE_MAC;
	}
	if( (s->flag & WWW_CHANGE_IP) || (s->flag & WWW_CHANGE_MAC) ){	
		//Save to flash
		save_net_addr();
	}
	//ip_mac_return:
	//moved_perm( (char*)www_ip_mac );
}
//----------------------------------------------------------------------------
void print_connections()
{
	uint8_t i;
	struct arp_entry *tabptr;
	
	http200ok();
	fill_fmt("arptime: %u<br>\n", arptime);  
	fill("Connections:<br>\n");
	for(i = 0; i < UIP_CONNS; i++) {
		fill_fmt("%u.%u.%u.%u\t%u<br>\n", 
		((uint8_t*) (uip_conns[i].ripaddr))[0],
		((uint8_t*) (uip_conns[i].ripaddr))[1],
		((uint8_t*) (uip_conns[i].ripaddr))[2],
		((uint8_t*) (uip_conns[i].ripaddr))[3],
		uip_conns[i].tcpstateflags);
	}
	fill("ARP table:<br>\n");
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    tabptr = &arp_table[i];
		fill_fmt("%u.%u.%u.%u\t%u.%u.%u.%u.%u.%u\t%u<br>\n",
    ((uint8_t*) (tabptr->ipaddr))[0],
		((uint8_t*) (tabptr->ipaddr))[1],
		((uint8_t*) (tabptr->ipaddr))[2],
		((uint8_t*) (tabptr->ipaddr))[3],
		tabptr->ethaddr.addr[0],
		tabptr->ethaddr.addr[1],
		tabptr->ethaddr.addr[2],
		tabptr->ethaddr.addr[3],
		tabptr->ethaddr.addr[4],
		tabptr->ethaddr.addr[5],
		tabptr->time);	
	}
	fill("<input type=submit value=\"Apply\"></form>"
	"<hr><a href=\".\">[Home]</a>\t<a href=\"./connections\">[Refresh]</a>");
	fill_fmt("%u",uip_slen);
}

//Make www server response
void www_ans(struct net_interface_state *s){
	switch(s->data_type){
		case WWW_MAIN:
			if(s->flag & WWW_REDIRECT){
				moved_perm(www_pages[WWW_MAIN]);
			}else{
				print_mainpage();
			}
			break;
		case WWW_IP_MAC:
			if(s->flag & WWW_REDIRECT){
				moved_perm(www_pages[WWW_IP_MAC]);
			}else{
				print_ip_mac_page();
			}
			break;
		case WWW_CONN:
			print_connections();
			break;	
		case WWW_FAVICON:
			if(s->flag & WWW_REDIRECT){
				moved_perm(www_pages[WWW_FAVICON]);
			}else{
				print_favico();
			}
			break;
		default:
			if(s->flag & WWW_REDIRECT){
				moved_perm(www_pages[WWW_MAIN]);
			}else{
				print_mainpage();
			}
	}
}
