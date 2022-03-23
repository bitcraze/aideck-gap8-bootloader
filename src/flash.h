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
 * flash.h - Interface for Hyperflash
 */

#include "com.h"

#ifndef __FLASH_H__
#define __FLASH_H__

// TODO: Set this size exactly
#define FLASH_BUFFER_SIZE (64)

void flash_init(void);

void flash_write(uint32_t addr, uint8_t * in_data, unsigned int len);

void flash_read(uint32_t addr, uint8_t * out_data, unsigned int len);

void flash_erase_sector(uint32_t addr);

#endif