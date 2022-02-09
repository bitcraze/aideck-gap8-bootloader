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

#if 0
#define DEBUG_PRINTF printf
#else
#define DEBUG_PRINTF(...) ((void) 0)
#endif /* DEBUG */

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

  DEBUG_PRINTF("Size left = %u, currentBase=0x%X\n", sizeLeft, currentBaseAddress);
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
  DEBUG_PRINTF("Read completed\n");
}

static uint8_t buffer[SIZE_OF_MD5_BUFER];
static MD5_CTX ctx;

uint32_t bl_handleMD5Command(ReadIn_t * info, MD5Out_t * dataout) {
  uint32_t sizeLeft;
  uint32_t currentBaseAddress;
  uint32_t chunkSize;

  MD5_Init(&ctx);

  sizeLeft = info->size;
  currentBaseAddress = info->start;
  chunkSize = 0;

  DEBUG_PRINTF("Calculating MD5 for %u bytes @ 0x%X\n", sizeLeft, currentBaseAddress);

  do {
    chunkSize = sizeLeft < SIZE_OF_MD5_BUFER ? sizeLeft : SIZE_OF_MD5_BUFER;
    flash_read(currentBaseAddress, buffer, chunkSize);
    MD5_Update(&ctx, buffer, chunkSize);    
    currentBaseAddress += chunkSize;
    sizeLeft -= chunkSize;
  } while (sizeLeft > 0);

  MD5_Final(dataout->md5, &ctx);

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
  DEBUG_PRINTF("Erasing flash from 0x%X to 0x%X...\n", currentBaseAddress, currentBaseAddress + sizeLeft);
  flash_erase(currentBaseAddress, sizeLeft);
  DEBUG_PRINTF("done!\n");

  DEBUG_PRINTF("Start update of size %ub @ 0x%X\n", sizeLeft, currentBaseAddress);
  do {
    // Read the next data packet
    uint32_t size = cpxReceivePacketBlocking(rxp);
    if (rxp->route.function == BOOTLOADER) {
      DEBUG_PRINTF("Writing chunk of %u@0x%X...\n", size, currentBaseAddress);
      flash_write(currentBaseAddress, rxp->data, size);
      DEBUG_PRINTF("done!\n");

      currentBaseAddress += size;
      sizeLeft -= size;
      DEBUG_PRINTF("Size left = %u, currentBase=0x%X\n", sizeLeft, currentBaseAddress);
    } else {
      DEBUG_PRINTF("We got a packet not for the bootloader while writing\n");
    }
  } while (sizeLeft > 0);
  DEBUG_PRINTF("Write completed\n");
}

#define MAX_NB_SEGMENT 16
#define L2_BUFFER_SIZE 512

static PI_L2 uint8_t l2_buffer[L2_BUFFER_SIZE]; 
typedef struct {
  uint32_t offset;
  uint32_t base;
  uint32_t size;
  uint32_t nBlocks;
} bin_segment_t;

typedef struct {
  uint32_t size;
  uint32_t nSegments;
  uint32_t entry;
  uint32_t entryBase;
  bin_segment_t segments[MAX_NB_SEGMENT];
} bin_header_t;

static void load_segment(const uint32_t application_offset, const bin_segment_t *segment)
{ 

    bool isL2Section = segment->base >= 0x1C000000 && segment->base < 0x1D000000;
    
    if(isL2Section) {
        DEBUG_PRINTF("Load segment to L2 memory at 0x%lX\n", segment->base);
        flash_read(application_offset + segment->offset, (void*) segment->base, segment->size);
    } else {
        DEBUG_PRINTF("Load segment to FC TCDM memory at 0x%lX (using a L2 buffer)\n", segment->base);
        size_t remaining_size = segment->size;
        uint32_t flashBase = application_offset + segment->offset;
        uint8_t * ramBase = (uint8_t *) segment->base;
        while (remaining_size > 0) {
            size_t iter_size = (remaining_size > L2_BUFFER_SIZE) ? L2_BUFFER_SIZE : remaining_size;
            DEBUG_PRINTF("Remaining size 0x%lX, it size %lu, 0x%X -> 0x%0X\n", remaining_size, iter_size, flashBase, ramBase);
            flash_read(flashBase, l2_buffer, iter_size);
            memcpy(ramBase, (void *) l2_buffer, iter_size);
            remaining_size -= iter_size;
            flashBase += iter_size;
            ramBase += iter_size;
        }
    }
}

