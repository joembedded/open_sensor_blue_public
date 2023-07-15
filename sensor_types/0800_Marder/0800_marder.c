/****************************************************
 * 0800_marder.c - MarderDriver
 * DEVICE_TYP 800
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

#include "app_error.h"
#include "nrfx_gpiote.h"
#include "nrfx_ppi.h" // 4 include fuer PPI
#include "nrfx_timer.h"

#include "device.h"
#include "filepool.h" // uni_line
#include "saadc.h"
#include "tb_tools.h"

#include "osx_main.h"
#include "osx_pins.h"

#include "intmem.h"

#if DEBUG
#include "nrf_delay.h"
//#include "i2c.h"
#include "nrfx_spim.h"
#endif

#include "drv_usonic.h"
#include "drv_vboost.h"

#include "osx_pins.h"

#include "nrf_drv_saadc.h" // Pins Analog

#if DEVICE_TYP != 800
#error "Wrong DEVICE_TYP, select other Source in AplicSensor"
#endif

// Analog
//#include "nrf_adc_blk.h"
//#include "nrf_pwm_sgen.h"

int16_t param; // Erstmal noch DUMMY

uint16_t alen = 1;

// Additional Test-CMDs via TB_UART
// IMPORTANT: only SMALL capitals for User-CMDs!
int16_t get_hvolt(int anz) {
  saadc_init();
  saadc_setup_any_adin(HVREF_PIN);
  int16_t hvcnt = saadc_get_any_adin(true, anz); // mit Cali und 1 Sample
  saadc_uninit();
  return hvcnt;
}
#define CNT2VOLT (0.19273) // 1GOhm-4.7MOhm-Teiler
void show_hvolt(void) {
  int16_t hvcnt = get_hvolt(1); // Schnelle Mesung
  tb_printf("HV(1) CNT:%d (%.1f V)\n", hvcnt, (float)(hvcnt)*CNT2VOLT);
}

#if DEBUG
// === DEBUG START ===

//====== TEST COMMANDS FOR NEW SENSOR START_A ========
//====== TEST COMMANDS FOR NEW SENSOR END_A ========
bool debug_tb_cmdline(uint8_t *pc, uint32_t val) {
  int16_t res;
  switch (*pc) {
  case 'S': // Often useful
    // ltx_i2c_scan(val, 7); // 0:W,1:R  NoPU */
    break;
    //====== TEST COMMANDS FOR NEW SENSOR START_S ========

    /* Aus Sicherheitsgruenden gesperrt! */
  case 'a':
    if (val)
      alen = val;
    tb_printf("PWM-Out-Test %d Rep\n", alen);
    vboost_run(alen);
    // break; // Fall Through to V
    while (vboost_isrunning())
      tb_delay_ms(1);
    vboost_stop();

    tb_delay_ms(500); // Sofort und 1 sec spaeter
    show_hvolt();
    tb_delay_ms(1000);
    show_hvolt();

    break;

  case 'h':
    show_hvolt();
    break;

  case 'f': // ultrasonic
    if (val)
      alen = val;
    tb_printf("USONIC %d\n", alen);
    usonic_run(alen);
    while (usonic_isrunning())
      tb_delay_ms(1);
    break;

  case 'p': // ultrasonic - dauer
    while (val > 0) {
      tb_printf("Ping %d\n", val--);
      usonic_run(1);
      tb_delay_ms(1000);
    }
    break;

  case 's': // Sequence
    tb_printf("Seq: %d\n", val);

    usonic_set_play_seq(val);
    usonic_run(1);
    while (usonic_isrunning())
      tb_delay_ms(1);
    break;

  //====== TEST COMMANDS FOR NEW SENSOR END_S ========
  default:
    return false;
  }
  return true; // Command processed
}
// === DEBUG END
#endif

// --- BLE-Daten XXX erzeugen Aufbau: Vx:vals...
// Wenn nicht vorh: 0 zurueckgeben.
int16_t sensor_ble_v_data(char *pc) {
  return 0; // NotUsed
}

void play_sound_wait(int16_t soundno, uint16_t reps) {
  usonic_set_play_seq(soundno);
  usonic_run(reps); // 3 * Play
  while (usonic_isrunning())
    tb_delay_ms(1);
}

