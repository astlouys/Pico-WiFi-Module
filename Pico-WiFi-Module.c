/* ============================================================================================================================================================= *\
   Pico-WiFi-Module.c
   St-Louys Andre - September 2024
   astlouys@gmail.com
   Revision 14-MAY-2025
   Langage: C
   Version 1.01

   Raspberry Pi Pico C-language add-on module to access a Wi-Fi network from a user program / project.

   NOTE:
   This program is provided without any warranty of any kind. It is provided
   simply to help the user develop his own program.

   REVISION HISTORY:
   =================
   02-SEP-2024 1.00 - Initial release.
   14-MAY-2025 1.01 - Rework some sections of code.
                    - Cleanup, cosmetic and optimisation changes.
\* ============================================================================================================================================================= */



/* ============================================================================================================================================================= *\
                                                                               Include files.
\* ============================================================================================================================================================= */
#include "baseline.h"
#include "stdio.h"

#include "Pico-WiFi-Module.h"



/* ============================================================================================================================================================= *\
                                                                                Definitions.
\* ============================================================================================================================================================= */
#define RELEASE_VERSION  ///



/* ============================================================================================================================================================= *\
                                                                              Global variables.
\* ============================================================================================================================================================= */



/* ============================================================================================================================================================= *\
                                                                             Function prototypes.
\* ============================================================================================================================================================= */
/* Log data to log file. */
extern void log_info(UINT LineNumber, const UCHAR *FunctionName, UCHAR *Format, ...);





/* $PAGE */
/* $TITLE=wait_ms() */
/* ============================================================================================================================================================= *\
                                                             Pause the program for specified number of msec.
                       NOTE: This function does not use a sleep_xx(), so it can be used inside a hardware or software interrupt (callback).
\* ============================================================================================================================================================= */
static void wait_ms(UINT16 WaitMSec)
{
  UINT64 TimeStamp;

  TimeStamp = time_us_64();
  while (time_us_64() < (TimeStamp + (WaitMSec * 1000ll)));

  return;
}





/* $PAGE */
/* $TITLE=wifi_blink() */
/* ============================================================================================================================================================= *\
                                                                   Blink PicoW's LED through CYW43.
\* ============================================================================================================================================================= */
void wifi_blink(UINT16 OnTimeMsec, UINT16 OffTimeMsec, UINT8 Repeat)
{
  UINT8 Loop1UInt8;

  UINT64 TimeStamp;


  for (Loop1UInt8 = 0; Loop1UInt8 < Repeat; ++Loop1UInt8)
  {
    cyw43_gpio_set(&cyw43_state, LED_GPIO, true);
    /// cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    
    wait_ms(OnTimeMsec);  // allows usage inside a callback.
    /// sleep_ms(OnTimeMsec);  // sleep_ms() can't be used while in ISR or callback.

    cyw43_gpio_set(&cyw43_state, LED_GPIO, false);
    /// cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    wait_ms(OffTimeMsec);  // allows usage inside a callback.
    /// sleep_ms(OffTimeMsec);  // sleep_ms() can't be used while in ISR or callback.
  }

  return;
}





