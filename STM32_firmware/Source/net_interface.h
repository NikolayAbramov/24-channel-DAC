#ifndef __HELLO_WORLD_H__
#define __HELLO_WORLD_H__

/* Since this file will be included by uip.h, we cannot include uip.h
   here. But we might need to include uipopt.h if we need the u8_t and
   u16_t datatypes. */
#include "uipopt.h"

//Connection flags
#define WWW_REDIRECT 1
#define WWW_CHANGE_IP 2
#define WWW_CHANGE_MAC 4

//Keepalive packets for SCPI
#define KEEPALIVE_PERIOD 10
#define KEEPALIVE_TRYS 3

extern uint8_t default_mac[6];
extern uint8_t default_ip[4];

typedef struct{
uint8_t ip[4];
uint8_t mac[6];
} net_addr_TypeDef;

extern net_addr_TypeDef my_net_addr;

typedef struct net_interface_state {
	uint8_t timer;//Connection timer 
	uint8_t state;//Connection state
	uint32_t data_type;//Data type sent to client
	uint8_t data_att;//Data attribute (channel number, variable page content)
	uint8_t flag;//
} uip_tcp_appstate_t;

/* Finally we define the application function to be called by uIP. */
void net_interface_appcall(void);

void net_interface_init(void);

void save_net_addr(void);
void load_net_addr(void);
void set_default_net_addr(void);

#ifndef UIP_APPCALL
#define UIP_APPCALL net_interface_appcall
#endif /* UIP_APPCALL */

#endif /* __HELLO_WORLD_H__ */
