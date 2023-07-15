/***********************************************************************
* filepool.c - Diverse allg. Fkt. fuer Files
* Minimalvariante fuer content.x
***********************************************************************/


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "device.h"
#include "allgdef.h"

#include "JesFs.h"  // required for fs_sec1970-conversions!
#include "filepool.h"


#ifndef NO_FS // FileSystem: Is a Logger
#include "internet.h"
  #include "ltx_defs.h"
#include "measure.h"
#endif

#if DEBUG
#include "tb_tools.h"
#endif


//----------------- GLOBALS ---------------
#ifndef NO_FS // Rest for FS operations
// 2 Allgemeine Puffer fuer File-Ops
// ***FOR CARFULLY COMMON USE***
FS_DATE uni_fs_date;    // For human readable dates
FS_DESC uni_fs_desc;    // For file i/o
FS_STAT uni_fs_stat;    // For statistics


//==== JesFs globals =====
// two global descriptors and helpers (global to save RAM memory and allow reuse)
// (JesFs was originally desigend for very small CPUs)

FS_DESC fs_desc;
FS_STAT fs_stat;
#endif

F_INFO f_info; // Haupts. von BLE-Transfer verw. (auch MEMORY)
FS_DATE fs_date; // Structe holding date-time

// Macht Sinn uni_line[] gut gross zu halten, z.B. damit Files schneller verifizierz werden koennen
uint8_t uni_line[UNI_LINE_SIZE]; // *** MULTI USE * UNI_LINE_SIZE 240 by default ***
uint16_t uni_line_cnt;  // Zaehlt die Zeilen

#define UNI_CACHE_SIZE 128    // max 255
static uint8_t _uni_cache[UNI_CACHE_SIZE];
static uint8_t _uni_cache_out; // Ausgangsziger
static uint8_t _uni_cache_len; // Wieviel max. da


char date_buffer[DBUF_SIZE];   // Datebuffer, max. 20 Chars

//=== common helpers === - Noch rausnehmen nach Filepool
// Helper Function for readable timestamps -- Hier optional noch Zeitformate anhand iparam.flags,,,
void conv_secs_to_date_buffer(uint32_t secs) {
    fs_sec1970_to_date(secs, &fs_date);
    if(secs<1000000000){ // max 11k days
      sprintf(date_buffer, "PwrOn+%u.%02u:%02u:%02u", (secs/86400), fs_date.h, fs_date.min, fs_date.sec);
    }else{
      sprintf(date_buffer, "%02u.%02u.%04u %02u:%02u:%02u", fs_date.d, fs_date.m, fs_date.a, fs_date.h, fs_date.min, fs_date.sec);
    }
}

/* Generate a base64-stream, Also useful for external use 
Base64 writes 4 Chars for 3 Input Bytes A B C 
Important: Correct Base64 Format requires Padding with '=' of '==': Len of Base64-String MUST 
be Multiple of 4 (Minimum Output is 2 Chars, hence Padding is '', '=' or '==') */
static const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static uint8_t b64_akku;  
static uint8_t b64_state;  // 3 States: for input Bytes A B C

void b64_init(void){
  b64_state=0;
}
char* b64_addchar(uint8_t neu, char *pc){ 
  switch(b64_state++){
  case 0: // Input A
    *pc++ = b64chars[neu>>2]; // Write A7..A2
    b64_akku = (neu<<4)&63; // keep adjusted A1.A0
    break;
  case 1: // Input B
    *pc++ = b64chars[b64_akku | (neu>>4)]; // Write (A1.A0),B7..B4
    b64_akku = (neu<<2)&63; // keep adjusted B3..B0
    break;
  default: // Input C
    *pc++ = b64chars[b64_akku | (neu>>6)]; // Write (B3..B0),C7.C6
    *pc++ = b64chars[neu&63]; // Write C5..C0
    b64_state=0;
  }
  return pc;
}
char* b64_flush(char *pc){ // Save missing Data, Padding Standard 0
  if(b64_state){
    *pc++ = b64chars[b64_akku];
  }
  return pc;
}


