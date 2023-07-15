/****************************************************
* poly16.c
*
* (C) joembedded@gmail.com - joembedded.de
* (calc_sdi12_crc16() also in SDI12Term.c / -Projekt)
*
***************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "poly16.h"

/* Calculating the SDI12 CRC16: Also useful for external use. 
* SDI12: Init CRC_RUN with 0x0
* MODBUS: Same Polynom is used for , but Init is 0xFFFF) 
*/

#define POLY16 0xA001
uint16_t poly16_track_crc16(uint8_t *pdata, uint16_t wlen, uint16_t crc_run) {
  uint8_t j;
  while (wlen--) {
    crc_run ^= *pdata++;
    for (j = 0; j < 8; j++) {
      if (crc_run & 1)
        crc_run = (crc_run >> 1) ^ POLY16;
      else
        crc_run = crc_run >> 1;
    }
  }
  return crc_run;
}

//***
