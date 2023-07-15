/***********************************************************************
* filepool.h - Diverse allg. Fkt. fuer Files
* Minimalvariante fuer content.x
***********************************************************************/

/* Fehlermeldungen:
* -1200: Line too long
* -1201: CRC32 of parameter file ERROR - ignored
*
* -1400: String too long
* -1401: Limit LOW
* -1402: Limit HIGH
*
*
* Hints/Warnings
* +200: 'sys_param.lxp' not found  (= using default)
* +201: Section 100 (Server) not found (= using default)
* +202..(akt.)+213: Parameter Fehler  (= using default)

*/

//----------------- GLOBALS ---------------
// Allgemeine Puffer fuer File-Ops
// For common use
// Running FS CMD (verwendet fs_desc)
typedef struct {
    uint8_t cmd; // 0:NIX, "G" oder ..
    uint8_t fname[22];
    uint32_t file_len;
    uint32_t getpos;
    uint32_t getlen;
    // Direkter Speicherzugriff
    uint32_t mem_addr;  
} F_INFO;
extern F_INFO f_info; // Haupts. von BLE-Transfer verw. MAIN

#ifndef NO_FS
extern FS_DESC uni_fs_desc;
extern FS_STAT uni_fs_stat;
extern FS_DATE uni_fs_date;
extern FS_DESC fs_desc; // Fuer MAIN

extern FS_STAT fs_stat; // Fuer MAIN
extern FS_DATE fs_date; // Structe holding date-time 
#endif


#define UNI_LINE_SIZE 240   //  (macht Sinn das gross zu waehlen um z.B. Files zu verifizieren)
extern uint8_t uni_line[UNI_LINE_SIZE];
extern uint16_t uni_line_cnt;  // Zaehlt die Zeilen

// Helper
#define DBUF_SIZE 24
extern char date_buffer[DBUF_SIZE];
void conv_secs_to_date_buffer(uint32_t secs);


int16_t fpool_par_open(char *fname, uint8_t check_flag);
int16_t fpool_uni_line(void);
int16_t fpool_find_next_section(void);
int32_t fpool_config_get_num(int32_t b_low, int32_t b_high, int16_t *pres);
int16_t fpool_config_get_string(uint8_t *pd, uint8_t maxlen);
float fpool_config_get_float(int16_t *pres);

int16_t fpool_write_sys_param(void);
int16_t fpool_sys_param_init(uint8_t wflag); // Read Main Config
uint32_t fpool_get_hex(char **ppc, uint8_t max);    // Read HEX

// Base64-Zeugs
void b64_init(void);
char* b64_addchar(uint8_t neu, char *pc);
char* b64_flush(char* pc);

// END
