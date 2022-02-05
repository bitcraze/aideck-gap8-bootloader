# Utility for showing information about segments in a GAP8 flash file (the ones that end in .img).
#
# The script will check if any segments overlap with the bootloader on the GAP8
# which will cause problems when loading it after flashing (i.e app will overwrite
# bootloader during jumping to application).
#
# Each information field in the flash image is 32-bits (unsigned) and is organized as follows:
#   The first 4 integers are the header for the information and contains the following:
#     * Size of binary (everything in the image file until the partition table)
#     * Number of segments in image
#     * Entrypoint where execution of the binary should start
#     * Unknown (base address of entry point or interrupt vector location?)
#   The following data is 4 integers per segment in the image (the number above in the header):
#     * Base in flash: The offset in flash (from where the img is stored) where the segment data starts
#     * Base in RAM: The offset in RAM where the data should be loaded
#     * Size: The size of the segment (i.e size of data that should be copied into RAM from flash)
#     * nBlocks: Number of blocks, not sure how this is used
#
# The limits for the regions used by the user application is according to the linker file for the bootloader
# where the L1 memory area used starts at 0x1b002000 and the L2 memory area used starts at 0x1C060000.
# If the user application has data that will be copied here the jumping to the application will most
# likely fail, since the bootloader will be overwritten while executing.
#
# If you application fails this test, then the solution is to move more data into the heap on L2, since
# this can safely be used once your application starts and can then overwrite the bootloader. 

import sys
import struct

startSBLInL2 = 0x1C060000
startSBLInL1 = 0x1b002000

if len(sys.argv) != 2:
  print("usage: {} [flash-image]".format(sys.argv[0]))
  sys.exit(1)

print("Showing info for {}".format(sys.argv[1]))
fw = bytearray()
with open(sys.argv[1], "rb") as f:
  fw.extend(f.read())

[size, nSegments, entry, entryBase] = struct.unpack("IIII", fw[0:16])

print("Size on disk: {}".format(len(fw)))
print("Size: {}".format(size))
print("nSegments: {}".format(nSegments))
print("Entrypoint: 0x{:X}".format(entry))
print("Entrypoint base?: 0x{:X}".format(entryBase))
print("")

if nSegments == 0 or nSegments > 16:
  print("The number of segments is out of bounds, is this really a GAP8 flash image?")
  sys.exit(1)

totalL2 = 0
totalL1 = 0

segmentSBLOverlap = []

i = 16
for si in range(nSegments):
  [base, offset, size, nBlocks] = struct.unpack("IIII", fw[i:i+16])
  info = "[{}]\tbase=0x{:X}\toffset=0x{:X}\tsize=0x{:X}\tnBlocks={}".format(
    si, base, offset, size, nBlocks
  ) 
  print(info)

  if offset >= 0x1C000000 and offset < 0x1D000000:
    totalL2 += size
    if offset + size >= startSBLInL2:
      segmentSBLOverlap.append(info)
  if offset >= 0x1B000000 and offset < 0x1C000000:
    totalL1 += size
    if offset + size >= startSBLInL1:
      segmentSBLOverlap.append(info)

  i += 16

print("")
print("Total L1 size: 0x{:X} ({})".format(totalL1, totalL1))
print("Total L2 size: 0x{:X} ({})".format(totalL2, totalL2))
print("")

if len(segmentSBLOverlap) > 0:
  print("The following segments overlap with bootloader:")
  for info in segmentSBLOverlap:
    print(info)
  sys.exit(1)

print("No overlap with bootloader, its all fine!")