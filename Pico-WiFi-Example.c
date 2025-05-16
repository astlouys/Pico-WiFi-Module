/*
* Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/* ============================================================================================================================================================= *\
   Pico-WiFi-Example.c
   St-Louys Andre - October 2024
   astlouys@gmail.com
   Revision 06-JAN-2025
   Langage: C with arm-none-eabi
   Version 2.02

   Raspberry Pi Pico Firmware to test the Pico-WiFi-Driver library.
   This firmware doesn't do much useful thing, but it shows how to implement
   the Pico-WiFi-Driver library in your own code in order to access your wi-fi network.


   NOTE:
   THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
   WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
   TIME. AS A RESULT, THE AUTHOR SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
   INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM
   THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
   INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS. 


   REVISION HISTORY:
   =================
   03-OCT-2024 1.00 - Original release from Raspberry Pi Ltd..
   03-OCT-2024 2.00 - Adapted by Andre St-Louys as an "add-in module" to facilitate the addition of Wi-Fi to an existing project.
   06-JAN-2025 2.01 - Add a way to monitor Wi-Fi network health and implement a callback to do it.
                    - Other minor and cosmetic changes.
   14-MAY-2025 2.02 - Cleanup, cosmetic and optimization changes.
\* ============================================================================================================================================================= */


/* $PAGE */
/* $TITLE=Include files. */
/* ============================================================================================================================================================= *\
                                                                          Include files
\* ============================================================================================================================================================= */
#include "baseline.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "Pico-WiFi-Module.h"
#include "ping.h"
#include "stdarg.h"
#include <stdio.h>



/* $TITLE=Definitions and macros. */
/* $PAGE */
/* ============================================================================================================================================================= *\
                                                                       Definitions and macros.
\* ============================================================================================================================================================= */
#define MAX_NETWORKS  200                  // maximum number of Access Points for allocated memory.
#define PING_ADDRESS  "192.168.0.2"



/* $TITLE=Global variables declaration / definition. */
/* $PAGE */
/* ============================================================================================================================================================= *\
                                                             Global variables declaration / definition.
\* ============================================================================================================================================================= */
UINT8 FlagLogon;
UINT8 APNumber;

struct
{
  INT8  SignalStrength;
  UINT8 Channel;
  UINT8 Security;
  UCHAR MacAddress[6];
  UCHAR NetworkName[40];
}WlanFound[MAX_NETWORKS];

struct repeating_timer Handle5SecTimer;



/* $TITLE=Function prototypes. */
/* $PAGE */
/* ============================================================================================================================================================= *\
                                                                     Function prototypes.
\* ============================================================================================================================================================= */
/* Callback in charge of Wi-Fi network monitoring. */
bool callback_5sec_timer(struct repeating_timer *t);

/* Retrieve Pico's Unique ID from the flash IC. */
void get_pico_unique_id(UCHAR *PicoUniqueId);

/* Read data from stdin. */
void input_string(UCHAR *String);

/* Log data to log file. */
void log_info(UINT LineNumber, const UCHAR *FunctionName, UCHAR *Format, ...);

/* Logon to local network. */
void network_logon(struct struct_wifi *StructWiFi);

/* Print results of the scan process. */
void print_results(UINT8 SortOrder);

/* Print a single entry. */
void print_single_entry(UINT16 EntryNumber);

/* Reverse order of two specific results. */
void reverse_order(UINT16 Position1, UINT16 Position2);

/* Retrieve results of the IP scan process. */
static int scan_results(void *env, const cyw43_ev_scan_result_t *result);

/* Sort results of the scan process. */
void sort_results(UINT8 SortOrder);

/* Terminal menu when a CDC USB connection is detected during power up sequence. */
void term_menu(struct struct_wifi *StructWiFi);

/* Wipe results. */
void wipe_results(void);




