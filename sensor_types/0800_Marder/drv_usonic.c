/***
 * Ultrasonic-Driver PWM - (C)JoEmbedded.de
 *
 * per PWM ENGINE fuer quasi-analog...
 *
 ***/

#include <stdbool.h>
#include <stdint.h>
#include <string.h> // memset

#include "tb_tools.h"

#include "nrf_drv_saadc.h" // Pins Analog
#include "nrf_gpio.h"      // Pins Digital
#include "nrfx_pwm.h"

#include "tb_tools.h" // tb_printf()

#include "drv_usonic.h"
#include "osx_pins.h"

#define NRF_PWM_SGEN_PWM_INSTANCE 1
#define NRF_PWM_SGEN_PRIORITY 6

// Countertop kann max. 65 k sein
#define NRF_PWM_SGEN_COUNTERTOP 200 // Bei >=MAX: Nie AN, 0 IMMER AN

//#define NRF_PWM_SGEN_BUFFER_LENGTH 50 // Fuer kurze Bursts, z-B. Distanz
#define NRF_PWM_SGEN_BUFFER_LENGTH 16000 // 200 msec
//#define NRF_PWM_SGEN_BUFFER_LENGTH 1000 // TEST

/* Die nrf_pwm_sequence_t enthaelt:
 * pointer auf uint16[]-PWMs
 * (uint16)anzahl PWMs
 * (uint32)repeatsDesDutyCyles
 * (uint32)delayAmEnde
 */

static nrf_pwm_sequence_t m_pwm_seq;
static uint16_t m_seq_buf[NRF_PWM_SGEN_BUFFER_LENGTH];

/* Mit BaseClock=16Mhz/NRF_PWM_SGEN_COUNTERTOP=200 wird
 * alle 12.5usec ein Element des seq_buf verarbeitet,
 * also mit  [100,100,100,100,0,0,0,0,0,0] und werden 4 Pulse ausgegeben
 */

// NRF_PWM_SGEN_COUNTERTOP: Output 0
// NRF_PWM_SGEN_COUNTERTOP-1: Minimum High-PEAK
// 0: Output Dauer-High
static nrfx_pwm_t m_pwm = NRFX_PWM_INSTANCE(NRF_PWM_SGEN_PWM_INSTANCE); ///< instantiate one PWM peripheral

static bool m_pwm_initialized = false;
static bool m_pwm_playback_in_progress = false;

// --- EventHandler RunningPWM---
static void pwm_handler(nrfx_pwm_evt_type_t evt) {
  switch (evt) {
  case NRFX_PWM_EVT_FINISHED:
  case NRFX_PWM_EVT_STOPPED:
    m_pwm_playback_in_progress = false;
    break;
  }
}

// ---- Pbeendet PWM (vorzeitig, falls noetig) ---
static void nrf_pwm_sgen_destroy(uint8_t gpio_pin) {
  if (m_pwm_initialized) {
    nrfx_pwm_stop(&m_pwm, false);

    nrfx_pwm_uninit(&m_pwm);
    nrf_gpio_cfg_default(gpio_pin);
  }

  m_pwm_playback_in_progress = false;
  m_pwm_initialized = false;
}

