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

#if 0
#define DEBUG_PRINTF printf
#else
#define DEBUG_PRINTF(...) ((void) 0)
#endif /* DEBUG */

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
static CPXPacket_t txp;
static CPXPacket_t rxp;

extern void pi_bsp_init(void);

void bl_task( void *parameters )
{
  uint8_t len;
  uint8_t count;

  vTaskDelay(1000);

  while (1) {
    uint32_t size = cpxReceivePacketBlocking(&rxp);
    
    DEBUG_PRINTF(">> 0x%02X->0x%02X (0x%02X) (size=%u)\n", rxp.route.source, rxp.route.destination, rxp.route.function, size);
    if (rxp.route.function == BOOTLOADER) {
      BLPacket_t * blpRx = (BLPacket_t*) rxp.data;
      BLPacket_t * blpTx = (BLPacket_t*) txp.data;

      DEBUG_PRINTF("Received command [0x%02X] for bootloader\n", blpRx->cmd);

      uint16_t replySize = 0;

      // Fix the header of the outgoing answer
      txp.route.source = GAP8;
      txp.route.destination = rxp.route.source;
      txp.route.function = BOOTLOADER;

      switch(blpRx->cmd) {
        case BL_CMD_VERSION:
          replySize = bl_handleVersionCommand((VersionOut_t*) blpTx->data);
          break;
        case BL_CMD_READ:
          bl_handleReadCommand( (ReadIn_t*) blpRx->data, &txp);
          break;
        case BL_CMD_WRITE:
          bl_handleWriteCommand( (ReadIn_t*) blpRx->data, &rxp, &txp);
          break;          
        case BL_CMD_MD5:
          replySize = bl_handleMD5Command((ReadIn_t*) blpRx->data, (MD5Out_t *) blpTx->data);
          break;
        case BL_CMD_JMP:
          bl_boot_to_application();
          break;  
        default:
          printf("Not handling bootloader command [0x%02X]\n", blpRx->cmd);
      }

      if (replySize > 0) {
        // Include command header byte
        blpTx->cmd = blpRx->cmd;
        replySize += sizeof(BLCommand_t);

        DEBUG_PRINTF("Sending back reply of %u bytes\n", replySize);

        cpxSendPacketBlocking(&txp, replySize);
      }
      
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

    printf("\n-- GAP8 bootloader --\n");

    flash_init();

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