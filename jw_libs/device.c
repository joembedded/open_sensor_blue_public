/* device.c */

#include <stdint.h>

#include "device.h"

// Wichtige Globals
const uint16_t device_type = DEVICE_TYP;
const uint16_t device_fw_version = DEVICE_FW_VERSION;
uint32_t mac_addr_h,mac_addr_l; 

uint32_t now_runtime; 
uint32_t now_time;
uint8_t ledflash_flag; 
uint16_t factory_test_lock; 

//**