/* $PAGE */
/* $TITLE=wifi_connect() */
/* ============================================================================================================================================================= *\
                                                                   Initialize Wi-Fi connection.
\* ============================================================================================================================================================= */
INT16 wifi_connect(struct struct_wifi *StructWiFi)
{
#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF all time.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_ON;   // may be turned On for debug purposes.
#endif  // RELEASE_VERSION

  UINT8 Loop1UInt8;
  UINT8 RetryCount;

  INT16 ReturnCode;


  /* Check if there is a monitor connected to stdout. */
  if (!stdio_usb_connected()) FlagLocalDebug = FLAG_OFF;

  /* Initializations. */
  RetryCount = 0;
  StructWiFi->FlagHealth = FLAG_OFF;  // assume failure on entry.

  if (FlagLocalDebug)
  {
    log_info(__LINE__, __func__, "Initializing Wi-Fi connection with the following credentials:\r");
    log_info(__LINE__, __func__, "Network name (SSID): <%s>.\r",   StructWiFi->NetworkName);
    log_info(__LINE__, __func__, "Network password:    <%s>.\r\r", StructWiFi->NetworkPassword);
  }


  /* Enable Wi-Fi Station mode. */
  cyw43_arch_enable_sta_mode();              // initialize Wi-Fi as a client (not as Access Point).
  if (stdio_usb_connected()) sleep_ms(400);  // to keep log display clean on screen.


  /* The time-out next line may be increased or reduced, depending on your Wi-Fi infrastructure response speed. */
  // ReturnCode = cyw43_arch_wifi_connect_timeout_ms(SSID, Password, CYW43_AUTH_WPA2_AES_PSK, 6000);
  // ReturnCode = cyw43_arch_wifi_connect_timeout_ms(SSID, Password, CYW43_AUTH_WPA2_MIXED_PSK, 6000);
  // ReturnCode = cyw43_arch_wifi_connect_blocking(StructWiFi->NetworkName, StructWiFi->NetworkPassword, CYW43_AUTH_WPA2_MIXED_PSK);
  ReturnCode = cyw43_arch_wifi_connect_timeout_ms(StructWiFi->NetworkName, StructWiFi->NetworkPassword, CYW43_AUTH_WPA2_MIXED_PSK, 100);
  if (ReturnCode != CYW43_LINK_UP)
  {
    do
    {
      /* While connection is not successful, blink PicoW's LED a number of times corresponding to the current retry count and wait until we reach maximum retry count. */
      ++RetryCount;
      wifi_blink(50, 200, RetryCount);  // blink current retry count on Pico's LED.

      if (stdio_usb_connected())
      {
        log_info(__LINE__, __func__, "Wi-Fi connection failure - Retry count: %2u / %u   (retrying... return code: %4d) ", RetryCount, MAX_NETWORK_RETRIES, ReturnCode);

        switch (ReturnCode)
        {
          case(CYW43_LINK_DOWN):
            printf("- Error: Link down\r");
          break;

          case(CYW43_LINK_JOIN):
            printf("- Error: Joining\r");
          break;

          case(CYW43_LINK_NOIP):
            printf("- Error: No IP\r");
          break;

          case(CYW43_LINK_UP):
            printf("- Link is up now!\r");
          break;

          case(CYW43_LINK_FAIL):
            printf("- Error: Link fail\r");
          break;

          case(CYW43_LINK_NONET):
            printf("- Error: Network fail\r");
          break;

          case(CYW43_LINK_BADAUTH):
            printf("- Error: Bad auth\r");
          break;

          default:
            printf("- Undefined error number\r");
          break;
        }
      }
      
      if (RetryCount >= MAX_NETWORK_RETRIES)
      {
        if (stdio_usb_connected()) log_info(__LINE__, __func__, "Wi-Fi connection failure - Retry count: %2u / %u   (aborting).\r", RetryCount, MAX_NETWORK_RETRIES);
        break;  // time-out.
      }

      /* No connection yet, wait and check again. */
      sleep_ms(600);
    } while ((ReturnCode = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA)) != CYW43_LINK_UP);


    /* If we went out of "while" loop after MAX_NETWORK_RETRIES connection failures, fast blink PicoW's LED many times to indicate Wi-Fi connection failure. */
    if (RetryCount >= MAX_NETWORK_RETRIES)
    {
      /* In case of error, fast-blink Pico's LED many times to indicate Wi-Fi connection failure. */
      if (stdio_usb_connected()) log_info(__LINE__, __func__, "Failed to establish a Wi-Fi connection.\r\r");
      wifi_blink(25, 150, 30);
      return ReturnCode;
    }
    else
    {
      log_info(__LINE__, __func__, "Wi-Fi connection succeeded after %u retries.\r", RetryCount + 1);
    }
  }


  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                    Wi-Fi connection successful. Keep track of device MAC address.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  StructWiFi->FlagHealth = FLAG_ON;
  cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, StructWiFi->MacAddress);
  // cyw43_hal_get_mac(CYW43_HAL_MAC_WLAN0, StructWiFi->MacAddress);

  if (FlagLocalDebug)
  {
    log_info(__LINE__, __func__, "Wi-Fi connection succeeded (Number of retries: %u).\r", RetryCount + 1);
    log_info(__LINE__, __func__, "Device MAC address: ");
    for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->MacAddress); ++Loop1UInt8)
    {
      printf("%2.2X", StructWiFi->MacAddress[Loop1UInt8]);
      if (Loop1UInt8 < (sizeof(StructWiFi->MacAddress) - 1))
        printf(":");
      else 
        printf("\r");
    }
  }


  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                   Keep track of device Host name.
      Create "ExtraHostName" by appending the last 2 hex digits of the MAC address to the host name to make it more meaningful.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  /* Initialize both host name and extra host name as null string on entry. */
  for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->HostName); ++Loop1UInt8)
    StructWiFi->HostName[Loop1UInt8] = 0x00;

  for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->ExtraHostName); ++Loop1UInt8)
    StructWiFi->ExtraHostName[Loop1UInt8] = 0x00;


  /* Then copy "plain" host name to both variables. */
  memcpy(&StructWiFi->HostName[0],      CYW43_HOST_NAME, sizeof(CYW43_HOST_NAME) - 1);
  memcpy(&StructWiFi->ExtraHostName[0], CYW43_HOST_NAME, sizeof(CYW43_HOST_NAME) - 1);
  
  
  /***
  log_info(__LINE__, __func__, "sizeof(CYW43_HOST_NAME):           %2u\r", sizeof(CYW43_HOST_NAME));
  log_info(__LINE__, __func__, "sizeof(StructWiFi->HostName):      %2u\r", sizeof(StructWiFi->HostName));
  log_info(__LINE__, __func__, "sizeof(CYW43_HOST_NAME) + 4:       %2u\r", sizeof(CYW43_HOST_NAME) + 4);
  log_info(__LINE__, __func__, "sizeof(StructWiFi->ExtraHostName): %2u\r", sizeof(StructWiFi->ExtraHostName));
  log_info(__LINE__, __func__, "1)   Host name:        ");

  for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->HostName); ++Loop1UInt8)
    printf("%2.2X - ", StructWiFi->HostName[Loop1UInt8]);
  printf("\r");

  log_info(__LINE__, __func__, "1)   Extra host name:  ");
  for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->ExtraHostName); ++Loop1UInt8)
    printf("%2.2X - ", StructWiFi->ExtraHostName[Loop1UInt8]);
  printf("\r");
  ***/


  /* Finally, append the last two hex digits of the mac address to the "plain" host name to complete the "extra" host name. */
  sprintf(&StructWiFi->ExtraHostName[strlen(StructWiFi->ExtraHostName)], "%2.2X%2.2X", StructWiFi->MacAddress[sizeof(StructWiFi->MacAddress) - 2], StructWiFi->MacAddress[sizeof(StructWiFi->MacAddress) - 1]);


  /***
  log_info(__LINE__, __func__, "2)   Host name:        ");
  for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->HostName); ++Loop1UInt8)
    printf("%2.2X - ", StructWiFi->HostName[Loop1UInt8]);
  printf("\r");

  log_info(__LINE__, __func__, "2)   Extra host name:  ");
  for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->ExtraHostName); ++Loop1UInt8)
    printf("%2.2X - ", StructWiFi->ExtraHostName[Loop1UInt8]);
  printf("\r");
  ***/


  log_info(__LINE__, __func__, "HostName:           <%s>\r", StructWiFi->HostName);
  log_info(__LINE__, __func__, "ExtraHostName:      <%s>\r", StructWiFi->ExtraHostName);
  netif_set_hostname(&cyw43_state.netif[CYW43_ITF_STA], StructWiFi->ExtraHostName);


  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                     Keep track of Pico IP address.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  StructWiFi->PicoIPAddress = *netif_ip4_addr(netif_list);
  log_info(__LINE__, __func__, "Pico IP Address:    <%s>\r", ip4addr_ntoa(&StructWiFi->PicoIPAddress));

  /* Fast blink Pico's LED 5 times to indicate wi-fi successful connection. */
  wifi_blink(100, 100, 5);

  return 0;
}





