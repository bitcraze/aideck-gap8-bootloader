/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * AI-deck GAP8 second stage bootloader
 *
 * Copyright (C) 2022 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * main.c - AI-deck GAP8 second stage bootloader
 */

#include <stdio.h>
#include <stdint.h>

#include "pmsis.h"

#include "com.h"
#include "cpx.h"
#include "bl.h"
#include "flash.h"

#include "wifi.h"
#include "wifi_credentials.h"


#define LED_PIN 2

static pi_device_t led_gpio_dev;

void hb_task( void *parameters )
{
    ( void ) parameters;
    char *taskname = pcTaskGetName( NULL );

    // Initialize the LED pin
    pi_gpio_pin_configure(&led_gpio_dev, LED_PIN, PI_GPIO_OUTPUT);

    const TickType_t xDelay = 100 / portTICK_PERIOD_MS;

    while (1) {
      pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 1);
      vTaskDelay( xDelay );
      pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 0);
      vTaskDelay( xDelay );
    }
}

// These must be in L2 for uDMA to work
static CPXPacket_t * txp;
static CPXPacket_t * rxp;

extern void pi_bsp_init(void);

void setup_wifi(void) {
  printf("Sending wifi info...\n");
  txp->route.destination = ESP32;
  txp->route.source = GAP8;
  txp->route.function = WIFI_CTRL;

  WiFiCTRLPacket_t * wifiCtrl = (WiFiCTRLPacket_t*) txp->data;
  wifiCtrl->cmd = WIFI_CTRL_SET_SSID;
  memcpy(wifiCtrl->data, ssid, sizeof(ssid));
  cpxSendPacketBlocking(txp, sizeof(ssid) + 1);

  wifiCtrl->cmd = WIFI_CTRL_SET_KEY;
  memcpy(wifiCtrl->data, passwd, sizeof(passwd));
  cpxSendPacketBlocking(txp, sizeof(passwd) + 1);

  wifiCtrl->cmd = WIFI_CTRL_WIFI_CONNECT;
  cpxSendPacketBlocking(txp, sizeof(WiFiCTRLPacket_t));
}

void bl_task( void *parameters )
{
  uint8_t len;
  uint8_t count;

  vTaskDelay(1000);

  setup_wifi();

  while (1) {
    uint32_t size = cpxReceivePacketBlocking(rxp);
    
    printf(">> 0x%02X->0x%02X (0x%02X) (size=%u)\n", rxp->route.source, rxp->route.destination, rxp->route.function, size);
    if (rxp->route.function == BOOTLOADER) {
      BLPacket_t * blpRx = (BLPacket_t*) rxp->data;
      BLPacket_t * blpTx = (BLPacket_t*) txp->data;

      printf("Received command [0x%02X] for bootloader\n", blpRx->cmd);

      uint16_t replySize = 0;

      // Fix the header of the outgoing answer
      txp->route.source = GAP8;
      txp->route.destination = rxp->route.source;
      txp->route.function = BOOTLOADER;

      switch(blpRx->cmd) {
        case BL_CMD_VERSION:
          replySize = bl_handleVersionCommand((VersionOut_t*) blpTx->data);
          break;
        case BL_CMD_READ:
          bl_handleReadCommand( (ReadIn_t*) blpRx->data, txp);
          break;
        case BL_CMD_WRITE:
          bl_handleWriteCommand( (ReadIn_t*) blpRx->data, rxp, txp);
          break;          
        case BL_CMD_MD5:
          replySize = bl_handleMD5Command((ReadIn_t*) blpRx->data, (MD5Out_t *) blpTx->data);
          break;          
        default:
          printf("Not handling bootloader command [0x%02X]\n", blpRx->cmd);
      }

      if (replySize > 0) {
        // Include command header byte
        blpTx->cmd = blpRx->cmd;
        replySize += sizeof(BLCommand_t);


        cpxSendPacketBlocking(txp, replySize);
      }
      
    } else if (rxp->route.function == WIFI_CTRL) {
      if (rxp->data[0] == WIFI_CTRL_STATUS_WIFI_CONNECTED) {
        printf("Wifi connected (%u.%u.%u.%u)\n", rxp->data[1], rxp->data[2], rxp->data[3], rxp->data[4]);
      } else {
        printf("Not handling WIFI_CTRL [0x%02X]\n", rxp->data[0]);
      }
    } else {
      printf("Not handling function [0x%02X]\n", rxp->route.function);
    }
  }
}

void start_bootloader(void)
{
  struct pi_uart_conf conf;
  struct pi_device device;
  pi_uart_conf_init(&conf);
  conf.baudrate_bps =115200;

  pi_open_from_conf(&device, &conf);
  if (pi_uart_open(&device))
  {
    pmsis_exit(-1);
  }

    printf("\nBootloader is starting up...\n");
    printf("FC at %u MHz\n", pi_freq_get(PI_FREQ_DOMAIN_FC)/1000000);

    flash_init();

    // These must be in L2 for uDMA to work
    txp = (CPXPacket_t *) pi_l2_malloc((uint32_t)sizeof(CPXPacket_t));
    rxp = (CPXPacket_t *) pi_l2_malloc((uint32_t)sizeof(CPXPacket_t));

    if (txp == NULL || rxp == NULL)
    {
      printf("Could not allocate txp and/or rxp, nothing to do now\n");
      pmsis_exit(1);
    }

    printf("Starting up tasks...\n");

    BaseType_t xTask;

    xTask = xTaskCreate( hb_task, "hb_task", configMINIMAL_STACK_SIZE,
                         NULL, tskIDLE_PRIORITY + 1, NULL );
    if( xTask != pdPASS )
    {
        printf("HB task did not start !\n");
        pmsis_exit(-1);
    }

    com_init();

    xTask = xTaskCreate( bl_task, "bootloader task", configMINIMAL_STACK_SIZE * 3,
                         NULL, tskIDLE_PRIORITY + 1, NULL );
    if( xTask != pdPASS )
    {
        printf("Bootloader task did not start !\n");
        pmsis_exit(-1);
    }

    while(1)
    {
        pi_yield();
    }
}

int main(void)
{
  pi_bsp_init();

  return pmsis_kickoff((void *)start_bootloader);
}