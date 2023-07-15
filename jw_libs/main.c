/*******************************************************************************
 * main.c -  Mit S112/S113, Achtung: das geht nur bis 4db TX
 * OopenSensorBlue
 *
 * Startup fuer:
 * - Loaders (100-200)
 * - OpenSensorBlue (OSB) - **NUR** Device-Typen 800-899 ohne serielles Flash
 *
 * main.c: Bildet Geruest fur Logger/Sensoren mit/ohne BLE
 * - keine externe RTC
 * - kein Filesystem
 * - Simpletimer
 * - CPU32 oder CPU40
 *
 * (C) joembedded@gmail.com - joembedded.de
 *
 *******************************************************************************/

#define GID 1 // Guard-ID fuer dieses Modul

#include <stdarg.h> // for var_args
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "device.h"
#include "allgdef.h"

//#define USE_NOTIFICATIONS   // BLE-Notifications sind nicht unbedingt noetig, ausser SIMPLETIMER wird nicht benutzt.
// SIMPLETIMER: Bei 1 sec. ca. 4uA
#if DEBUG
  #define USE_SIMPLETIMER 1000 // Schneller!
#elif defined(DEVICE_SIMPLETIMER)
  #define USE_SIMPLETIMER DEVICE_SIMPLETIMER
#else
  #define USE_SIMPLETIMER 4000 // Wenn KEIN BLE oder keine Notifications ist der Timer noetig, mit Timeout
#endif

#include "ltx_ble.h"
#include "ltx_errors.h"

#include "app_timer.h" // APP_TIMER_TICKS
#include "app_util_platform.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_ble_gatt.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_nvmc.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_queue.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "osx_main.h" // und OSX-Master verwendet
#include "osx_pins.h"

#include "intmem.h"

// JW Toolbox
#include "jesfs.h"
#include "jesfs_int.h" // fs_track_crc32()
#include "logfiles.h" // tbble_printf..
#include "tb_tools.h"

#include "intmem.h"

#include "bootinfo.h"
#include "filepool.h"
#include "i2c.h"
#include "saadc.h"


//---------------- Periodic_sec Verwaltungsvariablen -----------------
static uint32_t reset_reason_bits; // Backup Flags nach Reset vor Memeory Protection
void cmdline_check(void);          // formwar decl.

#if DEBUG
uint32_t dbg_idle_cnt; // Zaehlt Wakeups
#endif

// Jedes Quellfile kriegt das einzeln, falls LOC_ERROR_CHECK verwendet wird
static void local_error(uint32_t error, uint16_t line_num) {
  NRF_LOG_ERROR("ERROR %u in Line %u\n", error, line_num);
  NRF_LOG_FLUSH();
  app_error_handler(error, line_num, (uint8_t *)__FILE__);
}

//=== Platform specific, Helpers for JesFs-time ===
uint32_t _time_get(void) {
  return tb_time_get();
}
int16_t _supply_voltage_check(void){
  // Wenn Spannung zu nieder einfach garnix machen
  if(get_adintern(false)<2500)  return -1; // Power check(2500:Minimum 2.2V, 2275:2V)
  return 0;
}

/* Immer aufrufen, aber nur max. 1 mal/sec Sekunden durchlaufen
 * Diese Routine soll die regelmaessigen Tasks erledigen:
 * Messen/Alarme/Display/Timeouts */
void periodic_secs(void) {
  uint32_t nsecs;

  char stat;
  int16_t res;

  // --- Periodic each second ---
  nsecs = tb_get_runtime();
  if (nsecs == now_runtime)
    return; // War schon in dieser Sekunde.

  now_runtime = nsecs;               // *Globale Var. Runtime fuer aktuellen Moment
  now_time = tb_runtime2time(nsecs); // *Globale Time fuer aktuellen Moment
  tb_watchdog_feed(1);               // Feed Watchdog (fuer 1 Zyklus, ca. 4 min)
  ledflash_flag = 1;

  cmdline_check(); // Terminal max. alle sec

  stat = '.'; // 0 fuer ign.

#ifdef ENABLE_BLE
  if (ble_connected_flag == true) {
    stat = 'o';
    // Zaehlt alle Sekunde hoch
    if (ble_comm_started_flag == true) {
      // Nur wenn Notificatiions auch erlaubt sind
      ble_rssi_report_cnt++;
      stat = '*';
    }
    last_ble_cmd_cnt++;

    if (ble_isfastspeed()) {
      tbble_printf("AUTO:Slow\n");
      ble_change2slow();
    } else if (last_ble_cmd_cnt == (MAX_BLE_SESSION_SECS - 15)) { // Timeout abgelaufen -> Disconnect
      ble_printf("~X");                                           // Disconnect soon/Request for Disconnect

    } else if (last_ble_cmd_cnt >= MAX_BLE_SESSION_SECS) { // Timeout abgelaufen -> Disconnect
      ble_disconnect();
      last_ble_cmd_cnt = 0;
    }
  }
#endif
  if (device_periodic())
    tb_putc(stat); // 1-2 Sekunden-Check (nur UART)
}