void sensor_init(void) {
  intpar_mem_read(ID_INTMEM_USER0, sizeof(param), (uint8_t *)&param);
  vboost_init();
  usonic_init();

  // Als Hallo 3 * ARPDUR
  play_sound_wait(PLID_ARPDUR, 3);
}

// Wenn true als return war Wakeup vom Sensor, BLE scannen nicht notwendig (falls connected)
// bei false auch noch BLE scannen (falls connected)
bool device_service(void) {
  int16_t res;
  return false; // Periodic Service OK
}
// Hier der echte Service
// Called max. 1/sec - Device bedienen fuer periodische Aufgaben

// ------ HighVoltage Steuerung -------------
#define VBAT_LOW 9.0   // darunter Batterie schwach!
#define VBAT_HIGH 14.0 // drueber stimmt irgendwas nicht

#define VLIM_SHORT 50.0    // Drunter zaehlts als Kurzschluss
#define VLIM_HIT 200.0     // Drunter Annahme: Kontakt ausgeloest
#define VLIM_REFRESH 350.0 // runter Refresh
#define VLIM_MAX 400.0      // Ziel-Spannung
#define MAX_CYCLES 1700    // Mehr niemals, SIcherheitsgrenze

uint32_t next_hvgen = 5; // runtime ('nsecs') fuer naeches HV Gen. Sekunden

#define HVSTAT_SHORTCIRCUIT 1 // Aktuell Kurzschluss!
#define HVSTAT_HIT 2          // Wurde ausgeloest
#define HVSTAT_HVGENERATED 3  // Es wurde HV generiert, sollte also vorhanden sein

#define INTERVALL_HV_SHORT 10 // Nach Kurzschluss so lange warten
#define INTERVALL_HV_HIT 2    // Nac Hit solange Pause

#define JPC_X1 0.0214186e-3 // J per Cycle (abh, von VBAT)
#define JPC_X0 -0.12572964e-3

#define KAPH ((2.2e-6) / 2) // Halbe Kapazitaet, 2.2uF, Energie E = (C/2)*U*U

uint8_t hvstat = 0; // Nix

// --- Statistik -----
uint32_t hit_cnt=0;   // Zaehlt Ausloesungen
uint32_t shortc_cnt=0;  // Zaehlt Kurzschluesse

uint32_t last_charge_t0 = 0;  // intern
uint32_t last_delta_charge_t = 0; // Letzte Periode zwischen 2 Charges
float last_delta_energy;

// --- Ultrasonic Timing ---------
#define INTERVALL_USONIC 10 // alle x Sekunden
uint32_t next_usonic = INTERVALL_USONIC;

bool device_periodic(void) { // Ret: true: Zeige stat '.*o'.., sonst nix

  float hvolt = get_hvolt(1) * CNT2VOLT; // Schnelle Messung alle Sekunde
  if (tb_is_uart_init())
    tb_printf("HV %.1f V\n", hvolt); // For UART-Debug

  if (hvstat >= HVSTAT_HVGENERATED) { // Spannung sollte nun vorhanden sein
    if (hvolt < VLIM_SHORT) {         // Achtung, Kurzschluss?
      hvstat = HVSTAT_SHORTCIRCUIT;
      play_sound_wait(PLID_ERROR, 1);
      shortc_cnt++; // Zaehlt Schorts
      next_hvgen = now_runtime + INTERVALL_HV_SHORT;
    } else if (hvolt < VLIM_HIT) { // Marder-Kontakt?
      hvstat = HVSTAT_HIT;
      play_sound_wait(PLID_PING, 1);
      hit_cnt++; // Zaehlt Hits
      next_hvgen = now_runtime + INTERVALL_HV_HIT;
    }
  } else if (hvolt > VLIM_REFRESH) {
    next_hvgen = now_runtime; // Diese Runde HV noch OK
  }

  //return true;  // enable for ***MinimumPowerTest***

  if (now_runtime > next_hvgen || now_runtime > next_usonic) {
    float vbat = get_vbat_aio();
    if (vbat < VBAT_LOW || vbat > VBAT_HIGH)
      return true; // Batterie schwach oder 'komisch', Betrieb einstellen

    // ---HighVoltage Service---
    if ((now_runtime > next_hvgen) && (mode_flags&1)) {
      if (hvolt < VLIM_REFRESH) { // Nachladen noetig
        float jpc = JPC_X1 * vbat + JPC_X0;
        float denergy = (((VLIM_MAX * VLIM_MAX) - (hvolt * hvolt)) * KAPH); // Soviel Energy wird benoetigt
        int32_t anzn = (int32_t)( denergy / jpc); // soviele Zyklen werden benoetigt

        if (anzn > MAX_CYCLES)
          anzn = MAX_CYCLES;
        if (tb_is_uart_init()) tb_printf("ChargeCycles: %d\n", anzn);
        if (anzn > 0) {
          vboost_run(anzn);

           last_delta_energy = denergy;
           last_delta_charge_t = (now_runtime - last_charge_t0); // Track Energy
           last_charge_t0 = now_runtime;

          while (vboost_isrunning())
            tb_delay_ms(1);
          vboost_stop();
          hvstat = HVSTAT_HVGENERATED;
        }
      }
    }
    
    // ---Ultrasonic Service---
    if ((now_runtime > next_usonic)  && (mode_flags&2)) {
#if DEBUG
      play_sound_wait(PLID_USONIC_SIM, 1);  // Hoerbar und leise
#else
      play_sound_wait(PLID_USONIC, 1);  // >18kHz und laut
#endif
      next_usonic = now_runtime + INTERVALL_USONIC;
    }
  }

  return true;
}

