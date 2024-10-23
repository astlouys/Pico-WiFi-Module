/* ============================================================================================================================================================= *\
   Pico-WiFi-Module.c
   St-Louys Andre - September 2024
   astlouys@gmail.com
   Revision 20-OCT-2024
   Langage: C
   Version 1.00

   Raspberry Pi Pico C-language module to access a Wi-Fi network from a user program.

   NOTE:
   This program is provided without any warranty of any kind. It is provided
   simply to help the user develop his own program.

   REVISION HISTORY:
   =================
   02-SEP-2024 1.00 - Initial release.
\* ============================================================================================================================================================= */



/* ============================================================================================================================================================= *\
                                                                               Include files.
\* ============================================================================================================================================================= */
#include "baseline.h"
#include "Pico-WiFi-Module.h"
#include "stdio.h"



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





/* $PAGE */
/* $TITLE=wifi_blink() */
/* ============================================================================================================================================================= *\
                                                                   Blink Pico's LED through CYW43.
\* ============================================================================================================================================================= */
void wifi_blink(UINT16 OnTimeMsec, UINT16 OffTimeMsec, UINT8 Repeat)
{
  UINT8 Loop1UInt8;

  for (Loop1UInt8 = 0; Loop1UInt8 < Repeat; ++Loop1UInt8)
  {
    cyw43_gpio_set(&cyw43_state, LED_GPIO, true);
    /// cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(OnTimeMsec);
    cyw43_gpio_set(&cyw43_state, LED_GPIO, false);
    /// cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    sleep_ms(OffTimeMsec);
  }

  return;
}





/* $PAGE */
/* $TITLE=wifi_cyw43_init() */
/* ============================================================================================================================================================= *\
                                                                  Initialize the cyw43 on PicoW.
\* ============================================================================================================================================================= */
INT16 wifi_cyw43_init(struct struct_wifi *StructWiFi)
{
#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF all time.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_ON;   // may be modified for debug purposes.
#endif  // RELEASE_VERSION

  INT16 ReturnCode;


  if (FlagLocalDebug) printf("[%5u] - Entering wifi_cyw43_init().\r", __LINE__);

  if ((ReturnCode = cyw43_arch_init_with_country(StructWiFi->CountryCode)) != 0)
  {
    if (FlagLocalDebug) printf("[%5u] - Error %d while trying to initialize cyw43.\r", __LINE__, ReturnCode);
  }
  else
  {
    if (FlagLocalDebug) printf("[%5u] - cyw43 initialized without error.\r", __LINE__);
  }

  if (FlagLocalDebug) printf("[%5u] - Exiting wifi_cyw43_init().\r", __LINE__);

  return ReturnCode;
}





/* $PAGE */
/* $TITLE=wifi_display_info(). */
/* ============================================================================================================================================================= *\
                                                                    Display Wi-Fi information.
\* ============================================================================================================================================================= */
void wifi_display_info(struct struct_wifi *StructWiFi)
{
  UCHAR String[35];

  UINT8 Loop1UInt8;


  printf("[%5u]===============================================================\r", __LINE__);
  printf("[%5u]                       Wi-Fi information\r", __LINE__);
  printf("[%5u]===============================================================\r", __LINE__);

  if (StructWiFi->FlagHealth == FLAG_ON)
    strcpy(String, "Good");
  else
    strcpy(String, "Problems");

  printf("[%5u] -   Wi-Fi health:        %s\r",   __LINE__, String);
  printf("[%5u] -   Network name (SSID): <%s>\r", __LINE__, StructWiFi->NetworkName);
  printf("[%5u] -   Network password:    <%s>\r", __LINE__, StructWiFi->NetworkPassword);
  printf("[%5u] -   Device IP address:   %s\r",   __LINE__, StructWiFi->DeviceIPAddress);
  printf("[%5u] -   Device MAC address:  ",   __LINE__);

  for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->MacAddress); ++Loop1UInt8)
  {
    printf("%2.2X", StructWiFi->MacAddress[Loop1UInt8]);
    if (Loop1UInt8 < (sizeof(StructWiFi->MacAddress) - 1))
      printf(":");
    else
      printf("\r");
  }

  printf("[%5u] -   Host name:           %s\r",             __LINE__, StructWiFi->HostName);
  printf("[%5u] -   Extra host name:     %s\r",             __LINE__, StructWiFi->ExtraHostName);
  printf("[%5u] -   Country: code:       %c%c Rev: %u\r",   __LINE__, StructWiFi->CountryCode, (StructWiFi->CountryCode >> 8), (StructWiFi->CountryCode >> 16));
  printf("[%5u]===============================================================\r", __LINE__);

  return;
}