/*****************************************************************************
* fpool_get_hex - get a value in HEX representatin from a pointer in BE Format
* Reads max. max chars, so a Byte 1 is max=2 (and max<=8 of course)
* ppc might be NULL.
* This can be used to store/read binary blocks in Files.
****************************************************************************/
uint32_t fpool_get_hex(char **ppc, uint8_t max){
    char *pc,c,anz=0;
    uint32_t val=0;

    pc=*ppc;
    for(;;){
        c=*pc;
        if(c<32) break;   // End of String detected? STOP
        pc++;
        if((c<'0' || c>'9') && (c<'a' || c>'f') && (c<'A' || c>'F')) {
            if(!anz) continue;  // ignore leading Whitespace or Non-HEX
            else break;         // but exit after the first
        }
        if(c>='a') c-=('a'-10); // kleinbuchstaben hoechste!
        else if(c>='A') c-=('A'-10);
        else c-='0';

        val<<=4;;
        val+=c;
        anz++;
        if(anz==max) break; // limit length
    }
    if(ppc) *ppc=pc;    // Save End-Pointer (if not NULL)
    return val;
}

#ifndef NO_FS
/*******************************************************************************
* Zeichen holen und ggf. Cache fuellen. Binaery safe
* Fuer neues File UNBEDINGT _uni_cache_len auf 0!
*  ret: <-1: Fehler
*  -1: Ende der Datei
*  0..255 Zeichen
*******************************************************************************/
static int16_t fpool_unigetc(void){
    int16_t res;
    if(!_uni_cache_len){ // Nix da: Cache fuellen
        res=fs_read(&uni_fs_desc,_uni_cache,UNI_CACHE_SIZE);
        if(!res) return -1; // FILE END
        if(res<0) return res;   // ERROR
        _uni_cache_len=(uint8_t)res; // Soviel haben wir geholt, mind. 1 Zeichen!
        _uni_cache_out=0;
    }
    _uni_cache_len--;
    return _uni_cache[_uni_cache_out++];
}


/*******************************************************************************
* fpool_uniline() - 1 Zeile aus File lesen und Laenge rueckgeben.
* Zeile kann mit CR oder NL abgeschlossen sein, Fileende ist aber auch OK.
* -1: File-Ende
* sonst >=0: LEN
*******************************************************************************/
int16_t fpool_uni_line(void){
    uint8_t cnt=0;
    int16_t res;
    uint8_t *puni=uni_line;
    for(;;){
        res=fpool_unigetc();
        if(res==-1) {              // -1: Fileende
            if(!cnt) return -1;  // File-Ende OHNE Inhalt
            else break; // Falls noch Inhalt da: Zeile beenden und zurueckgeben
        }
        if(res=='\r') continue; // CR ignorieren
        if(!res || res=='\n') break;    // byte 0 duerfte eigtl. nicht vorkommen..
        *puni++=(uint8_t)res;   // Ansonsten brav eintragen
        cnt++;
        if(cnt==UNI_LINE_SIZE) return -1200;  // Zeile zu lang
    }
    uni_line_cnt++;
    *puni=0;
    return cnt;
}

/********************************************************************************
* Section finden, File solange lesen
* Return:
* <-1: File Error
* -1: Section not found
* 0..32767: OK! Ab nun kommt Section-Zeugs
*******************************************************************************/
int16_t fpool_find_next_section(void){
    int16_t res;
    for(;;){
        res=fpool_uni_line();       // read 1 Line from File
        if(res<0) return res;       // File end
        // 0-Files ignorieren
        if(res>1 && uni_line[0]=='@'){
            return (int16_t)atoi((char*)uni_line+1);    // Return ID of found section
        }
    }
}

/********************************************************************************
* Zahl aus String entnehmen und limitieren
* Grenzen sind inklusive! Wenn fehlt: LOW liefern,
* ansonsten limitierem und evtl. Erg. anmeckern
* pres darf auch NULL sein
*******************************************************************************/
int32_t fpool_config_get_num(int32_t b_low, int32_t b_high, int16_t *pres){
    int16_t res;
    int32_t lres;
    res=fpool_uni_line();       // read 1 Line from File
    if(res<0){
        lres=b_low;
    }else{
        lres=atoi((char*)uni_line);
        if(lres<b_low){
            res=-1401;
            lres=b_low;
        }else if(lres>b_high){
            res=-1402;
            lres=b_high;
        }else res=0;
    }
    if(pres) *pres=res; // Evtl. Result merken
    return lres; // Wert OK
}