/* $PAGE */
/* $TITLE=Main program entry point. */
/* ============================================================================================================================================================= *\
                                                                      Main program entry point.
\* ============================================================================================================================================================= */
int main()
{
  UCHAR String[65];
  UCHAR PicoUniqueId[25];

  INT16 ReturnCode;

  UINT8 Delay;
  UINT8 FlagScanning;

  UINT16 Loop1UInt16;

  struct struct_wifi StructWiFi;

  absolute_time_t ScanTime;


  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                            Initializations.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  Delay        = 0;
  FlagLogon    = FLAG_OFF;  // logon has not been done on entry.
  FlagScanning = FLAG_OFF;
  ScanTime     = nil_time;
  StructWiFi.CountryCode = COUNTRY_CODE;
  stdio_init_all();

  strcpy(StructWiFi.NetworkName,     WIFI_SSID);      // network name is read from environment variable (see User Guide).
  strcpy(StructWiFi.NetworkPassword, WIFI_PASSWORD);  // password is read from environment variable (see User Guide).



  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                    Wait for CDC USB connection.
                                  PicoW will blink its LED while waiting for a CDC USB connection.
                               It will give up and continue after a while and continue with the code.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  /* Wait for CDC USB connection. */
  printf("[%5u] - Before delay, waiting for a CDC USB connection.\r", __LINE__);
  sleep_ms(1000);  // slow down startup sequence for debugging purposes.

  /* Wait until CDC USB connection is established. */
  while (stdio_usb_connected() == 0)
  {
    ++Delay;  // one more 1-second cycle waiting for CDC USB connection.
    wifi_blink(250, 250, 1);

    /* If we waited for more than 120 seconds for a CDC USB connection, get out of the loop and continue. */
    if (Delay > 120) break;
  }

  get_pico_unique_id(PicoUniqueId);

  log_info(__LINE__, __func__, "==============================================================================================================\r");
  log_info(__LINE__, __func__, "                                              Pico-WiFi-Example\r");
  log_info(__LINE__, __func__, "                                    Part of the ASTL Smart Home ecosystem.\r");
  log_info(__LINE__, __func__, "                                    Pico unique ID: <%s>.\r", PicoUniqueId);
  log_info(__LINE__, __func__, "==============================================================================================================\r");
  log_info(__LINE__, __func__, "Main program entry point (Delay: %u msec waiting for CDC USB connection).\r", (Delay * 50));



  /* Check if CDC USB connection has been detected.*/
  if (stdio_usb_connected()) log_info(__LINE__, __func__, "CDC USB connection has been detected.\r", __LINE__);


  if (wifi_init(&StructWiFi))
  {
    log_info(__LINE__, __func__, "Failed to initialize cyw43\r");
    return 1;
  }
  else
  {
    log_info(__LINE__, __func__, "Cyw43 initialization successful.\r");
  }
  

  /* Set station mode. */
  log_info(__LINE__, __func__, "Setting station mode\r\r\r");
  cyw43_arch_enable_sta_mode();
  

  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                      Loop on the terminal menu.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  while (1)
  {
    term_menu(&StructWiFi);
  }

  return 0;
}





/* $TITLE=callback_5sec_timer() */
/* $PAGE */
/* ============================================================================================================================================================= *\
                                                          Callback in charge of monitoring Wi-Fi network health.
\* ============================================================================================================================================================= */
bool callback_5sec_timer(struct repeating_timer *t)
{
  INT ReturnCode;

  UINT16 Loop1UInt16;

  static UINT64 TimeStamp;


  if (TimeStamp == 0ll) TimeStamp = time_us_64();  // keep track of callback launch.

  ReturnCode = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
  if (ReturnCode == CYW43_LINK_UP)
  {
    // log_info(__LINE__, __func__, "time_us_64(): %12llu     TimeStamp: %12llu     TimeStamp + (30 * 1000000): %12llu\r", time_us_64(), TimeStamp, (TimeStamp + (30 * 1000000)));
    if (time_us_64() < (TimeStamp + (15 * 1000000))) log_info(__LINE__, __func__, "Wi-Fi connection OK.\r");
    wifi_blink(50, 200, 1);
  }
  else
  {
    if (time_us_64() < (TimeStamp + (15 * 1000000))) log_info(__LINE__, __func__, "Problems with Wi-Fi connection...\r");
    wifi_blink(50, 200, 3);
  }

  return true;
}





/* $PAGE */
/* $TITLE=get_pico_unique_id() */
/* ============================================================================================================================================================= *\
                                                           Retrieve Pico's Unique ID from the flash IC.
\* ============================================================================================================================================================= */
void get_pico_unique_id(UCHAR *PicoUniqueId)
{
  UINT8 Loop1UInt8;

  pico_unique_board_id_t board_id;


  /* Retrieve Pico Unique ID from its flash memory IC. */
  pico_get_unique_board_id(&board_id);

  /* Build the Unique ID string in hex. */
  PicoUniqueId[0] = 0x00;  // initialize as null string on entry.
  for (Loop1UInt8 = 0; Loop1UInt8 < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; ++Loop1UInt8)
  {
    // log_info(__LINE__, __func__, "%2u - 0x%2.2X\r", Loop1UInt8, board_id.id[Loop1UInt8]);
    sprintf(&PicoUniqueId[strlen(PicoUniqueId)], "%2.2X", board_id.id[Loop1UInt8]);
    if ((Loop1UInt8 % 2) && (Loop1UInt8 != 7)) sprintf(&PicoUniqueId[strlen(PicoUniqueId)], "-");
  }

  return;
}





