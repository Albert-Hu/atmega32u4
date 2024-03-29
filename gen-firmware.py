from __future__ import print_function

import argparse, os

def pack(application, bootloader, output):
  assert os.path.isfile(application), '{} doesn\'t exist.'.format(application)
  assert os.path.getsize(application) <= 0x7000, '{} large then 28KB.'
  assert os.path.isfile(bootloader), '{} doesn\'t exist.'.format(bootloader)
  assert os.path.getsize(bootloader) <= 0x1000, '{} should equal or less than 4KB'
  # create a buffer to build output file.
  firmware = bytes()
  # read the application file
  with open(application, 'rb') as file:
    data = file.read()
    firmware = firmware + data + (b'\xFF' * (0x7000 - len(data)))
  # read the bootloader file
  with open(bootloader, 'rb') as file:
    data = file.read()
    firmware = firmware + data + (b'\xFF' * (0x1000 - len(data)))
  # write the buffer to output file
  with open(output, 'wb') as file:
    file.write(bytearray(firmware))

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument('-a', '--application', type=str, required=True, help='The application file.')
  parser.add_argument('-b', '--bootloader', type=str, required=True, help='The bootloader file.')
  parser.add_argument('-o', '--output', type=str, required=True, help='The output file.')
  args = parser.parse_args()
  pack(**vars(args))
