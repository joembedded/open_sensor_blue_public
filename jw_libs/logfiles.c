/*****************************************************************************
* Logfiles 
* (C)2020 joembedded.de
*
* Schreibt File 'logfile.txt' auf Disk
*
******************************************************************************/

#include <stdarg.h> // for var_args
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "device.h"
#include "allgdef.h"
#include "jesfs.h"

#ifndef NO_FS // FileSystem: Is a Logger
#include "internet.h"
#endif

#include "logfiles.h"
#include "ltx_ble.h"
#include "tb_tools.h"
#include "osx_main.h"

#define LOG_SIZE_LINE_OUT 120 // 1 terminal line working buffer (opt. long filenames to print)
static char log_line_out[LOG_SIZE_LINE_OUT];
#ifndef NO_FS
static FS_DESC fs_log_desc;
#endif
// ---opt. Log To Disk---
static void log2tb_or_ble(char *pc) {
  tb_putsl(pc);
#ifdef ENABLE_BLE
  ble_putsl(pc);
#endif
}

#ifndef NO_FS
// ---opt. Log To Disk---
static void log2file(size_t ulen) {
  int16_t res;
  FS_DESC fs_log_desc_test;
  // Opt. Write to File, ignore unimportant IOs
  if (!(server.con_flags & (CF_LOG_FILE)))
    return;

  if (sflash_info.state_flags & STATE_DEEPSLEEP_OR_POWERFAIL) {
    res = fs_start(FS_START_FAST | FS_START_RESTART);
    if (res) {
      sprintf(log_line_out,"ERROR: Log Start FS(%d)\n", res);
      log2tb_or_ble(log_line_out);
      return;
    }
  }

  // Test if Logfile still exists wit _test?
  res = fs_open(&fs_log_desc_test, "logfile.txt", SF_OPEN_READ);
  if (res) { // Not existing: Re-Open for Raw
    log2tb_or_ble("Start Log\n");
    fs_log_desc.open_flags = 0;
  }

  if (!fs_log_desc.open_flags) {
    res = fs_open(&fs_log_desc, "logfile.txt", SF_OPEN_CREATE | SF_OPEN_RAW);
    if (res) {
      sprintf(log_line_out,"ERROR: Logfile fs_open:%d\n", res);
      log2tb_or_ble(log_line_out);
      return;
    }
    fs_read(&fs_log_desc, NULL, 0xFFFFFFFF); // (dummy) read as much as possible;
  }
  res = fs_write(&fs_log_desc, log_line_out, ulen);
  if (res) {
    sprintf(log_line_out,"ERROR: Logfile fs_write:%d\n", res);
    log2tb_or_ble(log_line_out);
    return;
  }
  // Now make a file shift if more data than HISTORY
  if (fs_log_desc.file_len >= MAX_LOG_HISTORY) {
    log2tb_or_ble("Shift 'logfile.txt'->'logfile.txt.old'\n");

    // Optionally delete and (create in any case) backup file
    res = fs_open(&fs_log_desc_test, "logfile.txt.old", SF_OPEN_CREATE);
    if (res) {
      sprintf(log_line_out,"ERROR: 'log..old' fs_open:%d\n", res);
      log2tb_or_ble(log_line_out);
      return;
    }

    fs_log_desc.open_flags = 0;
    // rename (full) data file to secondary file
    res = fs_rename(&fs_log_desc, &fs_log_desc_test);
    if (res) {
      sprintf(log_line_out,"ERROR: 'log..old' fs_rename:%d\n", res);
      log2tb_or_ble(log_line_out);
      return;
    }
  }
}

// log_printf(): printf() to FILE und/oder tb_uart / BLE
// Start-Char '$' NOT to File!
void log_printf(char *fmt, ...) {
  size_t ulen;
  va_list argptr;

  if (!(server.con_flags & (CF_LOG_FILE)) && (tb_is_uart_init() == false) 
#ifdef ENABLE_BLE
    && (ble_connected_flag == false)
#endif
  )
    return;

  va_start(argptr, fmt);
  ulen = vsnprintf(log_line_out, LOG_SIZE_LINE_OUT, fmt, argptr); // vsn: limit!
  va_end(argptr);
  // vsnprintf() limits output to given size, but might return more.
  if (ulen > LOG_SIZE_LINE_OUT - 1)
    ulen = LOG_SIZE_LINE_OUT - 1;
  if (!ulen)
    return;

  log2tb_or_ble(log_line_out);
  log2file(ulen);
}
#endif

// Ausgabe auf speziellem Kanal tb_uart oder BLE
// Muss man nix pruefen, da immer speziell angefragt
void isrc_printf(uint8_t isrc, char *fmt, ...) {
  size_t ulen;
  va_list argptr;

  va_start(argptr, fmt);
  ulen = vsnprintf(log_line_out, LOG_SIZE_LINE_OUT, fmt, argptr); // vsn: limit!
  va_end(argptr);
  // vsnprintf() limits output to given size, but might return more.
  if (ulen > LOG_SIZE_LINE_OUT - 1)
    ulen = LOG_SIZE_LINE_OUT - 1;
  if (ulen){
  // Nur auf bekannten Kanaelen
    if(isrc==SRC_CMDLINE) tb_putsl(log_line_out);
#ifdef ENABLE_BLE
    else if(isrc==SRC_BLE)  ble_putsl(log_line_out);
#endif
   }
}

// Ausgabe auf speziellem Kanal tb_uart UND BLE
void tbble_printf(char *fmt, ...) {
  size_t ulen;
  va_list argptr;

  if ((tb_is_uart_init() == false) 
#ifdef ENABLE_BLE
    && (ble_connected_flag == false)
#endif
  )
  return;

  va_start(argptr, fmt);
  ulen = vsnprintf(log_line_out, LOG_SIZE_LINE_OUT, fmt, argptr); // vsn: limit!
  va_end(argptr);
  // vsnprintf() limits output to given size, but might return more.
  if (ulen > LOG_SIZE_LINE_OUT - 1)
    ulen = LOG_SIZE_LINE_OUT - 1;
  if (ulen){
  // Nur auf bekannten Kanaelen
  tb_putsl(log_line_out);
#ifdef ENABLE_BLE
   ble_putsl(log_line_out);
#endif
  }
}


// END