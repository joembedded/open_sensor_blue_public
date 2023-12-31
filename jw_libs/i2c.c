/*******************************************************************************
* I2C.C - Lib fuer LTX-I2C-Sensorik
*
* Wichtig: Der NRF-Treiber verwendet 7-Bit Adressen! (0..127)
*
*******************************************************************************/

#include "nordic_common.h"  // Diverse Allgemeine Defs
#include "nrf.h"  // Spezielle CPU/SDK Defs z.B. NRF52_SERIES, 'nrf52840.h'

#include <stdarg.h> // for var_args
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "boards.h"
#include "nrf_delay.h"
#include "nrf_drv_twi.h"

// JW Toolbox
#include "device.h"
#include "tb_tools.h"

#include "osx_main.h"
#include "osx_pins.h"
#include "i2c.h"


//-------------------------------- I2C ---------------------------------
// Globals
uint8_t i2c_uni_rxBuffer[I2C_UNI_RXBUF];
uint8_t i2c_uni_txBuffer[I2C_UNI_TXBUF];

// Locals
#define TWI_INSTANCE_ID     0
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);
static nrf_drv_twi_config_t twi_i2c_config = {
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = true // changed 8/21 to true
    };

/* Dynamische Initialisierung vor Verwendung fuer entsprechenden Bus 0..6: Extern, 7: intern */
int16_t ltx_i2c_init(uint8_t busno){
    ret_code_t err_code;


    switch(busno){
#if defined(I2C_BUS0_100KHZ_DIV) // in device.h - regulaer 100kHz I2c
    #define I2C_BUS0_SPEED  (NRF_DRV_TWI_FREQ_100K/I2C_BUS0_100KHZ_DIV)
    case 0: // Externer Bus #0 ; Optional langsamer wenn Divider definiert
       twi_i2c_config.frequency = I2C_BUS0_SPEED;
       twi_i2c_config.scl= BUS0_SCL;
       twi_i2c_config.sda= BUS0_SDA;
       break;
#endif
    case 7: // Interner Bus , Kann auch ohne PWR sein und Maximalspeed
       twi_i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
       twi_i2c_config.scl= IX_SCL;
       twi_i2c_config.sda= IX_SDA;
       break;

    default:
       return -1; // unbekannter bus
    }

    err_code = nrf_drv_twi_init(&m_twi, &twi_i2c_config, NULL, NULL); // Blocking and no context
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
    return 0;
}

void ltx_i2c_uninit(uint8_t busno, bool port_no_default){ // CPU3(?): leichter PU nach uninit auf SDA/SCL
    nrf_drv_twi_disable(&m_twi);
    nrf_drv_twi_uninit(&m_twi); // Void

    // busno nicht gebraucht. Per Default bleiben PU nach twi_uninit anscheinend an. Daher nur aktiv abschalten..
    if(port_no_default == false){
      nrf_gpio_cfg_default(twi_i2c_config.scl);
      nrf_gpio_cfg_default(twi_i2c_config.sda);
    }
}

//------ Einfach SCAN-Routine I2C R,RW,W (Hier R LIS12H-WhoAmI) ----------
void ltx_i2c_scan(uint8_t dir, uint8_t busno){
    uint8_t i;
    int32_t res;
    tb_printf("---Scan I2C - M(W,RW,R):%u ---\n", dir);
    tb_printf("Start (Exit: <Key>)\n");
    res=ltx_i2c_init(busno);
    if(!res){
      for(i=0;i<127;i++){
        if(dir==0){
          res=i2c_write_blk(i,0);  // (Alt. A) Achtung: Read nicht nehmen, kann haengen (zieht dann irgendein Sensor SDA runter)
        }else if(dir==1){
          i2c_uni_txBuffer[0]=0x0F; // WhoAmI (Alt. B) mit Repeated Start!
          res=i2c_readwrite_blk_wt(i,1,1,0); // (Alt. B)
        }else if(dir==2){
          res=i2c_read_blk(i,1);  // (Alt. C) ReadOnly
        }else break;            // Unknown Scan CMD
        if(res>=0) tb_printf("\nADDR %d: %u\n",i,res);
        else if(res==-ERROR_HW_WRITE_NO_REPLY || res==-ERROR_HW_READ_NO_REPLY) tb_printf("-"); // No Reply
        else tb_printf("(%d)",res); // Reply Code
        tb_delay_ms(1);
        if(tb_getc()>0) break;
      }
      ltx_i2c_uninit(busno,false); // Disconnect fuer Tests
    }else tb_printf("I2C Init:%d\n",res);
    tb_printf("\n---OK---\n");
}