/* $PAGE */
/* $TITLE=input_string() */
/* ============================================================================================================================================================= *\
                                                                    Read a string from stdin.
\* ============================================================================================================================================================= */
void input_string(UCHAR *String)
{
  INT8 DataInput;

  UINT8 FlagLocalDebug = FLAG_OFF;
  UINT8 Loop1UInt8;

  UINT32 IdleTimer;


  if (FlagLocalDebug) printf("Entering input_string().\r");

  Loop1UInt8 = 0;
  IdleTimer  = time_us_32();  // initialize time-out timer with current system timer.
  do
  {
    DataInput = getchar_timeout_us(50000);

    switch (DataInput)
    {
      case (PICO_ERROR_TIMEOUT):
      case (0):
#if 0
        /* This section if we want input_string() to return after a timeout wait time. */ 
        if ((time_us_32() - IdleTimer) > 300000000l)
        {
          printf("[%5u] - Input timeout %lu - %lu = %lu!!\r\r\r", __LINE__, time_us_32(), IdleTimer, time_us_32() - IdleTimer);
          String[0]  = 0x1B;  // time-out waiting for a keystroke.
          Loop1UInt8 = 1;     // end-of-string will be added when exiting while loop.
          DataInput  = 0x0D;
        }
#endif  // 0
        continue;
      break;

      case (8):
        /* <Backspace> */
        IdleTimer = time_us_32();  // restart time-out timer.
        if (Loop1UInt8 > 0)
        {
          --Loop1UInt8;
          String[Loop1UInt8] = 0x00;
          printf("%c %c", 0x08, 0x08);  // erase character under the cursor.
        }
      break;

      case (27):
        /* <ESC> */
        IdleTimer = time_us_32();  // restart time-out timer.
        if (Loop1UInt8 == 0)
        {
          String[Loop1UInt8++] = (UCHAR)DataInput;
          String[Loop1UInt8++] = 0x00;
        }
        printf("\r");
      break;

      case (0x0D):
        /* <Enter> */
        IdleTimer = time_us_32();  // restart time-out timer.
        if (Loop1UInt8 == 0)
        {
          String[Loop1UInt8++] = (UCHAR)DataInput;
          String[Loop1UInt8++] = 0x00;
        }
        printf("\r");
      break;

      default:
        IdleTimer = time_us_32();  // restart time-out timer.
        printf("%c", (UCHAR)DataInput);
        String[Loop1UInt8] = (UCHAR)DataInput;
        // printf("Loop1UInt8: %3u   %2.2X - %c\r", Loop1UInt8, DataInput, DataInput);  /// for debugging purposes.
        ++Loop1UInt8;
      break;
    }
    sleep_ms(10);
  } while((Loop1UInt8 < 128) && (DataInput != 0x0D));

  String[Loop1UInt8] = '\0';  // end-of-string
  /// printf("\r\r\r");

  /* Optionally display each character entered. */
  /***
  for (Loop1UInt8 = 0; Loop1UInt8 < 10; ++Loop1UInt8)
    printf("%2u:[%2.2X]   ", Loop1UInt8, String[Loop1UInt8]);
  printf("\r");
  ***/

  if (FlagLocalDebug) printf("Exiting input_string().\r");

  return;
}





/* $PAGE */
/* $TITLE=log_info()) */
/* ============================================================================================================================================================= *\
                                                            Log info to log file through Pico UART or CDC USB.
\* ============================================================================================================================================================= */
void log_info(UINT LineNumber, const UCHAR *FunctionName, UCHAR *Format, ...)
{
  UCHAR Dum1Str[256];
  UCHAR TimeStamp[128];

  UINT Loop1UInt;
  UINT StartChar;

  va_list argp;


  /* Transfer the text to print to variable Dum1Str */
  va_start(argp, Format);
  vsnprintf(Dum1Str, sizeof(Dum1Str), Format, argp);
  va_end(argp);

  /* Trap special control code for <HOME>. Replace "home" by appropriate control characters for "Home" on a VT101. */
  if (strcmp(Dum1Str, "home") == 0)
  {
    Dum1Str[0] = 0x1B; // ESC code
    Dum1Str[1] = '[';
    Dum1Str[2] = 'H';
    Dum1Str[3] = 0x00;
  }

  /* Trap special control code for <CLS>. Replace "cls" by appropriate control characters for "Clear screen" on a VT101. */
  if (strcmp(Dum1Str, "cls") == 0)
  {
    Dum1Str[0] = 0x1B; // ESC code
    Dum1Str[1] = '[';
    Dum1Str[2] = '2';
    Dum1Str[3] = 'J';
    Dum1Str[4] = 0x00;
  }

  /* Time stamp will not be printed if first character is a '-' (for title line when starting debug, for example),
     or if first character is a line feed '\r' when we simply want add line spacing in the debug log,
     or if first character is the beginning of a control stream (for example 'Home' or "Clear screen'). */
  if ((Dum1Str[0] != '-') && (Dum1Str[0] != '\r') && (Dum1Str[0] != 0x1B) && (Dum1Str[0] != '|'))
  {
    /* Send line number through UART. */
    printf("[%7u] - ", LineNumber);

    /* Display function name. */
    printf("[%s]", FunctionName);
    for (Loop1UInt = strlen(FunctionName); Loop1UInt < 25; ++Loop1UInt)
    {
      printf(" ");
    }
    printf("- ");


    /* Retrieve current time stamp. */
    // date_stamp(TimeStamp);

    /* Send time stamp through UART. */
    // printf(TimeStamp);
  }

  /* Send string through stdout. */
  printf(Dum1Str);

  return;
}





