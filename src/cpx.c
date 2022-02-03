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

bool cpxSendPacket(CPXPacket_t * packet, uint32_t timeoutInMS) {
  // TODO
}