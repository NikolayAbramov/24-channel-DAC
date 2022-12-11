/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher 
 * Copyright:LGPL V2
 * See http://www.gnu.org/licenses/old-licenses/lgpl-2.0.html
 *
 * Based on the enc28j60.c file from the AVRlib library by Pascal Stang.
 * For AVRlib See http://www.procyonengineering.com/
 * Used with explicit permission of Pascal Stang.
 *
 * Title: Microchip ENC28J60 Ethernet Interface Driver
 *********************************************/
 
#include "enc28j60.h"
#include "stm32f0xx.h"
#include "Source/timer.h"
#include "Source/hardware_init.h"

static uint8_t Enc28j60Bank;
static int16_t gNextPacketPtr;
uint8_t* enc28j60_mac;

#define ENC28J60_SPI SPI1
#define ENC28J60_SPI_CLOCK_EN ( RCC->APB2ENR|=RCC_APB2ENR_SPI1EN )

//Delays
#define DELAY_RST 160000//20ms
#define DELAY_TX_UNLK 80000//10ms
#define DELAY_PHY_WR 80//10us
//Dlay after write operation 1/4 of SCK period
#define DELAY_WR 64

//SPI speed PCLK/2^(0b000...111 + 1)
//#define SPI_BR  ( SPI_CR1_BR_1 | SPI_CR1_BR_0 );
#define SPI_BR  ( SPI_CR1_BR_0  );

void enc28j60Tx(uint8_t data)
{
	//Wait for tx buffer empty
	while( (ENC28J60_SPI->SR & SPI_SR_TXE) == 0 );
	*(uint8_t *)&(SPI1->DR) = data;
	//Wait until transmission complete
	while( ENC28J60_SPI->SR & SPI_SR_BSY );
}

uint8_t enc28j60Rx()
{
	uint8_t data;
	//Wait for tx buffer empty
	while( ENC28J60_SPI->SR & SPI_SR_BSY );
	//Clear rx buffer
	while( ENC28J60_SPI->SR & SPI_SR_RXNE )
	{
		data = *(uint8_t *)&ENC28J60_SPI->DR;
	}
	//Start reading
	*(uint8_t *)&(SPI1->DR) = 0;
	//Wait for rx buffer not empty and get data
	while( (ENC28J60_SPI->SR & SPI_SR_RXNE) == 0 );
	data = *(uint8_t *)&ENC28J60_SPI->DR;
	return (data);
}

uint8_t enc28j60ReadOp(uint8_t op, uint8_t address)
{
        uint8_t data;
	
				ENC28J60_CS_ACTIVE;
        // issue read command
				enc28j60Tx( op | (address & ADDR_MASK) );
        // read data
				data = enc28j60Rx();
        // do dummy read if needed (for mac and mii, see datasheet page 29)
        if(address & 0x80)
        {
           data = enc28j60Rx();
        }
        // release CS
        ENC28J60_CS_PASSIVE;
        return(data);
}

void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data)
{
        ENC28J60_CS_ACTIVE;
        // issue write command
        enc28j60Tx( op | (address & ADDR_MASK) );
        // write data
        enc28j60Tx( data );
				delay(DELAY_WR);
        ENC28J60_CS_PASSIVE;
}

void enc28j60ReadBuffer(uint16_t len, uint8_t* data)
{
        ENC28J60_CS_ACTIVE;
        // issue read command
        enc28j60Tx( ENC28J60_READ_BUF_MEM );
        while(len)
        {
                len--;
                // read data
                *data = enc28j60Rx();
                data++;
        }
        *data='\0';
        ENC28J60_CS_PASSIVE;
}

void enc28j60WriteBuffer(uint16_t len, uint8_t* data)
{
        ENC28J60_CS_ACTIVE;
        // issue write command
        enc28j60Tx( ENC28J60_WRITE_BUF_MEM );
        while(len)
        {
                len--;
                // write data
                enc28j60Tx( *data );
                data++;
        }
        ENC28J60_CS_PASSIVE;
}