/* $PAGE */
/* $TITLE=network_logon(). */
/* ============================================================================================================================================================= *\
                                                                   Logon to local network.
\* ============================================================================================================================================================= */
void network_logon(struct struct_wifi *StructWiFi)
{
  UCHAR String[65];

  INT16 ReturnCode;


  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                       Give an opportunity for user to change network nanme (SSID).
  \* --------------------------------------------------------------------------------------------------------------------------- */
  log_info(__LINE__, __func__, "Current network name is <%s>\r", StructWiFi->NetworkName);
  log_info(__LINE__, __func__, "Enter new network name or <Enter> to keep current one: ");
  input_string(String);
  if ((String[0] != 0x0D) && (String[0] != 0x1B))
  {
    strcpy(StructWiFi->NetworkName, String);
    log_info(__LINE__, __func__, "Network name has been changed to: <%s>.\r", StructWiFi->NetworkName);
  }
  else
  {
    log_info(__LINE__, __func__, "Network name has not been changed: <%s>.\r", StructWiFi->NetworkName);
  }
  log_info(__LINE__, __func__, "Press <Enter> to continue: ");
  input_string(String);



  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                      Give an opportunity for user to change network password.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  printf("\r\r");
  log_info(__LINE__, __func__, "Current network password is <%s>\r", StructWiFi->NetworkPassword);
  log_info(__LINE__, __func__, "Enter new network password or <Enter> to keep current one: ");
  input_string(String);
  if ((String[0] != 0x0D) && (String[0] != 0x1B))
  {
    strcpy(StructWiFi->NetworkPassword, String);
    log_info(__LINE__, __func__, "Network password has been changed to: <%s>.\r", StructWiFi->NetworkPassword);
  }
  else
  {
    log_info(__LINE__, __func__, "Network password has not been changed: <%s>.\r", StructWiFi->NetworkPassword);
  }
  log_info(__LINE__, __func__, "Press <Enter> to continue: ", __LINE__);
  input_string(String);



  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                     Establish WiFi connection.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  printf("\r\r");
  log_info(__LINE__, __func__, "Trying to establish Wi-Fi connection.\r");
  if ((ReturnCode = wifi_connect(StructWiFi)) != 0)
  {
    log_info(__LINE__, __func__, "Error while trying to establish a Wi-Fi connection (ReturnCode: %d).\r", ReturnCode);
    log_info(__LINE__, __func__, "Aborting Firmware...\r");
  }
  log_info(__LINE__, __func__, "Wi-Fi connection established successfully.\r");
  FlagLogon = FLAG_ON;

  return;
}





/* $PAGE */
/* $TITLE=print_result(). */
/* ============================================================================================================================================================= *\
                                                                  Print results of the IP scan process.
\* ============================================================================================================================================================= */
void print_results(UINT8 SortOrder)
{
  UINT16 Loop1UInt16;
  UINT16 Loop2UInt16;


  /* Display  header. */
  log_info(__LINE__, __func__, "==================================================================================================================================\r");
  log_info(__LINE__, __func__, "                                                  Results of Access Points scan.\r");

  switch (SortOrder)
  {
    case (1):
      log_info(__LINE__, __func__, "                                             Listed in the order they were scanned.\r");
    break;
  
    case (2):
      log_info(__LINE__, __func__, "                                         Results have been sorted by MAC address order.\r");
    break;
  }

  log_info(__LINE__, __func__, "==================================================================================================================================\r");
  log_info(__LINE__, __func__, "         Network                        Signal    Channel       MAC             ------------------   Security   ------------------\r");
  log_info(__LINE__, __func__, "          name                         strength               address\r");
  log_info(__LINE__, __func__, "==================================================================================================================================\r");

  for (Loop1UInt16 = 1; WlanFound[Loop1UInt16].Channel; ++Loop1UInt16)
    print_single_entry(Loop1UInt16);

  log_info(__LINE__, __func__, "==================================================================================================================================\r\r\r");

  return;
}





/* $PAGE */
/* $TITLE=print_single_entry(). */
/* ============================================================================================================================================================= *\
                                                                      Print a single entry.
\* ============================================================================================================================================================= */
void print_single_entry(UINT16 EntryNumber)
{
  UINT16 Loop1UInt16;


  log_info(__LINE__, __func__, "%3u)   %-32s  %4d      %3ld   ", EntryNumber, WlanFound[EntryNumber].NetworkName, WlanFound[EntryNumber].SignalStrength, WlanFound[EntryNumber].Channel);

  for (Loop1UInt16 = 0; Loop1UInt16 < 6; ++Loop1UInt16)
  {
    printf("%02X", WlanFound[EntryNumber].MacAddress[Loop1UInt16]);
    if (Loop1UInt16 < 5) printf(":");
  }

  printf("     %u   ", WlanFound[EntryNumber].Security);

#if 0
  switch (WlanFound[EntryNumber].Security)
  {
    case (0):
      printf("Open network");
    break;

    case (0x00200002):
      printf("Encrypted      WPA");
    break;

    case (0x00400004):
      printf("Encrypted      WPA2");
    break;

    case (0x00400006):
      printf("Encrypted      WPA / WPA2 mixed");
    break;

    case (0x01000004):
      printf("Encrypted      WPA3 / AES");
    break;

    case (0x01400004):
      printf("Encrypted      WPA2 / WPA3");
    break;

    default:
      printf("security mode not found: 0x%X", WlanFound[EntryNumber].Security);
    break;
  }
#endif  // 0

  printf("\r");

  return;
}