/* $PAGE */
/* $TITLE=wifi_display_info(). */
/* ============================================================================================================================================================= *\
                                                                    Display Wi-Fi information.
\* ============================================================================================================================================================= */
void wifi_display_info(struct struct_wifi *StructWiFi)
{
  UCHAR String[35];

  INT ReturnCode;

  UINT8 BSSID[6];
  UINT8 Loop1UInt8;

  INT32 RssiValue;


  /* Check if there is a monitor connected to stdout. */
  if (!stdio_usb_connected()) return;

  log_info(__LINE__, __func__, "======================================================================\r");
  log_info(__LINE__, __func__, "                           Wi-Fi information\r");
  log_info(__LINE__, __func__, "======================================================================\r");

  if (StructWiFi->FlagHealth == FLAG_ON)
    strcpy(String, "Good");
  else
    strcpy(String, "Problems");

  ReturnCode = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
  log_info(__LINE__, __func__, "cyw43_tcpip_link_status() returned status:  %3d ", ReturnCode);
  switch (ReturnCode)
  {
    case(CYW43_LINK_DOWN):
      printf("(Error: Link down)\r");
    break;

    case(CYW43_LINK_JOIN):
      printf("(Error: Joining)\r");
    break;

    case(CYW43_LINK_NOIP):
      printf("(Error: No IP)\r");
    break;

    case(CYW43_LINK_UP):
      printf("(Link is up now!)\r");
    break;

    case(CYW43_LINK_FAIL):
      printf("(Error: Link fail)\r");
    break;

    case(CYW43_LINK_NONET):
      printf("(Error: Network fail)\r");
    break;

    case(CYW43_LINK_BADAUTH):
      printf("(Error: Bad auth)\r");
    break;

    default:
      printf("- Undefined error number\r");
    break;
  }
  sleep_ms(50);

  ReturnCode = cyw43_wifi_get_rssi(&cyw43_state, &RssiValue);
  log_info(__LINE__, __func__, "cyw43_wifi_get_rssi()  returned rssi value: %3ld\r", RssiValue);
  sleep_ms(50);

  ReturnCode = cyw43_wifi_get_bssid(&cyw43_state, BSSID);
  log_info(__LINE__, __func__, "cyw43_wifi_get_bssid() returned bssid:       %2.2X-%2.2X-%2.2X-%2.2X-%2.2X-%2.2X\r", BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5]);
  sleep_ms(50);

  log_info(__LINE__, __func__, "Wi-Fi health:        %s\r",   String);
  log_info(__LINE__, __func__, "Wi-Fi total errors:  %lu\r",  StructWiFi->TotalErrors);
  log_info(__LINE__, __func__, "Network name (SSID): <%s>\r", StructWiFi->NetworkName);
  log_info(__LINE__, __func__, "Network password:    <%s>\r", StructWiFi->NetworkPassword);
  log_info(__LINE__, __func__, "Pico IP address:     <%s>\r",  ip4addr_ntoa(&StructWiFi->PicoIPAddress));
  log_info(__LINE__, __func__, "Device MAC address:  <");

  for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->MacAddress); ++Loop1UInt8)
  {
    printf("%2.2X", StructWiFi->MacAddress[Loop1UInt8]);
    if (Loop1UInt8 < (sizeof(StructWiFi->MacAddress) - 1))
      printf(":");
    else
      printf(">\r");
  }

  log_info(__LINE__, __func__, "Host name:           %s\r",           StructWiFi->HostName);
  log_info(__LINE__, __func__, "Extra host name:     %s\r",           StructWiFi->ExtraHostName);
  log_info(__LINE__, __func__, "Country code:        %c%c Rev: %u\r", StructWiFi->CountryCode, (StructWiFi->CountryCode >> 8), (StructWiFi->CountryCode >> 16));
  log_info(__LINE__, __func__, "======================================================================\r", __LINE__);

  return;
}