/* $PAGE */
/* $TITLE=wifi_init_connection() */
/* ============================================================================================================================================================= *\
                                                                   Initialize Wi-Fi connection.
\* ============================================================================================================================================================= */
INT16 wifi_init_connection(struct struct_wifi *StructWiFi)
{
#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF all time.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_ON;   // may be modified for debug purposes.
#endif  // RELEASE_VERSION

  UINT8 Loop1UInt8;
  UINT8 RetryCount;

  INT16 ReturnCode;


  /* Initializations. */
  RetryCount = 0;
  StructWiFi->FlagHealth = FLAG_OFF;  // assume failure on entry.

  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "Initializing Wi-Fi connection with the following credentials:\r");
    uart_send(__LINE__, __func__, "Network name (SSID): [%s].\r", StructWiFi->NetworkName);
    uart_send(__LINE__, __func__, "Network password:    [%s].\r", StructWiFi->NetworkPassword);
  }


  /* Enable Wi-Fi Station mode. */
  cyw43_arch_enable_sta_mode();  // initialize Wi-Fi as a client (not as Access Point).


  /* The time-out next line may be increased or reduced, depending on your Wi-Fi infrastructure response speed. */
  // ReturnCode = cyw43_arch_wifi_connect_timeout_ms(SSID, Password, CYW43_AUTH_WPA2_AES_PSK, 6000);
  // ReturnCode = cyw43_arch_wifi_connect_timeout_ms(SSID, Password, CYW43_AUTH_WPA2_MIXED_PSK, 6000);
  ReturnCode = cyw43_arch_wifi_connect_blocking(StructWiFi->NetworkName, StructWiFi->NetworkPassword, CYW43_AUTH_WPA2_MIXED_PSK);
  if (ReturnCode != 0)
  {
    do
    {
      ++RetryCount;

      switch (ReturnCode)
      {
        case(CYW43_LINK_DOWN):
          printf("[%5u] - Error: Link down\r", __LINE__);
        break;

        case(CYW43_LINK_JOIN):
          printf("[%5u] - Error: Joining\r", __LINE__);
        break;

        case(CYW43_LINK_NOIP):
          printf("[%5u] - Error: No IP\r", __LINE__);
        break;

        case(CYW43_LINK_UP):
          printf("[%5u] - Link is up now!\r", __LINE__);
        break;

        case(CYW43_LINK_FAIL):
          printf("[%5u] - Error: Link fail\r", __LINE__);
        break;

        case(CYW43_LINK_NONET):
          printf("[%5u] - Error: Network fail\r", __LINE__);
        break;

        case(CYW43_LINK_BADAUTH):
          printf("[%5u] - Error: Bad auth\r", __LINE__);
        break;
      }

      
      if (RetryCount >= MAX_NETWORK_RETRIES)
      {
        printf("[%5u] - [%5u] - Wi-Fi connection failure - Retry count: %2u / %u   (aborting).\r", __LINE__, RetryCount, MAX_NETWORK_RETRIES);
        break;  // time-out.
      }


      /* While connection is not successful, blink PicoW's LED a number of times corresponding to the current retry count and wait until time-out. */
      printf("[%5u] - Wi-Fi connection failure - Retry count: %2u / %u   (retrying... return code: %d).\r", __LINE__, RetryCount, MAX_NETWORK_RETRIES, ReturnCode);
      wifi_blink(200, 300, RetryCount);

      /* No connection yet, wait and try again. */
      sleep_ms(3000);
    } while ((ReturnCode = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA)) != CYW43_LINK_UP);


    /* If we went out of "while" loop after MAX_NETWORK_RETRIES connection failures, fast blink PicoW's LED many times to indicate Wi-Fi connection failure. */
    if (RetryCount >= MAX_NETWORK_RETRIES)
    {
      /* In case of error, fast-blink Pico's LED many times to indicate Wi-Fi connection failure. */
      if (FlagLocalDebug) printf("[%5u] - Failed to establish a Wi-Fi connection.\r\r", __LINE__);
      wifi_blink(100, 100, 30);
      return ReturnCode;
    }
  }


  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                     Wi-Fi connection successful.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  printf("[%5u] - Wi-Fi connection succeeded (Number of retries: %u).\r", __LINE__, RetryCount);
  StructWiFi->FlagHealth = FLAG_ON;


  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                   Keep track of device MAC address.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  printf("[%5u] - Device MAC address: ", __LINE__);
  cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, StructWiFi->MacAddress);
  // cyw43_hal_get_mac(CYW43_HAL_MAC_WLAN0, StructWiFi->MacAddress);
  for (Loop1UInt8 = 0; Loop1UInt8 < sizeof(StructWiFi->MacAddress); ++Loop1UInt8)
  {
    printf("%2.2X", StructWiFi->MacAddress[Loop1UInt8]);
    if (Loop1UInt8 < (sizeof(StructWiFi->MacAddress) - 1))
      printf(":");
    else 
      printf("\r");
  }


  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                   Keep track of device Host name.
      Create "ExtraHostName" by appending the last 2 hex digits of the MAC address to the host name to make it more meaningful.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  memcpy(&StructWiFi->HostName[0], CYW43_HOST_NAME, sizeof(CYW43_HOST_NAME) - 1);
  memcpy(&StructWiFi->ExtraHostName[0], CYW43_HOST_NAME, sizeof(CYW43_HOST_NAME) - 1);
  sprintf(&StructWiFi->ExtraHostName[strlen(StructWiFi->ExtraHostName)], "%2.2X%2.2X", StructWiFi->MacAddress[sizeof(StructWiFi->MacAddress) - 2], StructWiFi->MacAddress[sizeof(StructWiFi->MacAddress) - 1]);
  printf("[%5u] - HostName:           <%s>\r", __LINE__, StructWiFi->HostName);
  printf("[%5u] - ExtraHostName:      <%s>\r", __LINE__, StructWiFi->ExtraHostName);
  netif_set_hostname(&cyw43_state.netif[CYW43_ITF_STA], StructWiFi->ExtraHostName);


  /* --------------------------------------------------------------------------------------------------------------------------- *\
                                                     Keep track of device IP address.
  \* --------------------------------------------------------------------------------------------------------------------------- */
  sprintf(StructWiFi->DeviceIPAddress, "%s", ip4addr_ntoa(netif_ip4_addr(netif_list)));
  printf("[%5u] - Device IP Address:  <%s>\r", __LINE__, StructWiFi->DeviceIPAddress);

  /* Fast blink Pico's LED 5 times to indicate wi-fi successful connection. */
  wifi_blink(100, 100, 5);

  return 0;
}