int16_t sensor_measure(uint8_t isrc) {
  sprintf(uni_line, "~e:%u %u", 0, 1000);
  type_cmdprint_line(isrc, uni_line);


  int16_t hvcnt = get_hvolt(16); // Langsame Mesung

  sprintf(uni_line, "~#0: HighVoltage: %.1f V", (float)(hvcnt)*CNT2VOLT);
  type_cmdprint_line(isrc, uni_line);

  // Abhaengig von VLIM_ ... Zahlen fuer Loss grob 
  char *pc = uni_line+sprintf(uni_line, "~#1: Loss: ");
  if(last_delta_charge_t) {
    float eps = ((last_delta_energy*1000.0)/(float)last_delta_charge_t);
    pc+=sprintf(pc," %.2f mJ/sec (",eps);
    if(eps<0.25) pc+=sprintf(pc,"good)");    // 1Gohm: 0.16 - optimal
    else if(eps<2.5) pc+=sprintf(pc,"ok)");   // 0.1GOhm: 1.6 - ok 
    else if(eps<25.0) pc+=sprintf(pc,"poor)"); // ca 15: Voltcraft Multimeter
    else pc+=sprintf(pc,"bad)"); //  1MOhm, ca. 78
  } else pc+= sprintf(pc, "(unknown)");
  type_cmdprint_line(isrc, uni_line);

  sprintf(uni_line, "~#2: Contacts: %d Cnt", hit_cnt);
  type_cmdprint_line(isrc, uni_line);
  sprintf(uni_line, "~#3: ShortCircuts: %d Cnt", shortc_cnt);
  type_cmdprint_line(isrc, uni_line);

  float vbat = get_vbat_aio();
  float premain = (vbat-VBAT_LOW)*33.333;
  if(premain<0.0) premain=0.0;
  else if(premain>100.0) premain=100.0;
  sprintf(uni_line, "~H90: BatV: %.3f V(Bat, %.1f%% full)", vbat,premain);
  type_cmdprint_line(isrc, uni_line);
  int32_t tint;
  sd_temp_get(&tint);
  float sys_temp = (float)tint * 0.25; // InternCPU
    sprintf(uni_line, "~H91: Temp: %.1f "
                        "\xc2\xb0"
                        "C(int)", sys_temp);
    type_cmdprint_line(isrc, uni_line);
    sprintf(uni_line, "~h:%u\n", 0); // Reset, Alarm, alter Alarm, Messwert, ..
    type_cmdprint_line(isrc, uni_line);

    sprintf(uni_line, "~H Mode: HighVoltage:%s, UltraSonic:%s\n",(mode_flags&1)?"On":"Off",(mode_flags&2)?"On":"Off");
    type_cmdprint_line(isrc, uni_line);
    

  return 0;
}

//***