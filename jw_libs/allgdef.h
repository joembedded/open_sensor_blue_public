// Allgemeine Defs LTX, OSX...

#define SRC_NONE 0 // No Output
#define SRC_CMDLINE 1 // Debug TB_UART
#define SRC_SDI 2 // SDI12
#define SRC_BLE 3 // BLE

// Parameter
#define SYS_PARAM_ID 200
#define IPARAM_ID 201
#define CHANNELS_ID 202


typedef union {   // Umwandlung FLOAT->Binaer (wie satmodem.c)
  uint32_t ulval;
  float fval;
} FXVAL;
// Darstellung einer allgemeinen Fliesskommazahl mit Option zum Fehler:
typedef struct{
  uint16_t errno;	// 0: No ERROR
  float floatval;
} FE_ZAHL;

// ***


