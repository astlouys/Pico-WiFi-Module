/*
* Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/* ============================================================================================================================================================= *\
   Pico-WiFi-Example.c
   St-Louys Andre - October 2024
   astlouys@gmail.com
   Revision 14-OCT-2024
   Langage: C with arm-none-eabi
   Version 2.00

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
               1.00 - Original release from Raspberry Pi Ltd.
   03-OCT-2024 2.00 - Adapted to Pico-WiFi-Driver by Andre St-Louys.
\* ============================================================================================================================================================= */


/* $TITLE=Include files. */
/* $PAGE */
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

struct struct_wifi StructWiFi;



/* $TITLE=Function prototypes. */
/* $PAGE */
/* ============================================================================================================================================================= *\
                                                                     Function prototypes.
\* ============================================================================================================================================================= */
/* Read a string from stdin. */
void input_string(UCHAR *String);

/* Logon to local network. */
void network_logon(void);

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
void term_menu(void);

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

  INT16 ReturnCode;

  UINT8 Delay;
  UINT8 FlagScanning;

  UINT16 Loop1UInt16;

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

  strcpy(StructWiFi.NetworkName,     WIFI_SSID);
  strcpy(StructWiFi.NetworkPassword, WIFI_PASSWORD);



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

    /* If we waited for more than 60 seconds for a CDC USB connection, get out of the loop and continue. */
    if (Delay > 120) break;
  }


  /* Check if CDC USB connection has been detected.*/
  if (stdio_usb_connected()) printf("[%5u] - CDC USB connection has been detected.\r", __LINE__);


  if (wifi_cyw43_init(&StructWiFi))
  {
    printf("[%5u] - Failed to initialize cyw43\r", __LINE__);
    return 1;
  }
  else
  {
    printf("[%5u] - Cyw43 initialization successful.\r", __LINE__);
  }
  

  /* Set station mode. */
  printf("[%5u] - Setting station mode\r\r\r", __LINE__);
  cyw43_arch_enable_sta_mode();
  

  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                      Loop on the terminal menu.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  while (1)
  {
    term_menu();
  }

  return 0;
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
/* $TITLE=network_logon(). */
/* ============================================================================================================================================================= *\
                                                                   Logon to local network.
\* ============================================================================================================================================================= */
void network_logon(void)
{
  UCHAR String[65];

  INT16 ReturnCode;


  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                       Give an opportunity for user to change network nanme (SSID).
  \* --------------------------------------------------------------------------------------------------------------------------- */
  printf("[%5u] - Current network name is <%s>\r", __LINE__, StructWiFi.NetworkName);
  printf("[%5u] - Enter new network name or <Enter> to keep current one: ", __LINE__);
  input_string(String);
  if ((String[0] != 0x0D) && (String[0] != 0x1B))
  {
    strcpy(StructWiFi.NetworkName, String);
    printf("[%5u] - Network name has been changed to: <%s>.\r", __LINE__, StructWiFi.NetworkName);
  }
  else
  {
    printf("[%5u] - Network name has not been changed: <%s>.\r", __LINE__, StructWiFi.NetworkName);
  }
  printf("[%5u] - Press <Enter> to continue: ", __LINE__);
  input_string(String);



  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                      Give an opportunity for user to change network password.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  printf("\r\r");
  printf("[%5u] - Current network password is <%s>\r", __LINE__, StructWiFi.NetworkPassword);
  printf("[%5u] - Enter new network password or <Enter> to keep current one: ", __LINE__);
  input_string(String);
  if ((String[0] != 0x0D) && (String[0] != 0x1B))
  {
    strcpy(StructWiFi.NetworkPassword, String);
    printf("[%5u] - Network password has been changed to: <%s>.\r", __LINE__, StructWiFi.NetworkPassword);
  }
  else
  {
    printf("[%5u] - Network password has not been changed: <%s>.\r", __LINE__, StructWiFi.NetworkPassword);
  }
  printf("[%5u] - Press <Enter> to continue: ", __LINE__);
  input_string(String);



  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                     Establish WiFi connection.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  printf("\r\r");
  printf("[%5u] - Trying to establish Wi-Fi connection.\r", __LINE__);
  if ((ReturnCode = wifi_init_connection(&StructWiFi)) != 0)
  {
    printf("[%5u] - Error while trying to establish a Wi-Fi connection (ReturnCode: %d).\r", __LINE__, ReturnCode);
    printf("[%5u] - Aborting Firmware...\r", __LINE__);
  }
  printf("[%5u] - Wi-Fi connection established successfully.\r", __LINE__);
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
  printf("     ==================================================================================================================================\r");
  printf("                                                       Results of Access Points scan.\r");

  switch (SortOrder)
  {
    case (1):
      printf("                                          Listed in the order they were scanned (Channel order).\r");
    break;
  
    case (2):
      printf("                                              Results have been sorted by MAC address order.\r");
    break;
  }

  printf("     ==================================================================================================================================\r\r");
  printf("              Network                        Signal    Channel       MAC             ------------------   Security   ------------------\r");
  printf("               name                         strength               address\r");
  printf("     ==================================================================================================================================\r");

  for (Loop1UInt16 = 1; WlanFound[Loop1UInt16].Channel; ++Loop1UInt16)
    print_single_entry(Loop1UInt16);

  printf("     ==================================================================================================================================\r\r\r");

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


  printf("     %3u)   %-32s  %4d      %3ld   ", EntryNumber, WlanFound[EntryNumber].NetworkName, WlanFound[EntryNumber].SignalStrength, WlanFound[EntryNumber].Channel);

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
    printf("     %2u)   %-32s   %4d      %3ld   %02X:%02X:%02X:%02X:%02X:%02X     %u     \r",
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
  UCHAR  String[65];

  INT16 ReturnCode;

  UINT16 Loop1UInt16;


  APNumber = 1;

  cyw43_wifi_scan_options_t ScanOptions = {0};


  /* Wipe WlanFound structure on entry. */
  printf("[%5u] - sizeof(WlanFound): %u\r", __LINE__, sizeof(WlanFound));
  Dum1Ptr = (UCHAR *)&WlanFound[0];
  for (Loop1UInt16 = 0; Loop1UInt16 < sizeof(WlanFound); ++Loop1UInt16)
    Dum1Ptr = '\0';


  /* Scan Wi-Fi frequency to find available Access Points. */
  printf("     ========================================================================================\r");
  printf("                       Scan Wi-Fi spectrum to find available Access Points.\r");
  printf("                      Listed in the order they were scanned (Channel order).\r");
  printf("                    Using frequencies used in the following country: %c%c Rev: %u\r", COUNTRY_CODE, (COUNTRY_CODE >> 8), (COUNTRY_CODE >> 16));
  printf("     ========================================================================================\r");
  printf("              Network                        Signal    Channel       MAC        Security\r");
  printf("               name                         strength               address\r");
  printf("     ========================================================================================\r");

  ReturnCode   = cyw43_wifi_scan(&cyw43_state, &ScanOptions, NULL, scan_results);
  if (ReturnCode != 0)
  {
    printf("[%5u] - Error while trying to scan Wi-Fi spectrum...\r");
  }
  else
  {
    while (cyw43_wifi_scan_active(&cyw43_state) == true);  // wait until the scan is over...
    printf("     ========================================================================================\r\r\r\r");
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


  // printf("[%5u] - Entering sort_results().\r", __LINE__);

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

  // printf("[%5u] - Exiting sort_results().\r", __LINE__);

  return;
}




/* $PAGE */
/* $TITLE=term_menu()) */
/* ============================================================================================================================================================= *\
                                          Terminal menu when a CDC USB connection is detected during power up sequence.
\* ============================================================================================================================================================= */
void term_menu(void)
{
  UCHAR String[33];

  UINT8 Loop1UInt16;
  UINT8 Menu;

  ip_addr_t PingAddress;
  ip_addr_t TestAddress;


  while (1)
  {
    printf("\r\r\r");
    printf("                         Terminal menu\r\r");
    printf("               1) - Scan Wi-Fi frequencies for available Access Points.\r");
    printf("               2) - Logon to local network.\r");
    printf("               3) - Display Wi-Fi network information.\r");
    printf("               4) - Blink Picow's LED.\r");
    printf("               5) - Re-initialize cyw43.\r");
    printf("               6) - Ping a specific IP address.\r");
    printf("               7) - Restart the Firmware.\r");
    printf("               8) - Switch Pico in upload mode\r\r");

    printf("                       Enter your choice: ");
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
        printf("[%5u] - NOTE: For some obscur reason, the scan must be done just after cyw43 initialization.\r",        __LINE__);
        printf("[%5u] -       Some results will not be reported on further reports once network login has been done\r", __LINE__);
        printf("[%5u] -       You can select the menu option to re-initialize the cyw43.\r", __LINE__);
        printf("[%5u] - Press <Enter> to continue: ", __LINE__);
        input_string(String);

        printf("[%5u] - Scan Wi-Fi frequencies to find available Access Points.\r",   __LINE__);
        printf("[%5u] - =======================================================\r\r", __LINE__);
        scan_wifi();
        printf("[%5u] - Press <Enter> to continue: ", __LINE__);
        input_string(String);
        printf("\r\r");
      break;

      case (2):
        /* Logon to local network using credentials specified in environmental variables. */
        printf("\r\r");
        printf("[%5u] - Logon to local network.\r",   __LINE__);
        printf("[%5u] - =======================\r", __LINE__);
        network_logon();
        wifi_display_info(&StructWiFi);
        printf("[%5u] - Press <Enter> to continue: ", __LINE__);
        input_string(String);
        printf("\r\r");
      break;

      case (3):
        /* Display Wi-Fi network information. */
        printf("\r\r");
        printf("[%5u] - Display Wi-Fi network information.\r",   __LINE__);
        printf("[%5u] - ==================================\r", __LINE__);
        if (FlagLogon == FLAG_OFF)
        {
          printf("[%5u] - NOTE: Logon to local network has not been done yet.\r", __LINE__);
          printf("[%5u] -       Network information will be wrong / incomplete.\r", __LINE__);
        }
        wifi_display_info(&StructWiFi);
        printf("[%5u] - Press <Enter> to continue: ", __LINE__);
        input_string(String);
        printf("\r\r");
      break;

      case (4):
        printf("\r\r");
        printf("[%5u] - Blink PicoW's LED.\r",   __LINE__);
        printf("[%5u] - ==================\r", __LINE__);
        wifi_blink(100, 200, 10);
        printf("[%5u] - Press <Enter> to continue: ", __LINE__);
        input_string(String);
        printf("\r\r");
      break;

      case (5):
        /* Re-init cyw43. */
        printf("\r\r");
        printf("[%5u] - Re-init cyw43.\r",   __LINE__);
        printf("[%5u] - ==============\r", __LINE__);
        printf("[%5u] - Press <G> to proceed: ",    __LINE__);
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          cyw43_arch_deinit();
          printf("[%5u] - Re-initializing cyw43...\r", __LINE__);
          if (wifi_cyw43_init(&StructWiFi))
          {
            printf("[%5u] - Failed to initialize cyw43\r", __LINE__);
            return;
          }
          else
          {
            printf("[%5u] - Cyw43 initialization successful.\r", __LINE__);
          }
          
          /* Set station mode. */
          printf("[%5u] - Setting station mode\r\r\r", __LINE__);
          cyw43_arch_enable_sta_mode();
        }
        else
        {
          printf("[%5u] - User didn't press <G>. Cyw43 hasn't been re-initialized.\r", __LINE__);
        }
        printf("[%5u] - Press <Enter> to continue: ", __LINE__);
        input_string(String);
        printf("\r\r");
      break;

      case (6):
        /* Ping a apecific IP address. */
        printf("\r\r");
        printf("[%5u] - Ping a specific IP address.\r",   __LINE__);
        printf("[%5u] - ===========================\r", __LINE__);

        /* Enter IP address to ping. */
        ip4addr_aton(PING_ADDRESS, &PingAddress);
        printf("[%5u] - Current IP address to ping is:   <%s>\r", __LINE__, ip4addr_ntoa(&PingAddress));
        printf("[%5u] - Enter new IP address to ping or <Enter> to keep current IP address: ", __LINE__);
        input_string(String);
        if ((String[0] != 0x0D) && (String[0] != 0x1B))
        {
          if (!ip4addr_aton(String, &TestAddress))
          {
            printf("[%5u] - Invalid IP address entered... IP address has not been changed: <%s>.\r", __LINE__, ip4addr_ntoa(&PingAddress));
          }
          else
          {
            ip4addr_aton(String, &PingAddress);
            printf("[%5u] - Ping IP address has been set to: <%s>\r", __LINE__, ip4addr_ntoa(&PingAddress));
          }
        }
        else
        {
          printf("[%5u] - No change to ping IP address: <%s>.\r", __LINE__, ip4addr_ntoa(&PingAddress));
        }

        printf("[%5u] - Press <G> to ping IP address: %s\r",   __LINE__, ip4addr_ntoa(&PingAddress));
        printf("[%5u] - Press any key while pinging is in progress to stop it and restart the firmware: ", __LINE__);
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          printf("[%5u] - Press any key to restart the firmware...\r", __LINE__);
          ping_init(&PingAddress);
        }
        else
        {
          printf("[%5u] - User didn't press <G> to start ping procedure... aborting.\r", __LINE__);
          break;
        }
        input_string(String);
        cyw43_arch_deinit();
        printf("[%5u] - Restarting the Firmware...\r", __LINE__);
        sleep_ms(1000);
        watchdog_enable(1, 1);
      break;

      case (7):
        /* Restart the Firmware. */
        printf("\r\r");
        printf("[%5u] - Restart the Firmware.\r",   __LINE__);
        printf("[%5u] - =====================\r", __LINE__);
        printf("[%5u] - Press <G> to proceed: ",    __LINE__);
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          cyw43_arch_deinit();
          printf("[%5u] - Restarting the Firmware...\r", __LINE__);
          watchdog_enable(1, 1);
        }
        sleep_ms(3000);  // prevent beginning of menu redisplay.
      break;

      case (8):
        /* Switch the Pico in upload mode. */
        printf("\r\r");
        printf("[%5u] - Switch Pico in upload mode.\r",   __LINE__);
        printf("[%5u] - ===========================\r", __LINE__);
        printf("[%5u] - Press <G> to proceed: ",          __LINE__);
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          cyw43_arch_deinit();
          printf("[%5u] - Toggling the Pico in upload mode...\r", __LINE__);
          reset_usb_boot(0l, 0l);
        }
        printf("\r\r");
      break;

      default:
        printf("\r\r");
        printf("                    Invalid choice... please re-enter [%s]  [%u]\r\r\r\r\r", String, Menu);
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