/*******************************************************************************
* dto. for FLOAT , No limits checks Default is 0
*******************************************************************************/
float fpool_config_get_float(int16_t *pres){
    int16_t res;
    float fres;
    res=fpool_uni_line();       // read 1 Line from File
    if(res<0){
        fres=0;
    }else{
        fres=atof((char*)uni_line);
        res=0;
    }
    if(pres) *pres=res; // Evtl. Result merken
    return fres; // Wert OK
}

/********************************************************************************
* String lesen aus Zeile. Ende: 0 oder #
* Fehler: -1400
*******************************************************************************/
int16_t fpool_config_get_string(uint8_t *pd, uint8_t maxlen){
    int16_t res;
    uint8_t *ps,c;
    res=fpool_uni_line();       // read 1 Line from File
    if(res<0) return res;
    ps=uni_line;
    while(maxlen--){
        c=*ps++;
        if(c=='#') c=0; // patchen
        *pd++=c;
        if(!c) return 0;    // Alles OK
    }
    return -1400;   // Zu lange!
}

/********************************************************************************
* Open Parameter File Standard Mode as fs_desc_uni
* parameter files must always have a valid CRC32, chich is checked here.
* Else the parameter file be ignored.
*******************************************************************************/
int16_t fpool_par_open(char *fname, uint8_t check_flag){
    int16_t res;

    res=fs_open(&uni_fs_desc,fname,SF_OPEN_READ|SF_OPEN_CRC);
    if(res) return res;

    // Line-Cache init und line-counter init
    _uni_cache_len=0;
    uni_line_cnt=0;

    if(!check_flag) return 0;

    // Test-Read for CRC-Check if required
    for(;;){
        res=fs_read(&uni_fs_desc,uni_line,UNI_LINE_SIZE);
        if(res!=UNI_LINE_SIZE) break;
    }
    if(uni_fs_desc.file_crc32!=fs_get_crc32(&uni_fs_desc)) return -1201;    // CRC Error!
    fs_rewind(&uni_fs_desc); // Rewind for read
    return 0;
}


/********************************************************************************
* Konfigurationsdaten schreiben. 0: OK. Filesystem muss wach sein!
* Filename: sys_param.lxp.
* Spaeter einfach bei Problemen letzte Konfig wieder nehmen oder werkskonfig
*******************************************************************************/
int16_t fpool_write_sys_param(void){
    int16_t res;
    int16_t len;
    res=fs_open(&uni_fs_desc,"sys_param.lxp",SF_OPEN_WRITE|SF_OPEN_CREATE|SF_OPEN_CRC);
    if(res) return res;

    // Header: @200      Type Cookie Name (Cookie: change time if something changed!)
    len=sprintf((char*)uni_line,"@200\n%s\n",server.apn);
    res=fs_write(&uni_fs_desc,uni_line,len);
    if(res) return res;
    len=sprintf((char*)uni_line,"%s\n",server.server);
    res=fs_write(&uni_fs_desc,uni_line,len);
    if(res) return res;
    len=sprintf((char*)uni_line,"%s\n",server.script);
    res=fs_write(&uni_fs_desc,uni_line,len);
    if(res) return res;
    len=sprintf((char*)uni_line,"%s\n%u\n%u\n",server.apikey,server.con_flags,server.pin);
    res=fs_write(&uni_fs_desc,uni_line,len);
    if(res) return res;
    len=sprintf((char*)uni_line,"%s\n",server.user);
    res=fs_write(&uni_fs_desc,uni_line,len);
    if(res) return res;
    len=sprintf((char*)uni_line,"%s\n",server.password);
    res=fs_write(&uni_fs_desc,uni_line,len);
    if(res) return res;
    len=sprintf((char*)uni_line,"%u\n%u\n%u\n%u\n%u\n",server.creg_maxcnt,server.port,server.server_timeout_0,server.server_timeout_run,server.modem_check_reload);
    res=fs_write(&uni_fs_desc,uni_line,len);
    if(res) return res;
    len=sprintf((char*)uni_line,"%u\n%f\n%f\n%u\n\%u\n%u\n",server.batcap,server.battery_0,server.battery_100, server.max_ring, server.messung_energy_cnt, server.mobile_protokoll);
    res=fs_write(&uni_fs_desc,uni_line,len);
    if(res) return res;
    return fs_close(&uni_fs_desc);
}