// ------init-----
static nrfx_err_t nrf_pwm_sgen_init(uint8_t gpio_pin) {
  nrfx_err_t retCode;

  if (m_pwm_initialized == true)
    nrf_pwm_sgen_destroy(gpio_pin);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  nrfx_pwm_config_t const pwm_config =
      {
          .output_pins = {gpio_pin | NRFX_PWM_PIN_INVERTED,
              NRFX_PWM_PIN_NOT_USED,
              NRFX_PWM_PIN_NOT_USED,
              NRFX_PWM_PIN_NOT_USED},
          .irq_priority = NRF_PWM_SGEN_PRIORITY,
          .base_clock = NRF_PWM_CLK_16MHz, // 125kHz-16Mhz mgl
          .count_mode = NRF_PWM_MODE_UP,
          .top_value = NRF_PWM_SGEN_COUNTERTOP,
          .load_mode = PWM_DECODER_LOAD_Common,
          .step_mode = PWM_DECODER_MODE_RefreshCount};

  retCode = nrfx_pwm_init(&m_pwm, &pwm_config, pwm_handler);
  if (NRFX_SUCCESS != retCode) {
    return retCode;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  m_pwm_seq.values = ((nrf_pwm_values_t)((uint16_t const *)m_seq_buf));
  m_pwm_seq.length = NRF_PWM_SGEN_BUFFER_LENGTH;
  m_pwm_seq.repeats = 0; // Repeat der DutyCycles
  m_pwm_seq.end_delay = 0;

  m_pwm_initialized = true;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  return NRFX_SUCCESS;
}

// ===== Public Functions =====
//#define FSTEP (0.2)  // 0.05:4.0kHz, 0.1:8.0kHz, 0.2:16kHz 0.3125:25kHz 0.33333:26.64kHz
// Intermodulation ist hoerbar, daher Einzeltoene
static void genpwm(uint16_t idx0, uint16_t mlen, float fstep, uint16_t ifin, uint16_t ifout, float famp, float fdelta) {
  uint16_t *px = &m_seq_buf[idx0];
  float fpos = 0.0;
  //tb_printf("F0 %f, ",fstep);

  float fmaxamp = ((NRF_PWM_SGEN_COUNTERTOP) * famp); // 1.0: 100% - no PWM
  for (uint16_t i = 0; i < mlen; i++) {
    uint16_t amp;
    if (fpos >= 0.5)
      amp = 0;
    else {
        uint16_t dend = mlen-i;
        if(i<ifin){ // Ist im Fade-In
          amp = i*fmaxamp/ifin;
        }else if(dend<ifout){
          amp = dend*fmaxamp/ifout;
        }else  amp = fmaxamp;
    }
    *(px++)  = amp;  // Set
    //tb_printf("%d:%d ",i,amp);
    if(px>=&m_seq_buf[NRF_PWM_SGEN_BUFFER_LENGTH]) break;
    fpos += fstep;
    if (fpos >= 1.0) {
      fpos -= 1.0;
      fstep+=fdelta;
    }
  }
  //tb_printf("Fx %f\n",fstep);
}


static int play_id = -1;
void usonic_set_play_seq(int16_t id) {
  if (play_id == id)
    return;
  play_id = id;
  switch (id) {

#if NRF_PWM_SGEN_BUFFER_LENGTH == 16000
  #define RAMP 25
  case PLID_USONIC_SIM:
    genpwm(0, 5334, 0.05, RAMP,RAMP, 0.5, 0); // 4kHz
    genpwm(5334, 5333, 0.075, RAMP,RAMP, 0.5, 0); // 6kHz
    genpwm(10667, 5333, 0.1, RAMP,RAMP, 0.5, 0); // 8kHz
    break;

  case PLID_USONIC:
    genpwm(0, 5334, 0.2, RAMP,RAMP, 1.0, 0); // 16kHz
    genpwm(5334, 5333, 0.25, RAMP,RAMP, 1.0, 0); // 20kHz
    genpwm(10667, 5333, 0.333333, RAMP,RAMP, 1.0, 0); // 26.67kHz
    break;
#endif

  case PLID_ARPDUR: 
    #define ARPVOL (0.5)
    genpwm(0, NRF_PWM_SGEN_BUFFER_LENGTH/4, 0.0125, 1,1, ARPVOL, 0); // 1kHz
    genpwm(NRF_PWM_SGEN_BUFFER_LENGTH/4, NRF_PWM_SGEN_BUFFER_LENGTH/4, 0.01575, 1,1, ARPVOL, 0); //4 Dur
    genpwm((2*NRF_PWM_SGEN_BUFFER_LENGTH)/4, NRF_PWM_SGEN_BUFFER_LENGTH/4, 0.018729, 1,1, ARPVOL, 0); //7 Akkord
    genpwm((3*NRF_PWM_SGEN_BUFFER_LENGTH)/4, NRF_PWM_SGEN_BUFFER_LENGTH/4, 0.0250, 1,NRF_PWM_SGEN_BUFFER_LENGTH/4, ARPVOL, 0); // 2kHz
    break;

  case PLID_PING:
    // Hartes, sauberes Ping 1kHz
    genpwm(0, NRF_PWM_SGEN_BUFFER_LENGTH, 0.05, 1,NRF_PWM_SGEN_BUFFER_LENGTH, 0.8, 0); // 0.05:4kHz ueber alles Ping
    break;

  case PLID_ERROR:
  default:
    genpwm(0, NRF_PWM_SGEN_BUFFER_LENGTH, 0.1, 1,1, 0.5, -0.0002); // 2 kHz down
    break;
  }
}

// Gibt hier 2 Moeglichkeiten: Rechtecksignale mit F/2 und PWM mit F
void usonic_init(void) {
  nrf_pwm_sgen_init(USONIC_PIN);

  /* Kurze Bursts fuer TRX
  for(uint16_t i=0;i<12;i++){
    m_seq_buf[i*2] = 255;     // Maximum fuer 12 Pulse Rechteck
  }
  */
  //usonic_set_play_seq(0);
}

bool usonic_isrunning(void) {
  return (m_pwm_initialized) ? (m_pwm_playback_in_progress) : (false);
}

void _usonic_stop(void) { // Komplette deaktivierung erstmal per _ disabled
  nrf_pwm_sgen_destroy(USONIC_PIN);
  // nrf_gpio_cfg_output(USONIC_PIN);
  // nrf_gpio_pin_clear(USONIC_PIN);
}

void usonic_run(uint16_t reps) { // Max. 65k repeats!
  if (!m_pwm_initialized)
    return;
  m_pwm_playback_in_progress = true;
  nrfx_pwm_simple_playback(&m_pwm, &m_pwm_seq, reps, NRFX_PWM_FLAG_STOP);
}

//***