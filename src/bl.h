
#include "com.h"
#include "cpx.h"

#ifndef __BL_H__
#define __BL_H__

#define BL_PAYLOAD (MTU - 2)

#define BL_BYTE          (0xFF)

typedef enum {
  BL_CMD_VERSION = 0,
  BL_CMD_ERASEPAGE = 1,
  BL_CMD_WRITE = 2,
  BL_CMD_READ = 3,
  BL_CMD_MD5 = 4,
  BL_CMD_INFO = 5, // Include sector and MTU size here!
  BL_CMD_JMP = 6
} __attribute__((__packed__)) BLCommand_t;

typedef struct {
  BLCommand_t cmd;
  uint8_t data[BL_PAYLOAD - sizeof(BLCommand_t)];
} __attribute__((__packed__)) BLPacket_t;

typedef struct {
  uint8_t version;
} __attribute__((__packed__)) VersionOut_t;

typedef struct {
  uint32_t start;
  uint32_t size;
} __attribute__((__packed__)) ReadIn_t;

typedef struct {
  uint8_t md5[16];
} __attribute__((__packed__)) MD5Out_t;

uint16_t bl_handleVersionCommand(VersionOut_t * info);

void bl_handleReadCommand(ReadIn_t * info,  CPXPacket_t * txp);

void bl_handleWriteCommand(ReadIn_t * info,  CPXPacket_t * rxp, CPXPacket_t * txp);

uint32_t bl_handleMD5Command(ReadIn_t * info, MD5Out_t * dataout);

#endif