/*** 
* Boost-Driver PWM - (C)JoEmbedded.de 
*
* Software-Boost-Controler fuer HV
* Realisiert LTSpice 'stepper1.asc'
* 
***/

#include <stdbool.h>
#include <stdint.h>

#include "tb_tools.h"

#include "nrfx_pwm.h"
#include "nrf_gpio.h"   // Pins Digital
#include "nrf_drv_saadc.h" // Pins Analog

#include "osx_pins.h"
#include "drv_vboost.h"

#define NRF_PWM_SGEN_PWM_INSTANCE  0   
#define NRF_PWM_SGEN_PRIORITY      6  

// Countertop kann max. 65 k sein
#define NRF_PWM_SGEN_COUNTERTOP 1   // Bei 1: 0: Off, >=1 On
#define NRF_PWM_SGEN_BUFFER_LENGTH 12 // 96us

/* Die nrf_pwm_sequence_t enthaelt:
* pointer auf uint16[]-PWMs
* (uint16)anzahl PWMs
* (uint32)repeatsDesDutyCyles
* (uint32)delayAmEnde
*/

static nrf_pwm_sequence_t m_pwm_seq;
static uint16_t m_seq_buf[NRF_PWM_SGEN_BUFFER_LENGTH];

/* Mit BaseClock=125kHz/NRF_PWM_SGEN_COUNTERTOP=1 wird 
* alle 8usec ein Element des seq_buf verarbeitet,
* also mit  [1,1,1,0,0,0,0,0,0,0] und PIN_INVERTED wird
* die Folge [0.0.0.1.1.1.1.1.1.1] ausgegeben (Dauer 80 usec)
*/


// NRF_PWM_SGEN_COUNTERTOP: Output 0
// NRF_PWM_SGEN_COUNTERTOP-1: Minimum High-PEAK
// 0: Output Dauer-High
static nrfx_pwm_t m_pwm= NRFX_PWM_INSTANCE(NRF_PWM_SGEN_PWM_INSTANCE);     ///< instantiate one PWM peripheral


static bool m_pwm_initialized = false;
static bool m_pwm_playback_in_progress = false;

// --- EventHandler RunningPWM---
static void pwm_handler(nrfx_pwm_evt_type_t evt)
{
  switch (evt)
  {
    case NRFX_PWM_EVT_FINISHED:
    case NRFX_PWM_EVT_STOPPED:
      m_pwm_playback_in_progress = false;
    break;
  }
}

// ---- Pbeendet PWM (vorzeitig, falls noetig) ---
static void nrf_pwm_sgen_destroy(uint8_t gpio_pin)
{
  if (m_pwm_initialized)
  {
    nrfx_pwm_stop(&m_pwm, false);
    
    nrfx_pwm_uninit(&m_pwm);
    nrf_gpio_cfg_default(gpio_pin);
  }

  m_pwm_playback_in_progress= false;
  m_pwm_initialized= false;
}


// ------init-----
static nrfx_err_t nrf_pwm_sgen_init(uint8_t gpio_pin)
{
  nrfx_err_t retCode;

  if(m_pwm_initialized == true) nrf_pwm_sgen_destroy(gpio_pin);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  nrfx_pwm_config_t const pwm_config= 
  {
    .output_pins  = { gpio_pin | NRFX_PWM_PIN_INVERTED, // 
                      NRFX_PWM_PIN_NOT_USED,
                      NRFX_PWM_PIN_NOT_USED,
                      NRFX_PWM_PIN_NOT_USED },
    .irq_priority = NRF_PWM_SGEN_PRIORITY,
    .base_clock   = NRF_PWM_CLK_125kHz, // NRF_PWM_CLK_16MHz, // 125kHz-16Mhz mgl
    .count_mode   = NRF_PWM_MODE_UP,
    .top_value    = NRF_PWM_SGEN_COUNTERTOP,
    .load_mode    = PWM_DECODER_LOAD_Common,
    .step_mode    = PWM_DECODER_MODE_RefreshCount      
  };

  retCode= nrfx_pwm_init(&m_pwm, &pwm_config, pwm_handler);
  if (NRFX_SUCCESS != retCode) { return retCode; }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  m_pwm_seq.values= ((nrf_pwm_values_t) ((uint16_t const*) m_seq_buf));
  m_pwm_seq.length= NRF_PWM_SGEN_BUFFER_LENGTH;
  m_pwm_seq.repeats= 0; // Repeat der DutyCycles
  m_pwm_seq.end_delay= 0;

  m_pwm_initialized= true;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  return NRFX_SUCCESS;
}


// ===== Public Functions =====
void vboost_init(void){
  nrf_pwm_sgen_init(VBOOST_DIG_OUT);
  
  nrf_gpio_cfg_output(VBOOST_PU); // Pullup 
  nrf_gpio_pin_clear(VBOOST_PU); // Off
  nrf_gpio_cfg_output(VBOOST_PUEN); // PullupEnable 
  nrf_gpio_pin_clear(VBOOST_PUEN); // Off


  m_seq_buf[0] = 1; // 8 usec per Tick
  m_seq_buf[1] = 1;
  m_seq_buf[2] = 1;
  m_seq_buf[3] = 1;
  m_seq_buf[4] = 1;
  // Rest is 0
}

bool vboost_isrunning(void)
{
  return (m_pwm_initialized)?(m_pwm_playback_in_progress):(false);
}

void vboost_stop(void){ // Komplette deaktivierung 
  nrf_gpio_pin_clear(VBOOST_PU); // OFF
  nrf_gpio_pin_clear(VBOOST_PUEN); // Off
  // nrf_pwm_sgen_destroy(VBOOST_DIG_OUT); - Destroy nicht noetig nur Treiber stoppen
}

void vboost_run(uint16_t reps){   // Max. 65k repeats!
  if (!m_pwm_initialized) return; 
  nrf_gpio_pin_set(VBOOST_PU); // ON
  nrf_gpio_pin_set(VBOOST_PUEN); // On
  m_pwm_playback_in_progress= true;
  nrfx_pwm_simple_playback(&m_pwm, &m_pwm_seq, reps, NRFX_PWM_FLAG_STOP); 
}

//***