void enc28j60SetBank(uint8_t address)
{
        // set the bank (if needed)
        if((address & BANK_MASK) != Enc28j60Bank)
        {
                // set the bank
                enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
                enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
                Enc28j60Bank = (address & BANK_MASK);
        }
}

uint8_t enc28j60Read(uint8_t address)
{
        // set the bank
        enc28j60SetBank(address);
        // do the read
        return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

// read 16 bits
uint16_t enc28j60PhyRead(uint8_t address)
{
	// Set the right address and start the register read operation
	enc28j60Write(MIREGADR, address);
	enc28j60Write(MICMD, MICMD_MIIRD);
	// wait until the PHY read completes
	while(enc28j60Read(MISTAT) & MISTAT_BUSY);
	// reset reading bit
	enc28j60Write(MICMD, 0x00);
        // get data value from MIRDL and MIRDH
	return ((enc28j60Read(MIRDH)<<8)|enc28j60Read(MIRDL));
}

void enc28j60Write(uint8_t address, uint8_t data)
{
        // set the bank
        enc28j60SetBank(address);
        // do the write
        enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

void enc28j60PhyWrite(uint8_t address, uint16_t data)
{
        // set the PHY register address
        enc28j60Write(MIREGADR, address);
        // write the PHY data
        enc28j60Write(MIWRL, data);
        enc28j60Write(MIWRH, data>>8);
        // wait until the PHY write completes
        while(enc28j60Read(MISTAT) & MISTAT_BUSY){
                delay(DELAY_PHY_WR); // 10us
					
        }
}

void enc28j60clkout(uint8_t clk)
{
        //setup clkout: 2 is 12.5MHz:
	enc28j60Write(ECOCON, clk & 0x7);
}

void enc28J60WriteMAC (uint8_t * macaddr)
{
	 enc28j60Write(MAADR5, macaddr[0]);
     enc28j60Write(MAADR4, macaddr[1]);
     enc28j60Write(MAADR3, macaddr[2]);
     enc28j60Write(MAADR2, macaddr[3]);
     enc28j60Write(MAADR1, macaddr[4]);
     enc28j60Write(MAADR0, macaddr[5]);
}

void enc28j60Init()
{
	//Enable clock for SPI1
	ENC28J60_SPI_CLOCK_EN;
	//SPI configuration
	ENC28J60_SPI->CR1 = 0;
	ENC28J60_SPI->CR2 = 0;
	
	ENC28J60_SPI->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI; //Master, NSS pin disabled
	ENC28J60_SPI->CR1 |= SPI_BR;
	
	ENC28J60_SPI->CR2 |= SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0 | SPI_CR2_FRXTH; //Data lenght 8bit
	
	ENC28J60_SPI->CR1 |= SPI_CR1_SPE; //Enable SPI

	//perform system reset
	//enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET); CSACTIVE;
        // issue write command
				ENC28J60_CS_ACTIVE;
        enc28j60Tx( ENC28J60_SOFT_RESET );
        // write data
        enc28j60Tx( ENC28J60_SOFT_RESET );
        ENC28J60_CS_PASSIVE;
	
        delay(DELAY_RST); // 20ms
	// check CLKRDY bit to see if reset is complete
        // The CLKRDY does not work. See Rev. B4 Silicon Errata point. Just wait.
	//while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	
	gNextPacketPtr = RXSTART_INIT;
        // Rx start
	enc28j60Write(ERXSTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXSTH, RXSTART_INIT>>8);
	// set receive pointer address
	enc28j60Write(ERXRDPTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXRDPTH, RXSTART_INIT>>8);
	// RX end
	enc28j60Write(ERXNDL, RXSTOP_INIT&0xFF);
	enc28j60Write(ERXNDH, RXSTOP_INIT>>8);
	// TX start
	enc28j60Write(ETXSTL, TXSTART_INIT&0xFF);
	enc28j60Write(ETXSTH, TXSTART_INIT>>8);
	// TX end
	enc28j60Write(ETXNDL, TXSTOP_INIT&0xFF);
	enc28j60Write(ETXNDH, TXSTOP_INIT>>8);
	// do bank 1 stuff, packet filter:
        // For broadcast packets we allow only ARP packtets
        // All other packets should be unicast only for our mac (MAADR)
        //
        // The pattern to match on is therefore
        // Type     ETH.DST
        // ARP      BROADCAST
        // 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
        // in binary these poitions are:11 0000 0011 1111
        // This is hex 303F->EPMM0=0x3f,EPMM1=0x30
	/*
	enc28j60Write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
	enc28j60Write(EPMM0, 0x3f);
	enc28j60Write(EPMM1, 0x30);
	enc28j60Write(EPMCSL, 0xf9);
	enc28j60Write(EPMCSH, 0xf7);
*/    
//N_N: Just set unicast(our MAC only) and broadcast fiters combined with OR (bit ANDOR=0) and also CRC check
//the above shit with pattern doesn't work and allows receptin of all the crap (see flowchart in datasheet)
 enc28j60Write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_BCEN);
	// do bank 2 stuff
	// enable MAC receive
	enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	enc28j60Write(MACON2, 0x00);
	// enable automatic padding to 60bytes and CRC operations
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
	// set inter-frame gap (non-back-to-back)
	enc28j60Write(MAIPGL, 0x12);
	enc28j60Write(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
	enc28j60Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
        // Do not send packets longer than MAX_FRAMELEN:
	enc28j60Write(MAMXFLL, MAX_FRAMELEN&0xFF);	
	enc28j60Write(MAMXFLH, MAX_FRAMELEN>>8);
	// do bank 3 stuff
        // write MAC address
        // NOTE: MAC address in ENC28J60 is byte-backward
        enc28J60WriteMAC (enc28j60_mac);
	// no loopback of transmitted frames
	enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);
	// switch to bank 0
	enc28j60SetBank(ECON1);
	// enable interrutps
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

}

// read the revision of the chip:
uint8_t enc28j60getrev(void)
{
        uint8_t rev;
        rev=enc28j60Read(EREVID);
        // microchip forgott to step the number on the silcon when they
        // released the revision B7. 6 is now rev B7. We still have
        // to see what they do when they release B8. At the moment
        // there is no B8 out yet
        if (rev>5) rev++;
	return(rev);
}

// dhcp needs general broadcast
#ifdef ENC28J60_BROADCAST
// A number of utility functions to enable/disable general broadcast (not just arp)
void enc28j60EnableBroadcast( void ) {
        enc28j60Write(ERXFCON, (uint8_t)((enc28j60Read(ERXFCON) | ERXFCON_BCEN)));
}
void enc28j60DisableBroadcast( void ) {
        enc28j60Write(ERXFCON, enc28j60Read(ERXFCON) & (0xff ^ ERXFCON_BCEN));
}
#endif

// link status
uint8_t enc28j60linkup(void)
{
        // PHSTAT1 LLSTAT (= bit 2 in lower reg), PHSTAT1_LLSTAT
        // LLSTAT is latching, that is: if it was down since last
        // calling enc28j60linkup then we get first a down indication
        // and only at the next call to enc28j60linkup it will come up.
        // This way we can detect intermittened link failures and
        // that might be what we want.
        // The non latching version is LSTAT.
        // PHSTAT2 LSTAT (= bit 10 in upper reg)
        if (enc28j60PhyRead(PHSTAT2) & (1<<10) ){
        //if (enc28j60PhyRead(PHSTAT1) & PHSTAT1_LLSTAT){
                return(1);
        }
        return(0);
}

void enc28j60PacketSend(uint16_t len, uint8_t* packet)
{
        // Check no transmit in progress
				struct timer wait_timer;
	
				timer_set( &wait_timer, 2000);
        while (enc28j60ReadOp(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_TXRTS)
				{
					if( timer_expired(&wait_timer)){
						enc28j60Init();
						break;
					}
				}
        // 
        // Reset the transmit logic problem. Unblock stall in the transmit logic.
        // See Rev. B4 Silicon Errata point 12.
        if( (enc28j60Read(EIR) & EIR_TXERIF) ) {
                enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
                enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
                enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXERIF); 
                delay(DELAY_TX_UNLK); // 10ms
        }
	// Set the write pointer to start of transmit buffer area
	enc28j60Write(EWRPTL, TXSTART_INIT&0xFF);
	enc28j60Write(EWRPTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	enc28j60Write(ETXNDL, (TXSTART_INIT+len)&0xFF);
	enc28j60Write(ETXNDH, (TXSTART_INIT+len)>>8);
	// write per-packet control byte (0x00 means use macon3 settings)
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);
	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

// just probe if there might be a packet
uint8_t enc28j60hasRxPkt(void)
{
	if( enc28j60Read(EPKTCNT) ==0 ){
		return(0);
        }
        return(1);
}

// Gets a packet from the network receive buffer, if one is available.
// The packet will by headed by an ethernet header.
//      maxlen  The maximum acceptable length of a retrieved packet.
//      packet  Pointer where packet data should be stored.
// Returns: Packet length in bytes if a packet was retrieved, zero otherwise.
uint16_t enc28j60PacketReceive(uint16_t maxlen, uint8_t* packet)
{
	uint16_t rxstat;
	uint16_t len;
	
	//RXERIF may be a sign of hanging ENC28J60 
	//we'd better to reset it
	if (enc28j60Read(EIR) & EIR_RXERIF ){
			enc28j60Init();
			return(0);
	}
	
	// check if a packet has been received and buffered
	//if( !(enc28j60Read(EIR) & EIR_PKTIF) )
        // The above does not work. See Rev. B4 Silicon Errata point 6.
	if( enc28j60Read(EPKTCNT) ==0 ){
		return(0);
        }

	// Set the read pointer to the start of the received packet
	enc28j60Write(ERDPTL, (gNextPacketPtr &0xFF));
	enc28j60Write(ERDPTH, (gNextPacketPtr)>>8);
	// read the next packet pointer
	gNextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	gNextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the packet length (see datasheet page 43)
	len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
        len-=4; //remove the CRC count
	// read the receive status (see datasheet page 43)
	rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= ((uint16_t)enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0))<<8;
	// limit retrieve length
        if (len>maxlen-1){
                len=maxlen-1;
        }
        // check CRC and symbol errors (see datasheet page 44, table 7-3):
        // The ERXFCON.CRCEN is set by default. Normally we should not
        // need to check this.
        if ((rxstat & 0x80)==0){
                // invalid
                len=0;
        }else{
                // copy the packet from the receive buffer
                enc28j60ReadBuffer(len, packet);
        }
	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	//enc28j60Write(ERXRDPTL, (gNextPacketPtr &0xFF));
	//enc28j60Write(ERXRDPTH, (gNextPacketPtr)>>8);
        //
        // Move the RX read pointer to the start of the next received packet
        // This frees the memory we just read out. 
        // However, compensate for the errata point 13, rev B4: never write an even address!
        // gNextPacketPtr is always an even address if RXSTOP_INIT is odd.
        if (gNextPacketPtr -1 > RXSTOP_INIT){ // RXSTART_INIT is zero, no test for gNextPacketPtr less than RXSTART_INIT.
                enc28j60Write(ERXRDPTL, (RXSTOP_INIT)&0xFF);
                enc28j60Write(ERXRDPTH, (RXSTOP_INIT)>>8);
        } else {
                enc28j60Write(ERXRDPTL, (gNextPacketPtr-1)&0xFF);
                enc28j60Write(ERXRDPTH, (gNextPacketPtr-1)>>8);
        }
	// decrement the packet counter indicate we are done with this packet
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
	return(len);
}

