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
 * cpx.c - Interface for CPX stack
 */

#include <stdint.h>
#include "com.h"
#include "pmsis.h"
#include "cpx.h"

static packet_t txp;
static packet_t rxp;

// Return length of packet
uint32_t cpxReceivePacketBlocking(CPXPacket_t * packet) {
  com_read(&rxp);
  uint32_t size = rxp.len - sizeof(CPXRouting_t);
  memcpy(&packet->route, rxp.data, rxp.len);
  return size;
}

void cpxSendPacketBlocking(CPXPacket_t * packet, uint32_t size) {
  uint32_t wireLength = size + sizeof(CPXRouting_t);
  txp.len = wireLength;
  memcpy(txp.data, &packet->route, wireLength);
  com_write(&txp);
}
