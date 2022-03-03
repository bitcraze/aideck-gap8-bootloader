#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#     ||          ____  _ __
#  +------+      / __ )(_) /_______________ _____  ___
#  | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
#  +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
#   ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
#
#  Copyright (C) 2022 Bitcraze AB
#
#  AI-deck demo
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  You should have received a copy of the GNU General Public License along with
#  this program; if not, write to the Free Software Foundation, Inc., 51
#  Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#  Used for bootloading GAP8 during development

import argparse
import time
import socket,os,struct, time
import numpy as np
import hashlib
import binascii
import sys


# Args for setting IP/port of AI-deck. Default settings are for when
# AI-deck is in AP mode.
parser = argparse.ArgumentParser(description='Connect to AI-deck JPEG streamer example')
parser.add_argument("-n",  default="192.168.4.1", metavar="ip", help="AI-deck IP")
parser.add_argument("-p", type=int, default='5000', metavar="port", help="AI-deck port")
parser.add_argument('image', metavar='image', help='firmware image to flash')
args = parser.parse_args()

deck_port = args.p
deck_ip = args.n
imageName = args.image

print("Connecting to socket on {}:{}...".format(deck_ip, deck_port))
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect((deck_ip, deck_port))
print("Socket connected")

imgdata = None
data_buffer = bytearray()

import cv2

start = time.time()
count = 0

class CPXTarget:
  """
  List of CPX targets
  """
  STM32 = 1
  ESP32 = 2
  HOST = 3
  GAP8 = 4    

class CPXFunction:
  """
  List of CPX targets
  """
  SYSTEM = 1
  CONSOLE = 2
  CRTP = 3
  WIFI_CTRL = 4
  APP = 5
  TEST = 0x0E
  BOOTLOADER = 0x0F

class CPXPacket(object):
    """
    A packet with routing and data
    """

    def __init__(self, function=0, destination=0, source=CPXTarget.HOST, data=bytearray(), wireHeader=None):
        """
        Create an empty packet with default values.
        """
        self.data = data
        self.source = source
        self.destination = destination
        self.function = function
        self._wireHeaderFormat = "<HBB"
        self.length = 0
        self.lastPacket = False
        if wireHeader:
            [self.length, targetsAndFlags, self.function] = struct.unpack(self._wireHeaderFormat, wireHeader)
            self.destination = (targetsAndFlags >> 3) & 0x07
            self.source = targetsAndFlags & 0x07
            self.lastPacket = targetsAndFlags & 0x40 != 0

    def _get_wire_data(self):
        """Create raw data to send via the wire"""
        raw = bytearray()
        # This is the length excluding the 2 byte legnth
        wireLength = len(self.data) + 2 # 2 bytes for CPX header
        targetsAndFlags = ((self.source & 0x7) << 3) | (self.destination & 0x7)
        if self.lastPacket:
          targetsAndFlags |= 0x40
        #print(self.destination)
        #print(self.source)
        #print(targets)
        function = self.function & 0xFF
        raw.extend(struct.pack(self._wireHeaderFormat, wireLength, targetsAndFlags, function))
        raw.extend(self.data)
        
        # We need to handle this better...
        if (wireLength > 1022):
          raise "Cannot send this packet, the size is too large!"

        return raw

    def __str__(self):
        """Get a string representation of the packet"""
        return "{:02X}->{:02X}/{:02X}".format(self.source, self.destination, self.function)

    wireData = property(_get_wire_data, None)

class CPX(object):
  """
  A packet with routing and data
  """

  def __init__(self, socket):
    self._socket = socket

  def _rx_bytes(self, size):
    data = bytearray()
    while len(data) < size:
      #print(size - len(data))
      data.extend(self._socket.recv(size-len(data)))
    return data

  def send(self, packet):
    self._socket.send(packet.wireData)

  def receive(self):
    header = self._rx_bytes(4)
    packet = CPXPacket(wireHeader=header)
    packet.data = self._rx_bytes(packet.length - 2) # remove routing info here
    return packet

  def transaction(self, packet):
    self.send(packet)
    return self.receive()

class GAP8Bootloader:
  def __init__(self, cpx):
    self._cpx = cpx

  def getVersion(self):
    version = self._cpx.transaction(CPXPacket(destination=CPXTarget.GAP8,
                                              function=CPXFunction.BOOTLOADER,
                                              data=bytearray([0x00])))
    return version.data[1:]

  def readFlash(self, start, count):
    cmd = struct.pack("<BII", 0x03, start, count)
    readPacket = CPXPacket(destination=CPXTarget.GAP8, function=CPXFunction.BOOTLOADER, data=cmd)

    self._cpx.send(readPacket)
    totalBytesRead = 0
    totalRead = bytearray()
    while (totalBytesRead < size):
      readAnswer = self._cpx.receive()
      totalRead.extend(readAnswer.data)
      totalBytesRead += readAnswer.length
    return totalRead

  def MD5Flash(self, start, count):
    md5 = self._cpx.transaction(CPXPacket(destination=CPXTarget.GAP8,
                                          function=CPXFunction.BOOTLOADER,
                                          data=struct.pack("<BII", 0x04, start, count)))
    return md5.data[1:]

  def startApplication(self):
    self._cpx.send(CPXPacket(destination=CPXTarget.GAP8,
                            function=CPXFunction.BOOTLOADER,
                            data=struct.pack("<B", 0x06)))

  def writeFlash(self, start, data):
    cmd = struct.pack("<BII", 0x02, start, len(data))
    cmdPacket = CPXPacket(destination=CPXTarget.GAP8, function=CPXFunction.BOOTLOADER, data=cmd)

    self._cpx.send(cmdPacket)

    totalBytesWritten = 0
    maxChunkSize = 512
    while (totalBytesWritten < len(data)):
      nextChunk = min(maxChunkSize, len(data) - totalBytesWritten)
      print("We're at {}, next chunk is {} bytes".format(totalBytesWritten, nextChunk))
      fwWritePacket = CPXPacket(destination=CPXTarget.GAP8,
                                function=CPXFunction.BOOTLOADER,
                                data=data[totalBytesWritten:totalBytesWritten+nextChunk])
      self._cpx.send(fwWritePacket)
      totalBytesWritten += nextChunk

def hex_hump(arr, start=None):
  i = 0
  if (start != None):
    print("{:08X}: ".format(start + i), end='')
  for b in arr:
    i+=1
    print("{:02X} ".format(b), end='')
    if (i % 16 == 0):
      print("")
      if (start != None):
        print("{:08X}: ".format(start + i), end='')
  print("")

## Main things!

cpx = CPX(client_socket)
bootloader = GAP8Bootloader(cpx)

version = bootloader.getVersion()

print("GAP8 bootloader is version 0x{:02X}".format(version[0]))

flashAppStart = 0x40000

fw = bytearray()
with open(imageName, "rb") as f:
  fw.extend(f.read())

print("Firmware is {} bytes".format(len(fw)))
fwMD5 = hashlib.md5(fw)
print("MD5: {}".format(fwMD5.hexdigest()))
bootloader.writeFlash(flashAppStart, fw)

gap8CalcMD5 = bootloader.MD5Flash(flashAppStart, len(fw))
print(binascii.hexlify(gap8CalcMD5))

if gap8CalcMD5 == fwMD5.digest():
  print("Flash OK: Firmware MD5 matches!")
  bootloader.startApplication()
else:
  print("Flash FAIL: Firmware MD5 does NOT match!")
