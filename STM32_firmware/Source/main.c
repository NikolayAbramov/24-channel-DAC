#include <string.h>

#include "stm32f0xx.h"
#include "hardware_init.h"
#include "I2C.h"
#include "enc28j60.h"
#include "MAX582x/MAX582x.h"
#include "timer.h"

#include "uip.h"
#include "uip_arp.h"
#include "uiplib.h"

#include "MDAC_conf.h"
#include "MAX582x/dac_functions.h"
#include "flash/flash.h"
#include "crc/crc.h"
#include "net_interface.h"

//TCP/IP buffer
#define BUF ( (struct uip_eth_hdr *) uip_buf )

//I2C slave addresses of ICs
uint8_t DAC_addr[DAC_N] = {0,2,3};
//DATA buffer
uint16_t DAC_buffer[NCHAN]; 
////////////////////////////////////////////////////////////////
uint8_t global_flags = 0;

uint8_t pkt_cnt = 0;
uint8_t eri_val = 0, econ1, erxfcon;
uint16_t erxrdpt, erxwrpt;

int main(void){
	
	uint8_t i;
	//timers
	struct timer periodic_timer;
	struct timer arp_timer;
	struct timer led_timer;
	struct timer ip_reset_button_timer;
	struct timer ENC20J60_periodic_reset_timer;
	
	hardware_init();
	i2c_init();
	//Analog circuit powerup delay
	delay_long(200);
	
	max582_set_addr(DAC_addr[0]);
	if(!max582x_init_ref()){
			global_flags |= DAC_FAULT;
	}
	
	for(i=1; i<DAC_N; i++)
	{
		max582_set_addr(DAC_addr[i]);
		if(!max582x_init())
			global_flags |= DAC_FAULT;
	}
	
	if( !dac_read_all() ){
		global_flags |= DAC_ERROR;
	}

	// TCP/IP
	uip_init();
	uip_arp_init();
	load_net_addr();
	
	enc28j60PhyWrite(PHLCON,0x476);
	
	net_interface_init();
	
	//ms timers
	timer_set( &periodic_timer, 1000);
	timer_set( &arp_timer, 10000);
	timer_set( &led_timer, 500);
	timer_set( &ENC20J60_periodic_reset_timer, 300000);
	
	while(1){
		/*
		//LED blinking
		//delay(80000);
		if(timer_expired(&led_timer)){
			//GPIOA->ODR &= ~GPIO_ODR_3;
			if (GPIOA->ODR & GPIO_ODR_3 ){
			GPIOA->ODR &= ~GPIO_ODR_3;
		}else{
			GPIOA->ODR |= GPIO_ODR_3;
		}
			timer_restart(&led_timer);
		}*/
		//Buttons
		if(!IP_RESET_BUTTON){
			if (!ip_reset_button_timer.run){
				timer_set(&ip_reset_button_timer, 3000);
			}
		}
		else if(ip_reset_button_timer.run){
				ip_reset_button_timer.run = 0;
		}
		if( timer_expired(&ip_reset_button_timer) && !IP_RESET_BUTTON){
			//Change IP/MAC to default
			set_default_net_addr();
			save_net_addr();
		}
		/*
		pkt_cnt = enc28j60Read(EPKTCNT);
		eri_val = enc28j60Read(EIR);
		erxrdpt = ((uint16_t)enc28j60Read(ERXRDPTH))<<8 | (uint16_t)enc28j60Read(ERXRDPTL);
		erxwrpt = ((uint16_t)enc28j60Read(ERXWRPTH))<<8 | (uint16_t)enc28j60Read(ERXWRPTL);
		econ1 = enc28j60Read(ECON1);
		erxfcon = enc28j60Read(ERXFCON);*/
		
		//TCP/IP
		//Periodically reset ENC28J60 if there are no opened connections
		//Just in case
		if( get_num_of_conns(UIP_CLOSED) == UIP_CONNS){
				if( timer_expired( &ENC20J60_periodic_reset_timer)){
					enc28j60Init();
					timer_restart( &ENC20J60_periodic_reset_timer);
				}
		}else{
				timer_restart( &ENC20J60_periodic_reset_timer);
		}
	
		uip_len = enc28j60PacketReceive(UIP_BUFSIZE, uip_buf);
		
		if (uip_len >0){
			if(BUF->type == htons(UIP_ETHTYPE_ARP)){
					uip_arp_arpin();
					if(uip_len > 0){
						uip_len = 0x3C;
						enc28j60PacketSend(uip_len, uip_buf );
					}
      }else if(BUF->type == htons(UIP_ETHTYPE_IP)){
				uip_arp_ipin();
				uip_input();
				if(uip_len > 0){
					uip_arp_out();
					enc28j60PacketSend(uip_len, uip_buf );
				}
			}
		//Periodic TCP/IP stuff
		}else if(timer_expired(&periodic_timer)){
				timer_restart(&periodic_timer);
				for(i = 0; i < UIP_CONNS; i++){
					uip_periodic(i);
					if(uip_len > 0){
						uip_arp_out();
						enc28j60PacketSend(uip_len, uip_buf );
					}
				}
		}else if(timer_expired(&arp_timer)){
				timer_restart(&arp_timer);
				uip_arp_timer();
		}
		//Change network addres
		if(global_flags & CHANGE_IP){
			uip_sethostaddr(my_net_addr.ip);
			global_flags &= ~CHANGE_IP;
		}
		if(global_flags & CHANGE_MAC){
			uip_setethaddr(my_net_addr.mac);
			enc28J60WriteMAC(my_net_addr.mac);
			global_flags &= ~CHANGE_MAC;
		}
	}
}

void TIM14_IRQHandler(){
	/*
	if (GPIOA->ODR & GPIO_ODR_3 ){
			GPIOA->ODR &= ~GPIO_ODR_3;
		}else{
			GPIOA->ODR |= GPIO_ODR_3;
		}*/
	//Clear flag
	TIM14->SR &= ~TIM_SR_UIF;
}
