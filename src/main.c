#include <stdio.h>
#include <stdint.h>

#include "pmsis.h"

#include "com.h"

#define LED_PIN 2

static pi_device_t led_gpio_dev;

void hb_task( void *parameters )
{
    ( void ) parameters;
    char *taskname = pcTaskGetName( NULL );

    // Initialize the LED pin
    pi_gpio_pin_configure(&led_gpio_dev, LED_PIN, PI_GPIO_OUTPUT);

    const TickType_t xDelay = 500 / portTICK_PERIOD_MS;

    while (1) {
      pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 1);
      vTaskDelay( xDelay );
      pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 0);
      vTaskDelay( xDelay );
    }
}

static routed_packet_t txp;
static routed_packet_t rxp;

void test_task( void *parameters )
{
  uint8_t len;
  uint8_t count;

  while (1) {
    com_read((packet_t*) &rxp);
    if (rxp.dst == 0x22) {// Test
      printf("Received test request from ");
      switch(rxp.src >> 4) {
        case 0x0: printf(" STM32\n"); break;
        case 0x1: printf(" ESP32\n"); break;
        default: printf(" UNKNOWN\n"); break;
      }

      printf("Run test: ");
      switch (rxp.data[0]) {
        case 0:
          // Sink, do nothing
          printf("sink\n");
          break;
        case 1:
          // echo
          printf("echo\n");
          memcpy(&txp, &rxp, rxp.len + 4);
          txp.src = rxp.dst;
          txp.dst = rxp.src;
          com_write((packet_t*) &txp);
          break;
        case 2:
          // Source
          len = rxp.data[2];
          count = rxp.data[1];
          txp.len = len;
          txp.src = rxp.dst;
          txp.dst = rxp.src;

          printf("source %u packets of size %u\n", count, len);

          for (int i = 0; i < count; i++) {
            for (int j = 0; j < len; j++) {
              txp.data[j] = j;
            }
            com_write((packet_t*) &txp);
          }
          break;
        default:
          printf(" unknown!\n");
      }
    } else {
      printf("Received packet not for test!\n");
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
    printf("[UART] open failed !\n");
    pmsis_exit(-1);
  }

    printf("\nBootloader is starting up...with 4.7.0!\n");
    printf("FC at %u MHz\n", pi_freq_get(PI_FREQ_DOMAIN_FC)/1000000);

    printf("Starting up tasks...\n");

    BaseType_t xTask;

    xTask = xTaskCreate( hb_task, "hb_task", configMINIMAL_STACK_SIZE * 2,
                         NULL, tskIDLE_PRIORITY + 1, NULL );
    if( xTask != pdPASS )
    {
        printf("HB task did not start !\n");
        pmsis_exit(-1);
    }

    com_init();

    xTask = xTaskCreate( test_task, "test_task", configMINIMAL_STACK_SIZE * 4,
                         NULL, tskIDLE_PRIORITY + 1, NULL );
    if( xTask != pdPASS )
    {
        printf("Test task did not start !\n");
        pmsis_exit(-1);
    }

    while(1)
    {
        pi_yield();
    }
}

int main(void)
{
  // Initialize the PAD config (taken from BSP)
  uint32_t pads_value[] = {0x00055500, 0x0f000000, 0x003fffff, 0x00000000};
  pi_pad_init(pads_value);

  // Increase the FC freq to 250 MHz
  pi_freq_set(PI_FREQ_DOMAIN_FC, 250000000);
  pi_pmu_voltage_set(PI_PMU_DOMAIN_FC, 1200);

  //printf("\n\tGAP8 bootloader\n\n");
  return pmsis_kickoff((void *)start_bootloader);
}