/*******************************************************************************
 * SAADC.C - Lib fuer LTX
 * Measure VBAT - V1.1 (C) JoEmbedded.de
 *******************************************************************************/

#include "nrf.h"
#include "nrf_drv_saadc.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "boards.h"
#include "nrf_delay.h"

// JW Toolbox
#include "osx_main.h"
#include "osx_pins.h"
// Device.h NACH osx_pins
#include "device.h"
#include "saadc.h"
//#include "sdi12sensor_drv.h"
#include "tb_tools.h"

// Pin und Setup extern

// Kanal 0 immer HK_VMON
// (more channels optional)

static volatile bool saadc_cali_fertig_flag;
static volatile bool saadc_init_flag = false;

// Non-Blocking Wandeln loest Event NRF_DRV_SAADC_EVT_DONE aus,
// Kalibrieren:  NRFX_SAADC_EVT_CALIBRATEDONE ..DONE, ..RESULTDONE
static void saadc_callback(nrf_drv_saadc_evt_t const *p_event) {
  if (p_event->type == NRFX_SAADC_EVT_CALIBRATEDONE) {
    // NRF_LOG_INFO("Kalibrieren fertig\n");
    saadc_cali_fertig_flag = true;
  }
}

void saadc_init(void) {
  ret_code_t err_code;
  if (saadc_init_flag)
    return;
  err_code = nrf_drv_saadc_init(NULL, saadc_callback); // NRFX_SAADC_CONFIG_RESOLUTION, etc in Config
  APP_ERROR_CHECK(err_code);
  saadc_init_flag = true;
}
void saadc_uninit(void) {
  nrf_drv_saadc_uninit();
  saadc_init_flag = false;
}

static void saadc_setup_vmon(void) { //  Call after saadc_init(), Res. per Def. 12 Bit aus sdk_config.h
  ret_code_t err_code;
  // Std-Einstellungen sind OK (10uS, Gain 1_6, Int.Ref) Single-Ended. Voller Bereich fuer 3.6V 4096 Cnts.
  nrf_saadc_channel_config_t channel_config0 = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(VMON_CHANNEL); //
  channel_config0.acq_time = NRF_SAADC_ACQTIME_40US;
  err_code = nrf_drv_saadc_channel_init(0, &channel_config0);
  APP_ERROR_CHECK(err_code);
}

static void saadc_setup_intern(void) { //  Call after saadc_init(), Res. per Def. 12 Bit aus sdk_config.h
  ret_code_t err_code;
  nrf_saadc_channel_config_t channel_config0 = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD); //
  channel_config0.acq_time = NRF_SAADC_ACQTIME_10US;
  err_code = nrf_drv_saadc_channel_init(0, &channel_config0);
  APP_ERROR_CHECK(err_code);
}

// Empfohlen alle paar Grad Temperaturschwankung
static void saadc_calibrate_offset(void) {
  ret_code_t err_code;
  saadc_cali_fertig_flag = false;
  err_code = nrf_drv_saadc_calibrate_offset();
  APP_ERROR_CHECK(err_code);
  while (saadc_cali_fertig_flag == false)
    ; // bissl warten bis fertig... (wenn adc nicht init: forever)
}

int16_t get_adintern(bool cali_flag) { // all In One als 0..4095(=3.6V), 2500:2.19V 2275:2V (dauert ca. 50usec)
  int16_t adcnt;
  saadc_init();
  saadc_setup_intern();
  if (cali_flag) {
    saadc_calibrate_offset();
    nrf_drv_saadc_sample_convert(0, &adcnt); // Ignore First!
  }
  nrf_drv_saadc_sample_convert(0, &adcnt);
  saadc_uninit();
  return adcnt;
}

//----------------- HK_VBAT------------------------------
// Analoge HK_VBAT Spannung holen - SAADC MUSS bereits initialisiert sein
// Z.B. fuer andere Anzahl Mittelungen
#ifdef HK_VBAT_KOEFF // Nur wenn definiert Batterie ueber ext. ADIN 'VMON' holen
static float saadc_get_vbat(bool cali_flag, int32_t anz_mittel) {
  if (cali_flag)
    saadc_calibrate_offset();
  int16_t value;
  int32_t sum = 0;
  nrf_drv_saadc_sample_convert(0, &value); // Ignore First!
  for (uint32_t i = 0; i < anz_mittel; i++) {
    nrf_drv_saadc_sample_convert(0, &value); // Channel, pWert
    sum += value;
  }
  sum /= anz_mittel;
  return ((float)sum) * HK_VBAT_KOEFF;
}
#endif

float get_vbat_aio(void) { // all In One, unkalibriert ist OK
  float fval;
#ifdef HK_VBAT_KOEFF
  saadc_init();
  saadc_setup_vmon();
  fval = saadc_get_vbat(true, 8); // Calibrate and 8 Averages
  saadc_uninit();
#else
  fval = get_adintern(true) * 0.000879; // 3.600V/4096Cnts
#endif
  return fval;
}

#ifdef PIN_MODEM_VINTCHAN
// ---------------  Modem-Supply holen ---------------
void saadc_setup_vmodem(void) { // ID unused. Call after saadc_init(), Res. per Def. 12 Bit aus sdk_config.h
  ret_code_t err_code;
  nrf_saadc_channel_config_t channel_config1 = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(PIN_MODEM_VINTCHAN); //
  channel_config1.acq_time = NRF_SAADC_ACQTIME_10US;
  err_code = nrf_drv_saadc_channel_init(0, &channel_config1);
  APP_ERROR_CHECK(err_code);
}
int16_t saadc_get_vmodem(bool cali_flag) {
  if (cali_flag)
    saadc_calibrate_offset();
  int16_t value;
  nrf_drv_saadc_sample_convert(0, &value); // Channel, pWert
  return value;
}
#endif

// Fuer beliebigen PIN:  - vorher   saadc_init(); setup_adin(), ..get()... uninit()
void saadc_setup_any_adin(uint32_t ad_pin) {
  ret_code_t err_code;
  nrf_saadc_channel_config_t channel_config2 = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(ad_pin); //
  channel_config2.acq_time = NRF_SAADC_ACQTIME_40US;
  err_code = nrf_drv_saadc_channel_init(0, &channel_config2);
  APP_ERROR_CHECK(err_code);
}
int16_t saadc_get_any_adin(bool cali_flag, int32_t anz_mittel) {
  if (cali_flag)
    saadc_calibrate_offset();
  int16_t value;
  int32_t sum = 0;
  nrf_drv_saadc_sample_convert(0, &value); // Ignore First!
  for (uint32_t i = 0; i < anz_mittel; i++) {
    nrf_drv_saadc_sample_convert(0, &value); // Channel, pWert
    sum += value;
  }
  sum /= anz_mittel;
  return (int16_t)sum;
}
int16_t saadc_get_any_adin_simple(void) { // 2. Version so schnell wie moeglich und ohne Cali
  int16_t value;
   nrf_drv_saadc_sample_convert(0, &value); // Channel, pWert
  return value;
}

//***