// Init UART and Fopt. ilesystem, set FS then to sleep
void jw_drv_init(void) {
  int16_t res;

  tb_init(); // Init Toolbox (ohne Watchdog) und Minimal (wie Std. pca10056)

  tb_watchdog_init();
  tb_watchdog_feed(1); // For 240 sec

  conv_secs_to_date_buffer(get_firmware_bootloader_cookie());
  tb_printf("\n\n*** " DEV_PREFIX " Type:%u V%u.%u(Built:%s) (C)JoEmbedded.de\n\n", DEVICE_TYP, DEVICE_FW_VERSION / 10, DEVICE_FW_VERSION % 10, date_buffer);
#ifdef NRF52832_XXAA
  tb_putsl("CPU: nRF52832\n");
#endif
#ifdef NRF52840_XXAA
  tb_putsl("CPU: nRF52840\n");
#endif

#ifdef DEBUG
  uint32_t uval = reset_reason_bits;

  tb_printf("Reset-Reason: 0x%X ", uval);
  if (uval & 1)
    tb_putsl("(Pin-Reset)"); // The low Nibble Reasons
  if (uval & 2)
    tb_putsl("(Watchdog)");
  if (uval & 4)
    tb_putsl("(Soft-Reset)");
  if (uval & 8)
    tb_putsl("(CPU Lockup)");
  tb_printf(", Bootcode: 0x%x\n", tb_get_bootcode(false));
  tb_printf("Time: %u\n", tb_time_get());

  uint8_t *psec = get_security_key_ptr();
  int16_t i;
  if (psec) {
    tb_putsl("KEY: ");
    for (i = 0; i < 16; i++) {
      tb_printf("%02X", psec[i]);
    }
    tb_putc('\n');
  }
#endif
  GUARD(GID); // GUARD: Save THIS line as last visited line in Module GID

  mac_addr_h = NRF_FICR->DEVICEADDR[1];
  mac_addr_l = NRF_FICR->DEVICEADDR[0];
  srand(mac_addr_l);
  tb_printf("MAC: %08X%08X\n", mac_addr_h, mac_addr_l);

#ifdef ENABLE_BLE                                          // Erstmalige Annahme fuer den Advertising-Namen
  sprintf(ble_device_name, DEV_PREFIX "%08X", mac_addr_l); // LowWord of Device Addr is ID !! LEN!
#endif
}

//========== Arbeitsflaeche===============

/************************** Lokale UART-Kommando-Schnittstelle ***************************/

// Input Line (UNI)
#define INPUT_LEN 80
uint8_t input[INPUT_LEN + 1];

