/*******************************************************************************************************************
* drv_vboost.h
*******************************************************************************************************************/

#define VBOOST_DIG_OUT   IX_SCL 
#define VBOOST_PU  IX_X2  // DigPU
#define VBOOST_PUEN  IX_X1  // EnableDigPU
#define HVREF_PIN NRF_SAADC_INPUT_AIN4 // Analoger RefPin


void vboost_init(void);
bool vboost_isrunning(void); // true
void vboost_stop(void);
void vboost_run(uint16_t reps);

//***