/* $PAGE */
/* $TITLE=reverse_order(). */
/* ============================================================================================================================================================= *\
                                                                  Reverse order of two specific results.
\* ============================================================================================================================================================= */
void reverse_order(UINT16 Position1, UINT16 Position2)
{
  UCHAR *PointerFrom;
  UCHAR *PointerTo;

  UINT16 Loop1UInt16;


  /* Use first vector position [0] for temporary storage. */
  PointerFrom = (UCHAR *)&WlanFound[Position1];
  PointerTo   = (UCHAR *)&WlanFound[0];
  for (Loop1UInt16 = 0; Loop1UInt16 < sizeof(WlanFound[0]); ++Loop1UInt16)
    PointerTo[Loop1UInt16] = PointerFrom[Loop1UInt16];

  /* Transfer first member. */
  PointerFrom = (UCHAR *)&WlanFound[Position2];
  PointerTo   = (UCHAR *)&WlanFound[Position1];
  for (Loop1UInt16 = 0; Loop1UInt16 < sizeof(WlanFound[0]); ++Loop1UInt16)
    PointerTo[Loop1UInt16] = PointerFrom[Loop1UInt16];

  /* Transfer other member. */
  PointerFrom = (UCHAR *)&WlanFound[0];
  PointerTo   = (UCHAR *)&WlanFound[Position2];
  for (Loop1UInt16 = 0; Loop1UInt16 < sizeof(WlanFound[0]); ++Loop1UInt16)
    PointerTo[Loop1UInt16] = PointerFrom[Loop1UInt16];

  return;
}





/* $PAGE */
/* $TITLE=scan_result(). */
/* ============================================================================================================================================================= *\
                                                                  Retrieve results of the IP scan process.
\* ============================================================================================================================================================= */
static int scan_results(void *env, const cyw43_ev_scan_result_t *Result)
{
  UINT8 Loop1UInt8;


  if (Result)
  {
    log_info(__LINE__, __func__, "%2u)   %-32s   %4d      %3ld   %02X:%02X:%02X:%02X:%02X:%02X     %u     \r",
           APNumber,
           Result->ssid, Result->rssi, Result->channel,
           Result->bssid[0], Result->bssid[1], Result->bssid[2], Result->bssid[3], Result->bssid[4], Result->bssid[5],
           Result->auth_mode);

    strcpy(WlanFound[APNumber].NetworkName, Result->ssid);
    WlanFound[APNumber].SignalStrength = Result->rssi;
    WlanFound[APNumber].Channel        = Result->channel;
    WlanFound[APNumber].Security       = Result->auth_mode;
    for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(Result->bssid); ++Loop1UInt8)
      WlanFound[APNumber].MacAddress[Loop1UInt8] = Result->bssid[Loop1UInt8];

    // printf("[%5u] = %d\r", __LINE__, Result->auth_type);
    // printf("[%5u] = %d\r", __LINE__, Result->hidden);
    // printf("[%5u] = %d\r", __LINE__, Result->security);

    ++APNumber;
  }


#if 0
  if (Result->auth_mode == CYW43_AUTH_OPEN)           printf("CYW43_AUTH_OPEN\r");
  if (Result->auth_mode == CYW43_AUTH_WPA_TKIP_PSK)   printf("CYW43_AUTH_WPA_TKIP_PSK\r");
  if (Result->auth_mode == CYW43_AUTH_WPA2_AES_PSK)   printf("CYW43_AUTH_WPA2_AES_PSK\r");
  if (Result->auth_mode == CYW43_AUTH_WPA2_MIXED_PSK) printf("CYW43_AUTH_WPA2_MIXED_PSK\r");



    [CYW43_EV_SET_SSID] = "SET_SSID",
    [CYW43_EV_JOIN] = "JOIN",
    [CYW43_EV_AUTH] = "AUTH",
    [CYW43_EV_DEAUTH_IND] = "DEAUTH_IND",
    [CYW43_EV_ASSOC] = "ASSOC",
    [CYW43_EV_DISASSOC] = "DISASSOC",
    [CYW43_EV_DISASSOC_IND] = "DISASSOC_IND",
    [CYW43_EV_LINK] = "LINK",
    [CYW43_EV_PSK_SUP] = "PSK_SUP",
    [CYW43_EV_ESCAN_RESULT] = "ESCAN_RESULT",
    [CYW43_EV_CSA_COMPLETE_IND] = "CSA_COMPLETE_IND",
    [CYW43_EV_ASSOC_REQ_IE] = "ASSOC_REQ_IE",
    [CYW43_EV_ASSOC_RESP_IE] = "ASSOC_RESP_IE",




  switch ((UINT32)Result->auth_mode)
  {
    case (CYW43_AUTH_OPEN):
      printf("No authorisation required\r");
    break;

    case (CYW43_AUTH_WPA_TKIP_PSK):
      printf(" WPA authorisation\r");
    break;

    case (CYW43_AUTH_WPA2_AES_PSK):
      printf("WPA2 authorization\r");
    break;

    case (CYW43_AUTH_WPA2_MIXED_PSK):
      printf("WPA2/WPA mixed (currently same as CYW43_AUTH_WPA2_AES_PSK)\r");
    break;

    default:
      printf("Authorization mode tbd: %d\r", Result->auth_mode);
    break;
  }
#endif  // 0

  return 0;
}





