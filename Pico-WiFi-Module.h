/* ============================================================================================================================================================= *\
   Pico-WiFi-Module.h
   St-Louys Andre - September 2024
   astlouys@gmail.com
   Revision 20-OCT-2024

   Include file for Pico-WiFi-Module.c
\* ============================================================================================================================================================= */
#ifndef _WIFI_MODULE_H
#define _WIFI_MODULE_H

#include "lwipopts.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"


/* --------------------------------------------------------------------------------------------------------------------------- *\
                                                          Definitions.
\* --------------------------------------------------------------------------------------------------------------------------- */
#define COUNTRY_CODE CYW43_COUNTRY_CANADA  // determine the WiFi frequencies allocated in each specific country.
#define LED_GPIO             0
#define MAX_NETWORK_RETRIES 10

struct struct_wifi
{
  UCHAR  NetworkName[40];      // must be provided by user's environment variable (see User Guide). SSID (Service Set Identifier)
  UCHAR  NetworkPassword[64];  // must be provided by user's environment variable (see User Guide).
  UINT16 CountryCode;          // must be provided by user (see "#define" above).
  UINT8  FlagHealth;
  UINT32 TotalErrors;          // cumulative number of errors in Wi-Fi connection.
  ip_addr_t PicoIPAddress;
  UCHAR  HostName[sizeof(CYW43_HOST_NAME)];
  UCHAR  ExtraHostName[sizeof(CYW43_HOST_NAME) + 4];
  UINT8  InterfaceMode;        // either CYW43_ITF_STA (station mode) or CYW43_ITF_AP (access point). This module assumes STA mode (client mode, not Access Point).
  UINT8  MacAddress[6];
};



/* --------------------------------------------------------------------------------------------------------------------------- *\
                                                       Functions prototype.
\* --------------------------------------------------------------------------------------------------------------------------- */
/* Log info to log file through Pico's UART or CDC USB. */
extern void log_info(UINT LineNumber, const UCHAR *FunctionName, UCHAR *Format, ...);

/* Pause for specified number of msec. */
static void wait_ms(UINT16 WaitMSec);

/* Blink Pico's LED through CYW43. */
void wifi_blink(UINT16 OnTimeMsec, UINT16 OffTimeMsec, UINT8 Repeat);

/* Initialize Wi-Fi connection. */
INT16 wifi_connect(struct struct_wifi *StructWiFi);

/* Display Wi-Fi information. */
void wifi_display_info(struct struct_wifi *StructWiFi);

/* Initialize the cyw43 on PicoW. */
INT16 wifi_init(struct struct_wifi *StructWiFi);

#endif  // _WIFI_MODULE_H