/*******************************************************************************
* SAADC.H - Lib fuer LTX
* Misst auch VBat
*******************************************************************************/

// all In One ANALOG;
float get_vbat_aio(void); 
int16_t get_adintern(bool cali_flag); // all In One als 0..

void saadc_init(void);
void saadc_uninit(void);

#ifdef  PIN_MODEM_VINTCHAN
void saadc_setup_vmodem(void);
int16_t saadc_get_vmodem(bool cali_flag);
#endif

void saadc_setup_any_adin(uint32_t ad_pin);
int16_t saadc_get_any_adin(bool cali_flag, int32_t anz_mittel);
int16_t saadc_get_any_adin_simple(void);



// ***