/* $PAGE */
/* $TITLE=scan_wifi(). */
/* ============================================================================================================================================================= *\
                                                        Scan Wi-Fi frequencies for available Access Points.
\* ============================================================================================================================================================= */
void scan_wifi(void)
{
  UCHAR *Dum1Ptr;

  INT16 ReturnCode;

  UINT16 Loop1UInt16;


  APNumber = 1;

  cyw43_wifi_scan_options_t ScanOptions = {0};


  /* Wipe WlanFound structure on entry. */
  log_info(__LINE__, __func__, "sizeof(WlanFound): %u\r", sizeof(WlanFound));
  Dum1Ptr = (UCHAR *)&WlanFound[0];
  for (Loop1UInt16 = 0; Loop1UInt16 < sizeof(WlanFound); ++Loop1UInt16)
    Dum1Ptr = '\0';


  /* Scan Wi-Fi frequency to find available Access Points. */
  log_info(__LINE__, __func__, "========================================================================================\r");
  log_info(__LINE__, __func__, "                  Scan Wi-Fi spectrum to find available Access Points.\r");
  log_info(__LINE__, __func__, "                         Listed in the order they were scanned.\r");
  log_info(__LINE__, __func__, "               Using frequencies used in the following country: %c%c Rev: %u\r", COUNTRY_CODE, (COUNTRY_CODE >> 8), (COUNTRY_CODE >> 16));
  log_info(__LINE__, __func__, "========================================================================================\r");
  log_info(__LINE__, __func__, "         Network                        Signal    Channel       MAC        Security\r");
  log_info(__LINE__, __func__, "          name                         strength               address\r");
  log_info(__LINE__, __func__, "========================================================================================\r");

  ReturnCode   = cyw43_wifi_scan(&cyw43_state, &ScanOptions, NULL, scan_results);
  if (ReturnCode != 0)
  {
    log_info(__LINE__, __func__, "Error while trying to scan Wi-Fi spectrum...\r");
  }
  else
  {
    while (cyw43_wifi_scan_active(&cyw43_state) == true);  // wait until the scan is over...
    log_info(__LINE__, __func__, "========================================================================================\r\r\r\r");
  }


  print_results(1);
  sort_results(2);
  print_results(2);
  wipe_results();


#if PICO_CYW43_ARCH_POLL
    // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
    // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
    cyw43_arch_poll();
    // you can poll as often as you like, however if you have nothing else to do you can
    // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
    cyw43_arch_wait_for_work_until(ScanTime);
#else
    // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
    // is done via interrupt in the background. This sleep is just an example of some (blocking)
    // work you might be doing.
    sleep_ms(1000);
#endif

  return;
}




/* $PAGE */
/* $TITLE=sort_result(). */
/* ============================================================================================================================================================= *\
                                                                  Sort results of the IP scan process.
\* ============================================================================================================================================================= */
void sort_results(UINT8 SortOrder)
{
  UINT16 Loop1UInt16;
  UINT16 Loop2UInt16;
  UINT8  MacPosition;


  // log_info(__LINE__, __func__, "Entering sort_results().\r");

  switch (SortOrder)
  {
    case (2):
      /* Sort by MAC address. */
      for (Loop1UInt16 = 1; WlanFound[Loop1UInt16].Channel; ++Loop1UInt16)
      {
        for (Loop2UInt16 = Loop1UInt16 + 1; WlanFound[Loop2UInt16].Channel; ++Loop2UInt16)
        {
          /***
          printf("[%5u] - Comparing %u and %u\r", __LINE__, Loop1UInt16, Loop2UInt16);
          print_single_entry(Loop1UInt16);
          print_single_entry(Loop2UInt16);
          ***/

          MacPosition = 0;
          while((WlanFound[Loop2UInt16].MacAddress[MacPosition] == WlanFound[Loop1UInt16].MacAddress[MacPosition]) && (MacPosition < 6)) ++MacPosition;
          if ((MacPosition < 6) && (WlanFound[Loop2UInt16].MacAddress[MacPosition] < WlanFound[Loop1UInt16].MacAddress[MacPosition]))
          {
            // printf("[%5u] - Inverting\r", __LINE__);
            reverse_order(Loop1UInt16, Loop2UInt16);
          }
        }
      }

    break;
  }

  // log_info(__LINE__, __func__, "Exiting sort_results().\r");

  return;
}




