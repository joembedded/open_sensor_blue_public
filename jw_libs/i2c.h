/*******************************************************************************
* I2C.H - Lib fuer LTX-I2C-Sensorik
*******************************************************************************/

int16_t ltx_i2c_init(uint8_t busno); // 11/2021: mehrere Busse
void ltx_i2c_uninit(uint8_t busno, bool port_no_default); 
void ltx_i2c_scan(uint8_t dir, uint8_t busno);

#define I2C_UNI_RXBUF 16          // ACHTUNG: Sollte gross genug sein, z.B. fuer UHR oder Sensoren
#define I2C_UNI_TXBUF 16          // dto. 
extern uint8_t i2c_uni_rxBuffer[I2C_UNI_RXBUF];
extern uint8_t i2c_uni_txBuffer[I2C_UNI_TXBUF];

int16_t i2c_write_blk(uint8_t i2c_addr, uint8_t anz );
int32_t i2c_read_blk(uint8_t i2c_addr, uint8_t anz);
int32_t i2c_readwrite_blk_wt(uint8_t i2c_addr, uint8_t anz_w, uint8_t anz_r, uint16_t wt_ms);

// anstelle von ltx_defs.h:
#define ERROR_HW_WRITE_NO_REPLY 9
#define ERROR_HW_READ_NO_REPLY  10
#define ERROR_HW_SEND_CMD 11

// ***
