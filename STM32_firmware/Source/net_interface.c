#include "net_interface.h"
#include "web_content.h"
#include "uip.h"
#include "scpi.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "uiplib.h"
#include "uip_arp.h"
#include "string_functions.h"
#include "ENC28J60.h"
#include "flash/flash.h"

#include "crc/crc.h"

//Application states
#define JUST_CONN 0
#define DATA_SENT 1

uint8_t default_mac[6] = {0x00,0x43,0x18,0x78,0x01,0x36};
uint8_t default_ip[4]={10,20,61,12};

//Network ports
uint16_t www_port = HTONS(80);
uint16_t scpi_port = HTONS(1000);

uint8_t keepalive_timer = 0;

net_addr_TypeDef my_net_addr;

void handle_www(struct net_interface_state *);

void handle_scpi(struct net_interface_state *);

//uint16_t print_conns(char * , uint16_t );

void net_interface_appcall(void)
{
	struct net_interface_state *s = &(uip_conn->appstate);

  //If a new connection has just established
  if(uip_connected()) {
		s->state = JUST_CONN;
		s->timer = 0;
		s->flag = 0;
		keepalive_timer = 0;
		s->data_type = 0;
  }
	
	if (uip_conn->lport == scpi_port){
		handle_scpi(s);
	}else if(uip_conn->lport == www_port){
		handle_www(s);
	}
}
//-------------------------------------------------
void net_interface_init()
{
	uip_listen(www_port);
	uip_listen(scpi_port);
}
//------------------------------------------------
void handle_scpi(struct net_interface_state *s)
{
	char *cmd;
	
	//Periodically check if peer on scpi port stil alive
	if( uip_poll() ){
			if(keepalive_timer >= KEEPALIVE_PERIOD){
				keepalive_timer = 0;
				if(uip_conn->len == 0){
					//Decremetn sequence number
					if(--(uip_conn->snd_nxt[3]) == 0xFF) {
						if(--(uip_conn->snd_nxt[2]) == 0xFF) {
							if(--(uip_conn->snd_nxt[1]) == 0xFF) {
								--(uip_conn->snd_nxt[0]);
					}}}
					s->data_type = SCPI_KEEPALIVE;
					scpi_ans(s->data_type, s->data_att);
				}
			}else{
				keepalive_timer++;
			}
		return;
	}
	
	if( uip_acked() ) {
	}
	
	//Retransmission
	if(uip_rexmit()){
		scpi_ans(s->data_type, s->data_att);
		return;
	}
	
	//Command handling
	if(uip_len){
		keepalive_timer = 0;
		((char*)uip_appdata)[uip_len] = '\0';
		cmd = strtok((char*)uip_appdata, ";");
		s->data_type = SCPI_NODATA;
		
		while(cmd!=NULL){
			cmd = rm_spaces(cmd);
			
			if( scpi_strcmp("*idn", &cmd ) ){
				handle_idn(cmd, &s->data_type);
			}
			
			else if( scpi_strcmp("volt", &cmd) ){
				scpi_parse_volt(cmd, &s->data_att, &s->data_type);
			}
			
			else if( scpi_strcmp("format", &cmd) ){
				handle_format(cmd, &s->data_type);
			}
			
			else if( scpi_strcmp("data", &cmd) ){
				handle_data(cmd, &s->data_type);
			}
				
			cmd = strtok(NULL, ";");
		}
		scpi_ans(s->data_type, s->data_att);
	}
}

//-------------------------------------------------------
void handle_www(struct net_interface_state *s)
{
	uint8_t *p; //Data pointer
	
	p = uip_appdata;
	
	if( uip_closed() ) {
		if(s->flag & WWW_CHANGE_IP){
			global_flags |= CHANGE_IP;
			s->flag &= ~WWW_CHANGE_IP;
		}
		if(s->flag & WWW_CHANGE_MAC){
			global_flags |= CHANGE_MAC;
			s->flag &= ~WWW_CHANGE_MAC;
		}
		return;	
	}
	
	//Retransmission
	if(uip_rexmit()){
		www_ans(s);
		return;
	}
	
	switch(s->state){
		case JUST_CONN:
			s->flag = 0;
			if (uip_len>0){
				if( url_strcmp("GET ", &p) ){
					//IP & MAC page
					if( url_strcmp(www_pages[WWW_IP_MAC], &p) ){
						if(url_strcmp(" ", &p)){
							//print_ip_mac_page();
							s->data_type = WWW_IP_MAC;
						}
						else if(url_strcmp("?", &p)){
							s->data_type = WWW_IP_MAC;
							s->flag |= WWW_REDIRECT;
							edit_ip_mac((char*)p--, s);
						}
						goto www_reply;
					}
					//Connections and ip table
					if( url_strcmp(www_pages[WWW_CONN], &p) ){
							s->data_type = WWW_CONN;
							goto www_reply;
					}
					//stupid annoying favicon
					if( url_strcmp(www_pages[WWW_FAVICON], &p) ){
						//print_favico();
						s->data_type = WWW_FAVICON;
						goto www_reply;
					}
					//Main page
					if( url_strcmp(www_pages[WWW_MAIN], &p) ){
						if(url_strcmp(" ", &p)){
							s->data_type = WWW_MAIN;
							//print_mainpage();
							goto www_reply;
						}else if(url_strcmp("?", &p)){
							handle_mainpage_action((char*)p--);
							s->data_type = WWW_MAIN;
							s->flag |= WWW_REDIRECT;
							//print_mainpage();
							goto www_reply;
						}
					}
					s->flag |= WWW_REDIRECT;
					s->data_type = WWW_MAIN;
					//moved_perm((char*)s->data_type);
					www_reply:					
					//uip_send(uip_appdata, len);
					www_ans(s);
					s->state = DATA_SENT;
				}	
			}
			break;
		case DATA_SENT:
				uip_close();
		}
}

//Save ip and mac to flash
void save_net_addr()
{
	flash_data_TypeDef new_data;
	
	new_data = *flash_data;
	new_data.user_data = my_net_addr;		
	new_data.checksum = crc_calc( (uint8_t*)&new_data.user_data, sizeof(flash_user_data_TypeDef));
	flash_write((uint32_t)flash_data, (uint16_t*)&new_data, sizeof(flash_data_TypeDef) );
}
//Load ip and mac from flash
void load_net_addr(){
	if(flash_data->checksum == crc_calc( (uint8_t*)&(flash_data->user_data), sizeof(flash_user_data_TypeDef))){
		uip_sethostaddr(flash_data->user_data.ip);
		uip_setethaddr(flash_data->user_data.mac);
		my_net_addr = flash_data->user_data;
	}else{
		uip_sethostaddr(default_ip);
		uip_setethaddr(default_mac);
		memcpy( my_net_addr.ip, default_ip, 4);
		memcpy( my_net_addr.mac, default_mac,6);
	}
	enc28j60_mac = my_net_addr.mac;
	enc28j60Init();
}
//Set default IP and MAC
void set_default_net_addr(){
		memcpy( my_net_addr.ip, default_ip, 4);
		memcpy( my_net_addr.mac, default_mac,6);
		enc28J60WriteMAC(default_mac);
		uip_sethostaddr(default_ip);
		uip_setethaddr(default_mac);
}
