void i2c_init(void);
void i2c_set_slave_addr(uint8_t);
int8_t i2c_tx(uint8_t *, uint8_t, uint8_t);
int8_t i2c_rx(uint8_t *, uint8_t, uint8_t);
int8_t i2c_rx_word(uint16_t *, uint8_t);