/********************************************************************************
* Konfigurationsdaten lesen. 0: OK. Filesystem muss wach sein!
* Filename: sys_param.lxp.
* Spaeter einfach bei Problemen letzte Konfig wieder nehmen oder werkskonfig
*******************************************************************************/
int16_t fpool_sys_param_init(uint8_t wflag){
    int16_t res;

    res=fpool_par_open("sys_param.lxp",1); // With check
#if DEVICE_TYP >= 1000
    if(res<0 && wflag){  // Parameter File nicht vorhanden oder beschaedigt?
      // Aus RAM schreiben
      res= fpool_write_sys_param();
      if(res) return res; // Konnte nicht schreiben!
      res=fpool_par_open("sys_param.lxp", 1); // Nochmal With check
    }
#endif

    if(res== -124) return 200; // File not found, using defaults
    if(res) return res; // Other error?

    /* Einfach eine Demo dass es laeuft!
    for(;;){
        res=fpool_uni_line();
        if(res==-1) break;  // ENDE
        if(res<0) return res;
        terminal_printf("[%d]'%s'",res,uni_line);
    }
    */

    res=fpool_find_next_section();    // '@200 = Server'
    if(res!=200) return 201;   // Section not found (not a real error

    res=fpool_config_get_string(server.apn,sizeof(server.apn)-1);   // Brauch noch Platz fuer Null
    if(res) return 202; // APN
    res=fpool_config_get_string(server.server,sizeof(server.server)-1);
    if(res) return 203; // Server
    res=fpool_config_get_string(server.script,sizeof(server.script)-1);
    if(res) return 204; // Script
    res=fpool_config_get_string(server.apikey,sizeof(server.apikey)-1);
    if(res) return 205; // Apikey
    server.con_flags=(uint8_t)fpool_config_get_num(0,255,&res);
    if(res) return 206;    // Cflags
    server.pin=(uint16_t)fpool_config_get_num(0,65535,&res);    // Nun relativ stupide lesen, Ob alles OK kann man (spaeter) mit CRC verifizieren
    if(res) return 207;    //Read PIN
    res=fpool_config_get_string(server.user,sizeof(server.user)-1);
    if(res) return 208; // USER
    res=fpool_config_get_string(server.password,sizeof(server.password)-1);
    if(res) return 209; // Password
    server.creg_maxcnt=(uint8_t)fpool_config_get_num(10,255,&res);
    if(res) return 210;    // Read creg_maxcnt
    server.port=(uint16_t)fpool_config_get_num(1,65535,&res);
    if(res) return 211;    // Port
    server.server_timeout_0=(uint16_t)fpool_config_get_num(1000,65535,&res);
    if(res) return 212;    // Timeout0
    server.server_timeout_run=(uint16_t)fpool_config_get_num(1000,65535,&res);
    if(res) return 213;    // Timeoutx
    server.modem_check_reload=(uint16_t)fpool_config_get_num(60,3600,&res);
    if(res) return 214;    // Modem_check
    server.batcap=(uint32_t)fpool_config_get_num(0,100000,&res);  // 100 Ah maximal
    if(res) return 215;    // Battery Capacity
    server.battery_0=fpool_config_get_float(&res);  // 0% Voltage
    if(res) return 216;   // battery 0
    server.battery_100=fpool_config_get_float(&res);  // 100% Voltage
    if(res) return 217;   // battery 0
    server.max_ring=(uint32_t)fpool_config_get_num(1000,0x7FFFFFFF,&res);  // 2GB max
    if(res) return 218;    // Battery Capacity
    server.messung_energy_cnt =(uint32_t)fpool_config_get_num(0,100000000,&res);  // max. 100sec/1A...
    if(res) return 219;    // Battery Capacity

    server.mobile_protokoll=(uint8_t)fpool_config_get_num(0,255,&res);
    if(res) return 220;    // Illegales Protokoll, neu 2023

    // Mehr bisher nicht def.
    // Close not required for READ
    return 0;   // TEST
}
#endif // NO_FS

// END
