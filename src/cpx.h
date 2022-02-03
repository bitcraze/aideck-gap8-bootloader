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
 * cpx.h - Interface for CPX stack
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "com.h"

typedef enum {
  STM32 = 1,
  ESP32 = 2,
  HOST = 3,
  GAP8 = 4
} __attribute__((packed)) CPXTarget_t; // Rename to Destination

typedef enum {
  SYSTEM = 1,
  CONSOLE = 2,
  CRTP = 3,
  WIFI_CTRL = 4,
  APP = 5,
  TEST = 0x0E,
  BOOTLOADER = 0x0F,
} __attribute__((packed)) CPXFunction_t;

typedef struct {
  CPXTarget_t destination : 4;
  CPXTarget_t source : 4;
  CPXFunction_t function;
} __attribute__((packed)) CPXRouting_t;

typedef struct {
    CPXRouting_t route;
    uint8_t data[MTU-2];
} __attribute__((packed)) CPXPacket_t;

// Return length of packet
uint32_t cpxReceivePacketBlocking(CPXPacket_t * packet);

void cpxSendPacketBlocking(CPXPacket_t * packet, uint32_t size);