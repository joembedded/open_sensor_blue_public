/****************************************************
* osx_main.c - OpenSensor-Driver OHNE SDI12!
*
* (C) joembedded@gmail.com - joembedded.de
*
* UART is shared with tb_tools
*
***************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nrf_soc.h> // sd_temp

#include "poly16.h"

#include "./platform_nrf52/tb_pins_nrf52.h"
#include "device.h"
#include "allgdef.h"


#include "osx_main.h"
#include "osx_pins.h"
#include "saadc.h"
#include "tb_tools.h"
#include "bootinfo.h"
#include "filepool.h"

#ifdef ENABLE_BLE
#include "ltx_ble.h"
#endif
#include "intmem.h"

// ---Globals---


// Systemdaten OSX von Disk laden/initialisieren
void system_init(void) {
  // z.B. Erstmalige Annahme fuer den Advertising-Namen
  // e.g. Advertising Data
#ifdef ENABLE_BLE
  // Try to read User-AdvertisingName
  int16_t res =intpar_mem_read(ID_INTMEM_ADVNAME, BLE_DEVICE_NAME_MAXLEN, (uint8_t *)&uni_line);
  if(res>=3) {
    uni_line[res]=0; // Terminate String
    strcpy(ble_device_name,uni_line);
  }
#endif
}


//--- Init (only call once)
void device_init(void) {
  // Hier evtl. IRQs init

  // - Setup lesen 
  // Try to read Parameters

  sensor_init(); // ID etc..

}


void type_cmdprint_line(uint8_t isrc, char *pc) {
  if (isrc == SRC_CMDLINE) {
    tb_printf("%s\n", pc);
#ifdef ENABLE_BLE
  } else if (isrc == SRC_BLE) {
    ble_putsl(pc);
#endif
  }
}



// Die Input String und Values
int16_t device_cmdline(uint8_t isrc, uint8_t *pc, uint32_t val) {
  int16_t res; 
#ifdef HAS_DEVICE_TYPE_CMDLINE
    res = device_type_cmdline(isrc,pc);
    if(res !=-ERROR_CMD_UNKNOWN) return res;
#endif
  res  = 0;

  switch (*pc) { // pc nicht veraendern wg. DBG

  case 'e': // Measure
    sensor_measure(isrc); // Sensor messen
    break;

#ifdef ENABLE_BLE
  case 'n': // Name
    pc++;
    while(*pc==' ') pc++;
    int16_t hl = strlen(pc);
    if(hl>=3){  // Adv-Name 3..(11) Zeichen
      if(hl>BLE_DEVICE_NAME_MAXLEN) hl = BLE_DEVICE_NAME_MAXLEN;
      intpar_mem_write(ID_INTMEM_ADVNAME,hl , pc); // return: Anzahl Bytes used in intmem, ignored
    }else res = -ERROR_TOO_SHORT_DATA;
    break;
#endif

#if DEBUG
  default:
    // If DEBUG Type specific Debugging via UART
    if (isrc == SRC_CMDLINE)
      if(debug_tb_cmdline(pc, val)==true) res=0; // extern, so gut wie OK
#endif
  }
  return res; // Command processed
}

//***