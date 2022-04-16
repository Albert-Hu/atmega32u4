from __future__ import print_function

import argparse, os

def pack(app, bootloader, output):
  assert os.path.isfile(app), '{} doesn\'t exist.'.format(app)
  assert os.path.getsize(app) < 0x7000, '{} large then 28KB.'
  assert os.path.isfile(bootloader), '{} doesn\'t exist.'.format(bootloader)
  assert os.path.getsize(bootloader) == 0x1000, '{} should be 4KB'
  # create a buffer to build output file.
  binary = [0xff for _ in range(0x8000)]
  # read the app file
  with open(app, 'rb') as file:
    data = list(file.read())
    bytes_copy = os.path.getsize(app)
    binary[:bytes_copy] = data[:]
  # read the bootloader file
  with open(bootloader, 'rb') as file:
    data = list(file.read())
    binary[0x7000:] = data[:]
  # write the buffer to output file
  with open(output, 'wb') as file:
    file.write(bytearray(binary))

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument('-a', '--app', type=str, required=True, help='The app file.')
  parser.add_argument('-b', '--bootloader', type=str, required=True, help='The bootloader file.')
  parser.add_argument('-o', '--output', type=str, required=True, help='The output file.')
  args = parser.parse_args()
  pack(**vars(args))
