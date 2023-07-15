/*******************************************************************************
* LTX_BLE.h
*
*******************************************************************************/


// ---Was immer verwendet wird---
#define BLE_DEVICE_NAME_MAXLEN 11                                    // 11: Max. Anz Chars for Fitting in 31-Bytes SRDATA with UUIDs
extern uint8_t ble_device_name[BLE_DEVICE_NAME_MAXLEN + 1];   // "LTX00000000"; // Short MAC-Name

#define MAX_BLE_SESSION_SECS 300  // sec, wie lange Verb. gehalten wird, wird von User-Aktivitaet und PLING gesetzt
extern uint32_t last_ble_cmd_cnt; // TimeoutCounter for Connection (MAX_BLE_SESSION_SECS)
extern bool ble_connected_flag; // True wenn Connected
extern uint16_t ble_rssi_report_cnt; // Counts RSSI Reports to Client
extern bool ble_comm_started_flag; // State of NUS (ready = true)


// Functions
void advertising_change_name(char *newname); 
uint32_t ble_printf(char *fmt, ...);
uint32_t ble_putsl(char *str);
void ble_stack_init(void);
void ble_services_init(void);

uint32_t conn_interval_change(uint32_t newcon_min);
bool ble_isfastspeed(void);
void ble_change2slow(void);

int32_t get_con_rssi(int32_t *p_anz);
void advertising_start(void);
void advertising_stop(void);
uint32_t ble_disconnect(void);

void ble_periodic_connected_service(void);
char* ble_poll_console(void);

//