/* $PAGE */
/* $TITLE=term_menu()) */
/* ============================================================================================================================================================= *\
                                          Terminal menu when a CDC USB connection is detected during power up sequence.
\* ============================================================================================================================================================= */
void term_menu(struct struct_wifi *StructWiFi)
{
  UCHAR String[33];

  UINT8 Loop1UInt16;
  UINT8 Menu;

  ip_addr_t PingAddress;
  ip_addr_t TestAddress;


  while (1)
  {
    printf("\r\r\r");
    log_info(__LINE__, __func__, "               Terminal menu\r");
    log_info(__LINE__, __func__, "               =============\r");
    log_info(__LINE__, __func__, "          1) - Scan Wi-Fi frequencies for available Access Points.\r");
    log_info(__LINE__, __func__, "          2) - Logon to local network.\r");
    log_info(__LINE__, __func__, "          3) - Display Wi-Fi network information.\r");
    log_info(__LINE__, __func__, "          4) - Blink Picow's LED.\r");
    log_info(__LINE__, __func__, "          5) - Re-initialize cyw43.\r");
    log_info(__LINE__, __func__, "          6) - Ping a specific IP address.\r");
    log_info(__LINE__, __func__, "          7) - Start a callback to monitor Wi-Fi network health.\r");
    log_info(__LINE__, __func__, "         88) - Restart the Firmware.\r");
    log_info(__LINE__, __func__, "         99) - Switch Pico in upload mode\r\r");

    log_info(__LINE__, __func__, "               Enter your choice: ");
    input_string(String);

    /* If user pressed <Enter> only, loop back to menu. */
    if (String[0] == 0x0D) continue;

    /* If user pressed <ESC>, loop back to menu. */
    if (String[0] == 0x1B)
    {
      String[0] = 0x00;
      printf("\r\r\r");

      return;
    }


    /* User pressed a menu option, execute it. */
    Menu = atoi(String);

    switch(Menu)
    {
       case (1):
        /* Scan Wi-Fi frequencies of specified country to find avaible Access Points. */
        printf("\r\r");
        log_info(__LINE__, __func__, "NOTE: For some obscur reason, the scan must be done just after cyw43 initialization.\r");
        log_info(__LINE__, __func__, "      Some results will not be reported on further reports once network login has been done\r");
        log_info(__LINE__, __func__, "      You can select the menu option to re-initialize the cyw43.\r");
        log_info(__LINE__, __func__, "Press <Enter> to continue: ");
        input_string(String);

        log_info(__LINE__, __func__, "Scan Wi-Fi frequencies to find available Access Points.\r");
        log_info(__LINE__, __func__, "=======================================================\r\r");
        scan_wifi();
        log_info(__LINE__, __func__, "Press <Enter> to continue: ");
        input_string(String);
        printf("\r\r");
      break;

      case (2):
        /* Logon to local network using credentials specified in environmental variables. */
        printf("\r\r");
        log_info(__LINE__, __func__, "Logon to local network.\r");
        log_info(__LINE__, __func__, "=======================\r");
        network_logon(StructWiFi);
        wifi_display_info(StructWiFi);
        log_info(__LINE__, __func__, "Press <Enter> to continue: ");
        input_string(String);
        printf("\r\r");
      break;

      case (3):
        /* Display Wi-Fi network information. */
        printf("\r\r");
        log_info(__LINE__, __func__, "Display Wi-Fi network information.\r");
        log_info(__LINE__, __func__, "==================================\r");
        if (FlagLogon == FLAG_OFF)
        {
          log_info(__LINE__, __func__, "NOTE: Logon to local network has not been done yet.\r");
          log_info(__LINE__, __func__, "      Network information will be wrong / incomplete.\r");
        }
        wifi_display_info(StructWiFi);
        log_info(__LINE__, __func__, "Press <Enter> to continue: ");
        input_string(String);
        printf("\r\r");
      break;

      case (4):
        printf("\r\r");
        log_info(__LINE__, __func__, "Blink PicoW's LED.\r");
        log_info(__LINE__, __func__, "==================\r");
        wifi_blink(100, 200, 10);
        log_info(__LINE__, __func__, "Press <Enter> to continue: ");
        input_string(String);
        printf("\r\r");
      break;

      case (5):
        /* Re-init cyw43. */
        printf("\r\r");
        log_info(__LINE__, __func__, "Re-init cyw43.\r");
        log_info(__LINE__, __func__, "==============\r");
        log_info(__LINE__, __func__, "Press <G> to proceed: ");
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          cyw43_arch_deinit();
          log_info(__LINE__, __func__, "Re-initializing cyw43...\r");
          if (wifi_init(StructWiFi))
          {
            log_info(__LINE__, __func__, "Failed to initialize cyw43\r");
            return;
          }
          else
          {
            log_info(__LINE__, __func__, "Cyw43 initialization successful.\r");
          }
          
          /* Set station mode. */
          log_info(__LINE__, __func__, "Setting station mode\r\r\r");
          cyw43_arch_enable_sta_mode();
        }
        else
        {
          log_info(__LINE__, __func__, "User didn't press <G>. Cyw43 hasn't been re-initialized.\r");
        }
        log_info(__LINE__, __func__, "Press <Enter> to continue: ");
        input_string(String);
        printf("\r\r");
      break;

      case (6):
        /* Ping a apecific IP address. */
        printf("\r\r");
        log_info(__LINE__, __func__, "Ping a specific IP address.\r");
        log_info(__LINE__, __func__, "===========================\r");

        /* Enter IP address to ping. */
        ip4addr_aton(PING_ADDRESS, &PingAddress);
        log_info(__LINE__, __func__, "Current IP address to ping is:   <%s>\r", ip4addr_ntoa(&PingAddress));
        log_info(__LINE__, __func__, "Enter new IP address to ping or <Enter> to keep current target IP address: ");
        input_string(String);
        if ((String[0] != 0x0D) && (String[0] != 0x1B))
        {
          if (!ip4addr_aton(String, &TestAddress))
          {
            log_info(__LINE__, __func__, "Invalid IP address entered... IP address has not been changed: <%s>.\r", ip4addr_ntoa(&PingAddress));
          }
          else
          {
            ip4addr_aton(String, &PingAddress);
            log_info(__LINE__, __func__, "Ping IP address has been set to: <%s>\r", ip4addr_ntoa(&PingAddress));
          }
        }
        else
        {
          log_info(__LINE__, __func__, "No change to ping IP address: <%s>.\r", ip4addr_ntoa(&PingAddress));
        }

        printf("\r");
        log_info(__LINE__, __func__, "NOTE: You must be logged on the local network (option 2) for the ping procedure to work.\r\r");
        log_info(__LINE__, __func__, "The Pico will ping the specified IP address on local network.\r");
        log_info(__LINE__, __func__, "If the target IP address is a 'pingable' system, you will see the ping sent and the answer\r");
        log_info(__LINE__, __func__, "received from the target system, along with the number of msec (latency) between the send and the receive.\r");
        log_info(__LINE__, __func__, "Press any key while ping is in progress to stop it and restart the firmware\r");
        log_info(__LINE__, __func__, "Press <G> to begin pinging IP address %s: ", ip4addr_ntoa(&PingAddress));
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          log_info(__LINE__, __func__, "Press any key to restart the firmware...\r");
          ping_init(&PingAddress);
        }
        else
        {
          log_info(__LINE__, __func__, "User didn't press <G> to start ping procedure... aborting.\r");
          break;
        }
        input_string(String);
        cyw43_arch_deinit();
        log_info(__LINE__, __func__, "Restarting the Firmware...\r");
        sleep_ms(1000);
        watchdog_enable(1, 1);
      break;

      case (7):
        /* Start a callback to monitor Wi-Fi network health. */
        printf("\r\r");
        log_info(__LINE__, __func__, "Start a 5-seconds callback to monitor Wi-Fi network health.\r");
        log_info(__LINE__, __func__, "===========================================================\r");
        log_info(__LINE__, __func__, "NOTE: The callback will display Wi-Fi network health for the first 30 seconds,\r");
        log_info(__LINE__, __func__, "      then will stop display network health but will continue monitoring.\r");
        log_info(__LINE__, __func__, "      The callback will blink Pico's LED as long as monitoring is active.\r");
        log_info(__LINE__, __func__, "Press <G> to proceed: ");
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          log_info(__LINE__, __func__, "Starting a 5-seconds callback to monitor Wi-Fi network health.\r");
          log_info(__LINE__, __func__, "NOTE: For the first 30 seconds, a message will show up on the screen to indicate Wi-Fi connection health.\r");
          log_info(__LINE__, __func__, "      Then, you must check Pico's LED:\r");
          log_info(__LINE__, __func__, "      1 blink  every 5 seconds means that Wi-Fi connection is OK.\r");
          log_info(__LINE__, __func__, "      3 blinks every 5 seconds means that there is a problem with Wi-Fi connection.\r");
          sleep_ms(500);  ///
          add_repeating_timer_ms(-5000, callback_5sec_timer, NULL, &Handle5SecTimer);
          sleep_ms(20000);
        }
        else
        {
          log_info(__LINE__, __func__, "User didn't press <G>, do not launch the callback...\r");
        }
        log_info(__LINE__, __func__, "Returning to terminal menu... Check Pico's LED for Wi-Fi status.\r");
        printf("\r\r");
      break;

      case (88):
        /* Restart the Firmware. */
        printf("\r\r");
        log_info(__LINE__, __func__, "Restart the Firmware.\r");
        log_info(__LINE__, __func__, "=====================\r");
        log_info(__LINE__, __func__, "Press <G> to proceed: ");
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          cyw43_arch_deinit();
          log_info(__LINE__, __func__, "Restarting the Firmware...\r");
          watchdog_enable(1, 1);
        }
        sleep_ms(3000);  // prevent beginning of menu redisplay.
      break;

      case (99):
        /* Switch the Pico in upload mode. */
        printf("\r\r");
        log_info(__LINE__, __func__, "Switch Pico in upload mode.\r");
        log_info(__LINE__, __func__, "===========================\r");
        log_info(__LINE__, __func__, "Press <G> to proceed: ");
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          cyw43_arch_deinit();
          log_info(__LINE__, __func__, "Toggling Pico in upload mode...\r");
          reset_usb_boot(0l, 0l);
        }
        printf("\r\r");
      break;

      default:
        printf("\r\r");
        log_info(__LINE__, __func__, "               Invalid choice... please re-enter [%s]  [%u]\r\r\r\r\r", String, Menu);
        printf("\r\r");
      break;
    }
  }

  return;
}





/* $PAGE */
/* $TITLE=wipe_result(). */
/* ============================================================================================================================================================= *\
                                                                           Wipe results.
\* ============================================================================================================================================================= */
void wipe_results(void)
{
  UCHAR *DataPointer;

  UINT16 Loop1UInt16;
  UINT16 Loop2UInt16;

  // printf("[%5u] - Entering wipe_results().\r", __LINE__);

  for (Loop1UInt16 = 0; Loop1UInt16 < MAX_NETWORKS; ++Loop1UInt16)
  {
    DataPointer = (UCHAR *)&WlanFound[Loop1UInt16];
    for (Loop2UInt16 = 0; Loop2UInt16 < sizeof WlanFound[0]; ++Loop2UInt16)
    {
      DataPointer[Loop2UInt16] = 0x00;
    }
  }

  /***
  printf("[%5u] - Vector after wiping: \r", __LINE__);
  for (Loop1UInt16 = 0; Loop1UInt16 < MAX_NETWORKS; ++Loop1UInt16)
    print_single_entry(Loop1UInt16);
  ***/

  // printf("[%5u] - Exiting wipe_results().\r", __LINE__);

  return;
}