//=== cmdline ===
void uart_cmdline(void) {
  int32_t i;
  int16_t res;
  uint32_t val, val2;
  char *pc;
#ifdef DEBUG
  float fval; // Debug (VBat, tt)
#endif

  tb_putc('>');
  res = tb_gets(input, INPUT_LEN, 20000, 1); // Max. 20 secs for input, echo
  tb_putc('\n'); // Achtung: Bei Problemen hier: CPU LOESCHEN!!! Liegt an Parametern!
  if (res < 0) {
    tb_putsl("<UART ERROR>");
    tb_delay_ms(2);
    tb_uninit();
    return;
  } else if (res >= 0) {
    uint32_t t0 = tb_get_ticks();
    pc = input + 1;
    val = strtol(pc, &pc, 0);  // Parameter 1
    val2 =strtol(pc, &pc, 0); // Parameter 2


    switch (*input) {
      // --------- Commands -------------------
#ifdef DEBUG
    //==== Memory-Zeugs  ====
    case 'H': // Adresse, 256 Bytes zeigen
      tb_putsl("Memory:");
      for (i = 0; i < 256; i++) {
        if (!(i % 16))
          tb_printf("\n%x:", val);
        tb_printf(" %02x", *(uint8_t *)val++);
      }
      tb_putc('\n');
      break;
    case 'K': // Geht nur auf nrf52832 ohne MWU
      if (val & 4095)
        tb_putsl("Error: Sector Adr.\n");
      else {
        tb_printf("Erase %x\n", val);
        nrf_nvmc_page_erase(val);
      }
      break;
    case 'I': // Geht nur auf nrf52832 ohne MWU
      i = (strlen(pc) + 3) / 4;
      tb_printf("Write %u(%u) Bytes at %x:'%s'\n", i, strlen(pc), val, pc);
      nrf_nvmc_write_words(val, (uint32_t *)pc, i);
      break;
    case 'J':
      tb_printf("CRC32 %x[%d]: %x\n", val, val2, fs_track_crc32((uint8_t *)val, val2, 0xFFFFFFFF));
      break;
      //==== Memory-Zeugs End  ====

      // === Div Info
    case 'N':
      tb_printf("Novo: %x %x %x %x\n", _tb_novo[0], _tb_novo[1], _tb_novo[2], _tb_novo[3]);
      tb_printf("Button: %u\n", tb_board_button_state(0));
      tb_printf("Idle: %u\n", dbg_idle_cnt);
      fval = get_vbat_aio();
      tb_printf("HK-BAT: %f V\n", fval);
      tb_printf("AD_Intern: %d Cnt\n", get_adintern(false));

      break;
    case '!':
      if (val) { // An P0.0 normalerweise das Quarz
        tb_dbg_pinview(val);
      }
      break;
#endif

    // --intmem--
    case 'U': // U- Internes Memory
      tb_delay_ms(10);
      switch(val){

      case 1:
        res = intpar_mem_write(val2, 0, NULL);
        tb_printf("Intmem1: %d\n", res); // Test - 1 Byte write
        break;
      case 2:
        res = intpar_mem_write(val2,strlen(pc) , pc);
        tb_printf("Intmem2: %d\n", res); // Test - String schreiben
        break;
      case 3:
        res = intpar_mem_read(val2,80 , pc);
        tb_printf("IntmemR: %d\n", res); // Test - 1 Byte write
        for (i = 0; i < res; i++) tb_printf("%d ",*pc++);
        break;
      case 9:
        intpar_mem_erase();
        tb_printf("Erased\n"); 
        break;
      default:
        tb_printf("Param 'U'?\n"); 
        break;
      }
      break;    
    


    // 'T' und 'R' sind als schwerer lesbares Bluetooth-CMD ebenso vorhanden
    case 'T': // ZEIT holen oder setzen (see notes in tb_tools.c)
      if (val) {
        tb_time_set(val);
      }
      val = tb_time_get();
      tb_printf("Time:%u, Runtime:%u\n", val, tb_get_runtime());

      break;

#ifdef ENABLE_BLE
    case 'B': // B0: Bluetooth abschalten, B1: Adv. einschalten - z.B. fuer Debugging
      if (val)
        advertising_start();
      else
        advertising_stop();
      tb_printf("BLE Adv:%d Conn.:%u\n", val, ble_connected_flag);
      break;
#endif

    case 'R':
      tb_putsl("Reset...\n");
      tb_delay_ms(1000);
      tb_system_reset();
      break;

    case 0:
       tb_putsl("OK\n");
      break;

    default: // Type Specific -Command?

      res = device_cmdline(SRC_CMDLINE, input, val);
      if (res == -ERROR_CMD_UNKNOWN)
        tb_putsl("???\n");
      else if (res != 0)
        tb_printf("ERROR: %d\n", res);
    }
    // Measure runtime
    tb_printf("(Run: %u msec)\n", tb_deltaticks_to_ms(t0, tb_get_ticks()));
  } // if (cmdlen)
}
// ---- UART-Service (or UART-ERROR) Wichtig zu pollen, sonst evtl. 500uA Iq---
void cmdline_check(void) {
  int16_t res;
  res = tb_kbhit();
  if (res == -2) {
    tb_init();
    tb_putsl("<UART OK>\n");
    return;
  }
  if (res) {
    GUARD(GID); // GUARD: Save THIS line as last visited line in Module GID
    uart_cmdline();
  }
}

