/*

Info from flash:

Flash start: 0x40000
Sector size: 0x40000 = 262 144 b
Fancy test@: 0x40080

*/


#include "com.h"

#ifndef __FLASH_H__
#define __FLASH_H__

// TODO: Set this size exactly
#define FLASH_BUFFER_SIZE (64)

void flash_init(void);

//void flash_erase_page()

void flash_write(uint32_t addr, uint8_t * in_data, unsigned int len);

void flash_read(uint32_t addr, uint8_t * out_data, unsigned int len);

#endif