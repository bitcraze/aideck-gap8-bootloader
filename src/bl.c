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
 * bl.c - Bootloader logic
 */

#include "pmsis.h"

#include "bsp/crc/md5.h"

#include "flash.h"
#include "bl.h"
#include "cpx.h"

#define SIZE_OF_MD5_BUFER (512)

#define FIRMWARE_START_ADDRESS (0x40000)

uint16_t bl_handleVersionCommand(VersionOut_t * out) {
  out->version = 1;

  return 1;
}

void bl_handleReadCommand(ReadIn_t * info,  CPXPacket_t * txp) {
  uint32_t sizeLeft;
  uint32_t currentBaseAddress;
  uint32_t chunkSize;

  sizeLeft = info->size;
  currentBaseAddress = info->start;
  chunkSize = 0;

  printf("Size left = %u, currentBase=0x%X\n", sizeLeft, currentBaseAddress);
  do {
    chunkSize = sizeLeft < sizeof(txp->data) ? sizeLeft : sizeof(txp->data);
    //printf("Reading chunk of %u\n", chunkSize);
    flash_read(currentBaseAddress, txp->data, chunkSize);
    //printf("have read flash\n");

    //printf("%02X->%02X (%02X)\n", txp->route.source, txp->route.destination, txp->route.function);

    //printf("sendin data\n");
    cpxSendPacketBlocking(txp, chunkSize);
    //printf("have sent data\n");

    currentBaseAddress += chunkSize;
    sizeLeft -= chunkSize;
    //printf("Size left = %u, currentBase=0x%X\n", sizeLeft, currentBaseAddress);
  } while (sizeLeft > 0);
  printf("Read completed\n");
}

uint32_t bl_handleMD5Command(ReadIn_t * info, MD5Out_t * dataout) {
  uint32_t sizeLeft;
  uint32_t currentBaseAddress;
  uint32_t chunkSize;
  uint8_t * buffer = (uint8_t *) pi_l2_malloc(SIZE_OF_MD5_BUFER);
  MD5_CTX * ctx = (MD5_CTX *) pi_l2_malloc(sizeof(MD5_CTX));
  if (buffer == NULL) {
    // TODO: Should we return an error here?
    printf("Could not malloc MD5 buffer\n");
    pmsis_exit(1);
  }

  MD5_Init(ctx);

  sizeLeft = info->size;
  currentBaseAddress = info->start;
  chunkSize = 0;

  do {
    chunkSize = sizeLeft < SIZE_OF_MD5_BUFER ? sizeLeft : SIZE_OF_MD5_BUFER;
    flash_read(currentBaseAddress, buffer, chunkSize);
    MD5_Update(ctx, buffer, chunkSize);    
    currentBaseAddress += chunkSize;
    sizeLeft -= chunkSize;
  } while (sizeLeft > 0);

  MD5_Final(dataout->md5, ctx);

  pi_l2_free(buffer, SIZE_OF_MD5_BUFER);
  pi_l2_free(ctx, sizeof(MD5_CTX));

  return sizeof(MD5Out_t);
}

void bl_handleWriteCommand(ReadIn_t * info,  CPXPacket_t * rxp, CPXPacket_t * txp) {

  // Sanity check data and return something

  // Read out and flash all the data
  // TODO: Auto-erase at boundry

  uint32_t sizeLeft;
  uint32_t currentBaseAddress;
  uint32_t chunkSize;

  sizeLeft = info->size;
  currentBaseAddress = info->start;
  chunkSize = 0;

  // Sanity check data
  printf("Erasing flash from 0x%X to 0x%X...\n", currentBaseAddress, currentBaseAddress + sizeLeft);
  flash_erase(currentBaseAddress, sizeLeft);
  printf("done!\n");

  printf("Start update of size %ub @ 0x%X\n", sizeLeft, currentBaseAddress);
  do {
    // Read the next data packet
    printf("Waiting for packet...\n");
    uint32_t size = cpxReceivePacketBlocking(rxp);
    printf("got it!\n");
    if (rxp->route.function == BOOTLOADER) {
      printf("Writing chunk of %u@0x%X...\n", size, currentBaseAddress);
      flash_write(currentBaseAddress, rxp->data, size);
      printf("done!\n");

      currentBaseAddress += size;
      sizeLeft -= size;
      printf("Size left = %u, currentBase=0x%X\n", sizeLeft, currentBaseAddress);
    } else {
      printf("We got a packet not for the bootloader while writing\n");
    }
  } while (sizeLeft > 0);
  printf("Write completed\n");
}