/*** Start Softdevice - Benoetigt von der Toolbox! ***/

/* Function for initializing the nrf log module. */
static void log_init(void) {
  ret_code_t err_code = NRF_LOG_INIT(NULL);
  LOC_ERROR_CHECK(err_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/* Function for initializing power management. */
static void power_management_init(void) {
  ret_code_t err_code;
  err_code = nrf_pwr_mgmt_init();
  LOC_ERROR_CHECK(err_code);
}

/* Function for handling the idle state (main loop).
If there is no pending log operation, then sleep until next the next event occurs. */
static void idle_state_handle(void) {
  if (NRF_LOG_PROCESS() == false) {
    nrf_pwr_mgmt_run();
  }
}


//-------------- NO_BLE_TIMER (simuliert BLE IRQ) ---------------
static void no_ble_timeout_handler(void *p_context) {
  ledflash_flag = 1;
}

APP_TIMER_DEF(no_ble_timer_id);

void simple_no_ble_timer(void) {
  uint32_t ticks = APP_TIMER_TICKS(USE_SIMPLETIMER); // msec
  ret_code_t ret = app_timer_create(&no_ble_timer_id,
      APP_TIMER_MODE_REPEATED,
      no_ble_timeout_handler);
  LOC_ERROR_CHECK(ret);

  ret = app_timer_start(no_ble_timer_id, ticks, NULL);
  LOC_ERROR_CHECK(ret);
}


/***************** M A I N *************************/
void main(void) {

  log_init(); // the nrf_log

  // Get/prepare Reset Reason, not possible after SD started
  reset_reason_bits = (NRF_POWER->RESETREAS);
  (NRF_POWER->RESETREAS) = 0xFFFFFFFF; // Clear with '1'

  power_management_init();

  ble_stack_init(); // Startet Timer! Zuerst starten

  jw_drv_init(); // Systemtreiberm nach SD init.

  // z.B. vor Start des Advertising evtl. HW aktivieren
  system_init();

#ifdef ENABLE_BLE
#ifdef DEBUG
  tb_printf("***** BLUETOOTH ON ****\n");
  tb_printf("Advertising Name: '%s'\n", ble_device_name);
#endif

#ifdef USE_NOTIFICATIONS
  radio_notification_init(); // Ausm Forum - def. in LTX_BLE
#endif

  ble_services_init();
  advertising_start();
#endif


  simple_no_ble_timer();


#ifdef NRF_LOG
  NRF_LOG_INFO("Debug logging RTT started.\n");
#else
  NRF_LOG_INFO("Minimum Debug logging RTT started.\n");
#endif

#ifdef DEBUG
  tb_printf("*** DEBUG - " __DATE__ " "__TIME__
            " ***\n");
  if (tb_is_wd_init())
    tb_putsl("ACHTUNG: WD ist an!\n");
#endif
  device_init(); // Typspezifisch initialisieren

  // Startinfos ausgeben, dann aber sofort Power Saving
  if(tb_kbhit()<0) tb_uninit();

  // Enter main loop.
  for (;;) {

    // periodische Aufgaben?
    GUARD(GID); // GUARD: Save THIS line as last visited line in Module GID

    if (!device_service()) {
#ifdef ENABLE_BLE
      // --- BLE Data Input Scanner (nur wenn connected, aber max. Prio) ---
      if (ble_connected_flag == true) {
        GUARD(GID); // GUARD: Save THIS line as last visited line in Module GID
        ble_periodic_connected_service();
        ledflash_flag = 1; // Connected: in jedem Fall Blinken
      }
#endif
      periodic_secs();
    } // sensor_service

    // -------------- Epilog (spaeter kummulieren auf z.B. 1 Blink/5 sec) -----------
    if (ledflash_flag) {
      tb_board_led_on(0);
      nrf_delay_us(100);
      tb_board_led_off(0);
      ledflash_flag = 0;
    }
#if DEBUG
    dbg_idle_cnt++;
#endif
    idle_state_handle();
  }
}
// ***