/*******************************************************************************************************************
* drv_usonic.h
*******************************************************************************************************************/

// Die Sounds
#define PLID_ERROR 0
#define PLID_PING 1
#define PLID_ARPDUR 2
#define PLID_USONIC 3 // Nur wenn NRF_PWM_SGEN_BUFFER_LENGTH == 16000
#define PLID_USONIC_SIM 4 // Fuer Simulation Hoerbar


#define USONIC_PIN NRF_GPIO_PIN_MAP(0, 26) // Ultraschall out, Def. LOW

void usonic_init(void);
bool usonic_isrunning(void); // true
void usonic_stop(void);
void usonic_run(uint16_t reps);

void usonic_set_play_seq(int16_t id); // 0..


//***