static inline void __attribute__((noreturn)) jump_to_address(unsigned int address)
{
    void (*entry)() = (void (*)())(address);
    entry();
    while (1);
}

#define VECTOR_TABLE_SIZE 0x94
static bin_header_t header;

void bl_boot_to_application(void) {
  DEBUG_PRINTF("Booting to application in flash @ 0x%X\n", FIRMWARE_START_ADDRESS);

  flash_read(FIRMWARE_START_ADDRESS, (uint8_t *) &header, sizeof(bin_header_t));

  // Binary size is header + segments until the partition table starts
  // Segments is number of things to load
  // Entry point is where to jump after loading
  // Entry point base, no idea...

  // For each segment
    // base is where to load the segment
    // offset is where in the binary it is (i.e in flash, so 0x40000 + offset)
    // size is the size of what to load
    // nBlocks is the number of blocks, not sure how this is used, it doesn't correspond to flash
    
  // For this to work, the bootloader must be linked in FC L1 and L2 high up, so we can copy
  // in data from application to start. These functions are not public in the SDK, so they are
  // copied here.

  static PI_L2 uint8_t buff[VECTOR_TABLE_SIZE];
  bool differ_copy_of_irq_table = false;

  DEBUG_PRINTF("Binary size: %u\n", header.size);
  DEBUG_PRINTF("Segments: %u\n", header.nSegments);
  DEBUG_PRINTF("Entrypoint: 0x%X\n", header.entry);
  DEBUG_PRINTF("Entrypoint base?: 0x%X\n", header.entryBase);

  if (header.nSegments == 0 || header.nSegments > MAX_NB_SEGMENT) {
    DEBUG_PRINTF("Binary header seems to be not ok, not trying to boot to application\n");
    return;
  }

  for (unsigned int i=0; i < header.nSegments; i++) {
    bin_segment_t * segment = &header.segments[i];
    DEBUG_PRINTF("[%u]: base=0x%X\toffset=0x%X\tsize=0x%X\tnBlocks=%u\n", 
      i,
      segment->base,
      segment->offset,
      segment->size,
      segment->nBlocks);
  }

  // Start loading
  for (unsigned int i=0; i < header.nSegments; i++) {
    bin_segment_t * segment = &header.segments[i];

    DEBUG_PRINTF("Load segment %u: flash offset 0x%lX - size 0x%lX\n",
          i, segment->offset, segment->size);

    // Skip interrupt table entries
    if(segment->base == 0x1C000000) {
      differ_copy_of_irq_table = true;
      flash_read(FIRMWARE_START_ADDRESS + segment->offset, (uint8_t*) buff, VECTOR_TABLE_SIZE);
      segment->base += VECTOR_TABLE_SIZE;
      segment->offset += VECTOR_TABLE_SIZE;
      segment->size -= VECTOR_TABLE_SIZE;
    }

      load_segment(FIRMWARE_START_ADDRESS, segment);
  }

  //pi_flash_close(flash);
    
  DEBUG_PRINTF("Disable global IRQ and timer interrupt\n");
  disable_irq();
  NVIC_DisableIRQ(SYSTICK_IRQN);
   
  if(differ_copy_of_irq_table)
  {
    DEBUG_PRINTF("Copy IRQ table whithout uDMA.\n");
    uint8_t *ptr = (uint8_t * ) 0x1C000000;
    for (size_t i = 0; i < VECTOR_TABLE_SIZE; i++)
    {
      ptr[i] = buff[i];
    }
  }
    
  DEBUG_PRINTF("Flush icache\n");
  SCBC_Type *icache = SCBC;
  icache->ICACHE_FLUSH = 1;
    
  printf("Jump to app entry point at 0x%lX\n", header.entry);
  jump_to_address(header.entry);
}
