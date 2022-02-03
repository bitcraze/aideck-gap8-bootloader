#include "pmsis.h"

#include <bsp/flash/hyperflash.h>

#include "flash.h"

static pi_device_t flash_dev;
static struct pi_flash_info flash_info;
static struct pi_hyperflash_conf flash_conf;
static uint32_t flash_start;

// L2 size if 512 kB
static uint8_t * flash_l2_buffer;

static void open_flash(pi_device_t *flash)
{
  pi_hyperflash_conf_init(&flash_conf);
  pi_open_from_conf(flash, &flash_conf);

  if (pi_flash_open(flash) < 0)
  {
    printf("unable to open flash device\n");
    pmsis_exit(PI_FAIL);
  }
}

#define BUFFER_SIZE 16

void flash_init() {
  open_flash(&flash_dev);

  pi_flash_ioctl(&flash_dev, PI_FLASH_IOCTL_INFO, (void *)&flash_info);

  flash_start = ((flash_info.flash_start + flash_info.sector_size - 1) &
                ~(flash_info.sector_size - 1)) +
               128;

  printf("Flash start on: 0x%X\n", flash_info.flash_start);
  printf("Flash start on: 0x%X\n", flash_start);
  printf("Sector size is: 0x%X\n", flash_info.sector_size);

}

void flash_write(uint32_t addr, uint8_t * in_data, unsigned int len) {

  uint8_t * alignedData = (uint8_t *) pi_l2_malloc(len);
  
  memcpy(alignedData, in_data, len);

  pi_flash_program(&flash_dev, addr, alignedData, len);

  pi_l2_free(alignedData, len);
}

void flash_read(uint32_t addr, uint8_t * out_data, unsigned int len) {
  // Copy data to L2 cache (needed for DMA)

  uint8_t * alignedData = (uint8_t *) pi_l2_malloc(len);

  pi_flash_read(&flash_dev, addr, alignedData, len);

  memcpy(out_data, alignedData, len);
  pi_l2_free(alignedData, len);
  //memcpy(data, flash_l2_buffer, len);
}

void flash_erase(uint32_t addr, unsigned int len) {
  // Copy data to L2 cache (needed for DMA)
  pi_flash_erase(&flash_dev, addr, len);
  //pi_flash_erase_sector(&flash_dev, addr);
  //memcpy(data, flash_l2_buffer, len);
}