/* $PAGE */
/* $TITLE=wifi_init() */
/* ============================================================================================================================================================= *\
                                                        Initialize PicoW's integrated cyw43 (WiFi electronic module).
\* ============================================================================================================================================================= */
INT16 wifi_init(struct struct_wifi *StructWiFi)
{
#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF all time.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;   // may be turned On for debug purposes.
#endif  // RELEASE_VERSION

  UINT8 Loop1UInt8;

  INT16 ReturnCode;


  /* Check if there is a terminal connected to stdout. */
  if (!stdio_usb_connected()) FlagLocalDebug = FLAG_OFF;

  if (FlagLocalDebug) log_info(__LINE__, __func__, "Entering wifi_init().\r");

  if ((ReturnCode = cyw43_arch_init_with_country(StructWiFi->CountryCode)) != 0)
  {
    if (stdio_usb_connected()) log_info(__LINE__, __func__, "Error %d while trying to initialize cyw43 on the PicoW.\r", ReturnCode);
  }
  else
  {
    if (FlagLocalDebug) log_info(__LINE__, __func__, "cyw43 initialization was successful.\r");
  }

  
  /* Initialize both host name and extra host name as null string. */
  for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->HostName); ++Loop1UInt8)
    StructWiFi->HostName[Loop1UInt8] = 0x00;

  for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->ExtraHostName); ++Loop1UInt8)
    StructWiFi->ExtraHostName[Loop1UInt8] = 0x00;

  StructWiFi->TotalErrors = 0l;

  if (FlagLocalDebug) log_info(__LINE__, __func__, "Exiting wifi_init().\r");

  return ReturnCode;
}
