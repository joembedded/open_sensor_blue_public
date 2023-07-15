/****************************************************
* osx_main.h - Sensor Cmdline
*
***************************************************/


// Allg. Errors (def positive) - wie ltx_defs.h for Logger. Manche auch in sdi12_drv (nochmal) def....
#define ERROR_NO_REPLY_LTX        2 

#define ERROR_CMD_UNKNOWN             256 // Komando unbekannt und wurde nicht verarbeitet (auch in Kommandozeile)
#define ERROR_NOT_INIT                257 // Haupts. std_uart0 oder SDI12
#define ERROR_TOO_SHORT_DATA          270 // SDI12,HC2, Input
#define ERROR_NOT_READY               271 // HC2: --.--
#define ERROR_ILLEGAL_CMD             272 // div.
#define ERROR_SENSOR_NO_DISK          273 // Eigl. NUR fuer OSX Sensoren
#define ERROR_MEMORY_SECTOR           274 // Nur MEMORY Direct
#define ERROR_DRV_BUF                 282 // Sizeof Driver Buffers


#define VMON_CHANNEL            NRF_SAADC_INPUT_AIN7
#define HK_VBAT_KOEFF           0.00346 // Div: 4M7-1M6 (0 - 14.2V)

void device_init(void);
int16_t device_cmdline(uint8_t isrc, uint8_t *pc, uint32_t val);
bool device_service(void); // Ret: false: Kein Periodic
bool device_periodic(void); // Ret: true: Zeige stat '.*o'..
void system_init(void);
void type_cmdprint_line(uint8_t isrc, char *pc);

// === Parameter ===
#define ID_INTMEM_MODEFLAGS 98 // Unterhalb 100: Systemparameter
#define ID_INTMEM_ADVNAME 99 // Unterhalb 100: Systemparameter
#define ID_INTMEM_USER0 100 // Ab hier freu fuer Sensor


//=== in xxxx_sensor_xxx.c: ===
extern uint16_t mode_flags; // Fuer schnelles Setup per 'f', Init mit 15 nach Reset

void sensor_init(void);
int16_t sensor_measure(uint8_t isrc);
int16_t sensor_ble_v_data(char *pc); // Neu 2/23

#ifdef HAS_DEVICE_TYPE_CMDLINE
    int16_t device_type_cmdline(uint8_t isrc, uint8_t *pc);
#endif
#if DEBUG
bool debug_tb_cmdline(uint8_t *pc, uint32_t val);
#endif

//***