/*****************************************************************************
* Logfiles 
* (C)2022 joembedded.de
*
******************************************************************************/
#ifdef DEBUG
 #define MAX_LOG_HISTORY 25000  // 50 kByte max. History Logs 
#else
 #define MAX_LOG_HISTORY 25000  // 50 kByte Summe  max. History Logs
#endif


void log_printf(char* fmt, ...);
void tbble_printf(char *fmt, ...) ;
void isrc_printf(uint8_t isrc, char *fmt, ...);


// END