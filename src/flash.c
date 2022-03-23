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
 * flash.c - Interface for Hyperflash
 */

#include "pmsis.h"

#include <bsp/flash/hyperflash.h>

#include "flash.h"

static pi_device_t flash_dev;
static struct pi_flash_info flash_info;
static struct pi_hyperflash_conf flash_conf;
static uint32_t flash_start;

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

void flash_init() {
  open_flash(&flash_dev);

  pi_flash_ioctl(&flash_dev, PI_FLASH_IOCTL_INFO, (void *)&flash_info);
}

void flash_write(uint32_t addr, uint8_t * in_data, unsigned int len) {
  pi_flash_program(&flash_dev, addr, in_data, len);
}

void flash_read(uint32_t addr, uint8_t * out_data, unsigned int len) {
  pi_flash_read(&flash_dev, addr, out_data, len);
}

void flash_erase_sector(uint32_t addr) {
  pi_flash_erase_sector(&flash_dev, addr);
}