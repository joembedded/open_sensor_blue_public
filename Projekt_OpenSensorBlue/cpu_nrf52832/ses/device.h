//**** Device.h - Defines LEO Type and Features  ***

// *** Select DEVICE_TYP to build here: ***
// Typen 100-299 sind Loader/Inits 300-999 Sensoren! >=1000..xx FS-Anwendungen
// ---Loaders/Test-Basis---
//#define DEVICE_TYP  200 // Dummy For Tests or Bootloader (ASX/OSX Ident.)

// ---800-899: Open-Sensor-Blue - Projekte mit BLE-Lib ---
#define DEVICE_TYP  800  // *** MARDER  **

#define NO_FS            // Sensoren haben i.d.R: kein FS

// Features
// ---Test/Loaders---
#if DEVICE_TYP == 200
  #define DEV_PREFIX "AMX"  // exakt 3 Zeichen - Standard "Aquatos Master xxx" // AMX-Bootloader (ident. OSX-Boot)
  // Div. LTX-Tracker/LTX-Pegel BLE-User-"Device for Bootloader" - Minimalversion fuer Sensoren OHNE Speicher
  // Der BLE-/Test-Typ kann garnix, NUR (opt.) Disk, Terminal und BLE, damit kann man aber andere Firmware nachladen
  #define ENABLE_BLE // Wenn definiert SD anwerfen fuer IRQs, ***Set to OFF for Easy Debug!***
  #define DEVICE_FW_VERSION 2 // Release in Steps of 10 (35 == V3.5, 1: V0.1)  
#endif


#if DEVICE_TYP == 800
  #define DEV_PREFIX "OSB"  // exakt 3 Zeichen - Standard "Open_Sensor_Blue"
  #define ENABLE_BLE // Wenn definiert SD anwerfen fuer IRQs

  #define DEVICE_FW_VERSION 1 // Release in Steps of 10 (35 == V3.5, 1: V0.1) 
  #define DEVICE_SIMPLETIMER 1000          // Zyklus immer 1 sec
#endif



#ifndef DEBUG
#if !defined(ENABLE_BLE)
  #warning "Release: BLE OFF?"
#endif
#endif

// Wichtige Globals
extern uint32_t mac_addr_h,mac_addr_l; // Muss von MAIN gesetzt werden, da SD Zugriff auf diese Register blockt
extern uint8_t ledflash_flag;    // Radio-Active-Notification/Heartbeat/Etc
extern uint32_t now_runtime; // Aktuelle Runtime (gemerkt, incrementiert konst.)
extern uint32_t now_time; // Aktuelle UNIX-UTC Zeit (gemerkt)

extern const uint16_t device_type; 
extern const uint16_t device_fw_version;

// **