//-------------------------------- Fkts START ---------------------------------
// 2 Universelle I2C-Puffer
uint8_t i2c_uni_rxBuffer[I2C_UNI_RXBUF];
uint8_t i2c_uni_txBuffer[I2C_UNI_TXBUF];

// Einfache Block-Schreib-routine I2C. Zu sendende Bytes stehen im i2c_uni_txBuffer
// Return 0: OK. WICHTIG: I2C-Addr 0..127 (7Bit, Manche Hersteller geben 8-Bit Adressen an)
int16_t i2c_write_blk(uint8_t i2c_addr, uint8_t anz ){
    ret_code_t err_code;
    // drv_twi_tx OK with permanent pulldown of SCL or SDA: internal Timeout after ca. 700 msec
    err_code = nrf_drv_twi_tx(&m_twi, i2c_addr, i2c_uni_txBuffer, anz, false);
    if(err_code != NRF_SUCCESS){
      if(err_code == NRF_ERROR_INTERNAL) return -ERROR_HW_SEND_CMD; // Timout: Z.B. Bus auf LOW fix
      return -ERROR_HW_WRITE_NO_REPLY;
    }
    return 0; // OK
}
// Einfache Block-Leseroutine I2C. 
// Return 0: OK und gelesene Bytes stehen im i2c_uni_rxBuffer, max. 255 Bytes
// Rueckgabe je nach anz:
// anz: 0 oder >2: 0: OK
// anz: 1: Gelesenes Byte
// anz: 2: Gelesenes Wort als BE. Achtung: STM anscheinen LE?
//
// *** Achtung: Manche Sensoren halten SDA oder SCL auf 0 (ergibt ERROR_HW_SEND_CMD(9)), da hilft dann nur noch RESET
// Solche Sensoren koennen nur durch Stromabschalten resettet werden!
// -> Scannen von Sensoren mit read(adr,0) ist keine gute Idee, erzeugt leicht diesen Effekt!
// 
int32_t i2c_read_blk(uint8_t i2c_addr, uint8_t anz){
    ret_code_t err_code;
    err_code = nrf_drv_twi_rx(&m_twi, i2c_addr, i2c_uni_rxBuffer, anz);
    if(err_code != NRF_SUCCESS){
      if(err_code == NRF_ERROR_INTERNAL) return -ERROR_HW_SEND_CMD; // Timout: Z.B. Bus auf LOW fix
      return -ERROR_HW_READ_NO_REPLY;
    }
    // Rueckgabe 
    if(anz==1) return i2c_uni_rxBuffer[0];
    if(anz==2) return (uint16_t)((i2c_uni_rxBuffer[0]<<8)+i2c_uni_rxBuffer[1]); // BE (neu: 7/22)
    return 0;
 }

// Kombiroutine R/W mit Repeated Start Condition und opt. Waittime
int32_t i2c_readwrite_blk_wt(uint8_t i2c_addr, uint8_t anz_w, uint8_t anz_r, uint16_t wt_ms){
    ret_code_t err_code;
    // drv_twi_tx OK with permanent pulldown of SCL or SDA: internal Timeout after ca. 700 msec
    err_code = nrf_drv_twi_tx(&m_twi, i2c_addr, i2c_uni_txBuffer, anz_w, true); // CMDs senden, nostop=true: Repeated Start
    if(err_code != NRF_SUCCESS){
      if(err_code == NRF_ERROR_INTERNAL) return -ERROR_HW_SEND_CMD; // Timout: Z.B. Bus auf LOW fix
      return -ERROR_HW_WRITE_NO_REPLY;
    }
    if(wt_ms) tb_delay_ms(wt_ms); // Optional Warten
    return i2c_read_blk(i2c_addr,anz_r);
}

//***

