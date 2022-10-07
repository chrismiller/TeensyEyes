#!/usr/bin/python

"""
Image converter for 'Uncanny Eyes' project.  Generates tables for
eyeData.h file.  Requires Pillow imaging and numpy libraries. Expects
six image files: sclera, iris, upper and lower eyelid map (symmetrical),
upper and lower eyelid map (asymmetrical L/R) -- defaults will be used
for each if not specified.  Also generates polar coordinate map for iris
rendering (pass diameter -- must be an even value -- as 7th argument),
pupil is assumed round unless pupilMap.png image is present.
Output is to stdout; should be redirected to file for use.
"""

import sys
import math
import os
import numpy as np
from PIL import Image
from hextable import HexTable

SCREEN_WIDTH = 240
SCREEN_HEIGHT = 240

DEFAULT_IRIS_SIZE = 150

M_PI = math.pi
M_PI_2 = math.pi / 2.0


def getParam(index: int, default: str) -> str:
  """
  Returns a string command line parameter, or a default value if the parameter does not exist
  """
  try:
    return sys.argv[index]
  except IndexError:
    return default


def outputImage(filename: str, image: Image, name: str) -> None:
  """
  Writes an image file out to a C style array of uint16_t in 565 RGB format
  """
  width = image.size[0]
  height = image.size[1]

  print('  // {} - {}x{}, 16 bit 565 RGB'.format(filename, width, height))
  print('  constexpr uint16_t {} = {};'.format(name + 'Width', width))
  print('  constexpr uint16_t {} = {};'.format(name + 'Height', height))
  print('  const uint16_t {}[{} * {}] PROGMEM = {{'.format(name, name + 'Width', name + 'Height'))

  pixels = image.load()
  hexTable = HexTable(width * height, 12, 4, 2)
  # Convert 24-bit image to 16 bits:
  for y in range(height):
    for x in range(width):
      p = pixels[x, y]  # Pixel data (tuple)
      hexTable.write(
        ((p[0] & 0b11111000) << 8) |  # Convert 24-bit RGB
        ((p[1] & 0b11111100) << 3) |  # to 16-bit value with
        (p[2] >> 3))  # 5/6/5-bit packing

  print('  const Image {}Image({}, {}, {});'.format(name, name, name + 'Width', name + 'Height'))
  print()


def outputSclera(arg: int) -> None:
  """
  Load, validate and output the sclera image file
  """
  filename = getParam(arg, 'sclera.png')
  image = Image.open(filename)
  image = image.convert('RGB')
  width = image.size[0]
  height = image.size[1]

  # Check the image is square
  if width != height:
    sys.stderr.write('{} is {}x{} but it needs to be square'.format(filename, width, height))
    exit(1)

  outputImage(filename, image, 'sclera')


def outputIris(arg: int) -> None:
  """
  Load, validate and output the iris image file
  """
  filename = getParam(arg, 'iris.png')
  image = Image.open(filename)
  image = image.convert('RGB')
  width = image.size[0]
  height = image.size[1]

  if width > 512 or height > 128:
    sys.stderr.write('Iris image can\'t exceed 512 pixels wide or 128 pixels tall')
    exit(1)

  outputImage(filename, image, 'iris')


def outputEyelid(arg: int, defaultFilename: str, tableName: str) -> None:
  """
  Load, validate and output an eyelid threshold lookup table
  """
  filename = getParam(arg, defaultFilename)

  image = Image.open(filename)
  if (image.size[0] != SCREEN_WIDTH) or (image.size[1] != SCREEN_HEIGHT):
    sys.stderr.write('{} dimensions must match screen size of {}x{}'.format(filename, SCREEN_WIDTH, SCREEN_HEIGHT))
    exit(1)
  image = image.convert('L')
  pixels = image.load()

  print('  // An array of vertical start (inclusive) and end (exclusive) locations for each {} eyelid column'.format(filename))
  print('  const uint8_t {}[screenWidth * 2] PROGMEM = {{'.format(tableName))

  hexTable = HexTable(image.size[0] * 2, 16, 2, 2)
  for x in range(image.size[0]):
    found_start = False
    found_end = False
    y = 0
    for y in range(image.size[1]):
      if not found_start:
        if pixels[x, y] > 0:
          hexTable.write(y)
          found_start = True
      elif not found_end:
        if pixels[x, y] == 0:
          hexTable.write(y)
          found_end = True
          break
    if not found_start:
      sys.stderr.write('{} doesn''t have an eyelid in column {}'.format(filename, y))
      exit(1)
    if not found_end:
      # The eyelid goes all the way to the bottom of this column
      hexTable.write(SCREEN_HEIGHT)


def screenToMap(mapRadius: int, eyeRadius: int, value: int) -> float:
  """
  Scale a measurement in screen pixels to polar map pixels
  """

  return math.atan2(value, math.sqrt(eyeRadius * eyeRadius - value * value)) / M_PI_2 * mapRadius


def mapToScreen(mapRadius: int, eyeRadius: int, value: int) -> float:
  """
  Inverse of the above function
  """

  return math.sin(value / mapRadius) * M_PI_2 * eyeRadius


def outputPolarMaps(mapRadius: int, eyeRadius: int, irisRadius: int, slitPupilRadius: int = 0) -> None:
  """
  Generates one quadrant of a polar coordinate map radius x radius in size, suitable for
  mapping iris and sclera images into polar coordinates for display.

  :param mapRadius:       the radius of the polar map to generate, in pixels.
  :param eyeRadius:       the radius of the eye (sclera), in pixels.
  :param irisRadius:      the radius of the eye's iris, in pixels.
  :param slitPupilRadius: the radius of the slit pupil. Zero will result in a round pupil,
                          larger values (between 1 and irisRadius) make a taller/thinner pupil.
  """

  if slitPupilRadius < 0 or slitPupilRadius > irisRadius:
    sys.stderr.write('slitPupilRadius must be a value between 0 and {}'.format(irisRadius))
    exit(1)

  mapRadius2 = mapRadius * mapRadius
  polarAngle = np.full(mapRadius2, 0, dtype=np.uint8)
  polarDist = np.full(mapRadius2, 0, dtype=np.uint8)

  # Iris size, in polar map pixels
  iRad = screenToMap(mapRadius, eyeRadius, irisRadius)
  irisRadius2 = iRad * iRad

  angleIndex = 0
  distIndex = 0

  # Only the first quadrant is calculated, the other three are mirrored/rotated from this.
  for y in range(mapRadius):
    dy = y + 0.5                # Y distance to map center
    dy2 = dy * dy
    for x in range(mapRadius):
      dx = x + 0.5              # X distance to map center
      d2 = dx * dx + dy2        # Distance to center of map, squared
      if d2 > mapRadius2:
        # The point is outside the bounds of the eye, mark it as such
        polarAngle[angleIndex] = 0
        polarDist[distIndex] = 128
      else:
        # This point is within the eye area
        angle = math.atan2(dy, dx)              # -pi to +pi (0 to +pi/2 in first quadrant)
        angle = M_PI_2 - angle                  # Clockwise, 0 at top
        angle *= 512.0 / M_PI                   # 0 to <256 in 1st quadrant
        polarAngle[angleIndex] = int(angle)

        d = math.sqrt(d2)
        if d2 > irisRadius2:
          # This point is in the sclera area
          d = (mapRadius - d) / (mapRadius - iRad) * 127.0
          polarDist[distIndex] = int(d)   # 0 to 127, 0 = outer edge of sclera
        else:
          # This point is in the iris. We use values in the range 128-255 to indicate this.
          if slitPupilRadius == 0:
            d = (iRad - d) / iRad * -127.0
            polarDist[distIndex] = int(d) + 255   # 128 to 255
          else:
            # This is ugly, it iteratively calculates the polarDist value by trial
            # and error. It should be possible to algebraically simplify this and
            # find the single polarDist point for a given pixel, but it hasn't been
            # implemented yet
            xp = x + 0.5
            for i in range(126, -1, -1):
              ratio = i / 128.0             # 0.0 (open) to just-under-1.0 (slit) (>= 1.0 will cause trouble)
              # Interpolate a point between top of iris and top of slit pupil, based on ratio
              y1 = iRad - (iRad - slitPupilRadius) * ratio
              # x1 is 0 and so is dropped from equation below
              # And another point between right of iris and center of eye, inverse ratio
              x2 = iRad * (1.0 - ratio)
              # y2 is zero too so is also dropped
              # Find X coordinate of center of circle that crosses above two points and has Y at 0.0
              xc = (x2 * x2 - y1 * y1) / (2 * x2)
              dx = x2 - xc          # Distance from center of circle to right edge
              r2 = dx * dx          # center-to-right distance squared
              dx = xp - xc          # X component
              d2 = dx * dx + dy2    # Distance from pixel to left 'xc' point
              if d2 <= r2:
                # The point is within the circle
                polarDist[y * mapRadius + x] = int(-1 - i) + 256 # Set to distance 'i'
                break

          if polarDist[distIndex] < 128:
            sys.stderr.write("polarDist - iris value out of [128, 255] range at {},{} -> {}\n".format(x, y, d))
            # exit(1)

      angleIndex += 1
      distIndex += 1

  print('  // Polar coordinate mappings for the iris and sclera')
  print('  const uint8_t polarAngle[{} * {}] PROGMEM = {{'.format(mapRadius, mapRadius))
  hexTable = HexTable(mapRadius2, 16, 2, 2)
  for i in range(mapRadius2):
    hexTable.write(polarAngle[i])

  f = open('polarAngle.bin', 'w+b')
  f.write(bytes(polarAngle))
  f.close()

  img = Image.frombytes('L', (mapRadius, mapRadius), polarAngle)
  img.save("polarAngle.png")

  print('  const uint8_t polarDist[{} * {}] PROGMEM = {{'.format(mapRadius, mapRadius))
  hexTable = HexTable(mapRadius2, 16, 2, 2)
  for i in range(mapRadius2):
    hexTable.write(polarDist[i])

  f = open('polarDist.bin', 'w+b')
  f.write(bytes(polarDist))
  f.close()

  img = Image.frombytes('L', (mapRadius, mapRadius), polarDist)
  img.save("polarDist.png")


def outputPolar(arg: int, defaultFilename: str) -> None:
  """
  Generate the polar coordinate table, return the iris size
  """

  # For unusual-shaped pupils (dragon, goat, etc.), a precomputed image
  # provides polar distances.  Optional additional argument is filename
  # (or file 'pupilMap.png' in the local directory) is what's used,
  # otherwise a regular round iris is calculated.
  filename = getParam(arg, defaultFilename)
  try:
    image = Image.open(filename)
    width = image.size[0]
    height = image.size[1]
    if width != height:
      sys.stderr.write('Pupil map image {} must be square, but is {}x{}'.format(filename, width, height))
      exit(1)

    image = image.convert('L')
    pixels = image.load()
    irisSize = width
    usePupilMap = True

  except IOError:
    irisSize = DEFAULT_IRIS_SIZE
    usePupilMap = False

  if irisSize % 2 != 0:
    sys.stderr.write('Iris diameter {} must be an even value'.format(irisSize))
    exit(1)

  # One element per screen pixel, 16 bits per element -- high 9 bits are
  # angle relative to center point (fixed point, 0-511) low 7 bits are
  # distance from circle perimeter (fixed point, 0-127, pixels outsize circle
  # are set to 127).
  print('  constexpr uint8_t irisSize = {};'.format(irisSize))
  print('  const uint16_t polar[irisSize * irisSize] PROGMEM = {')

  hexTable = HexTable(irisSize * irisSize, 12, 4, 2)
  radius = irisSize / 2
  for y in range(irisSize):
    dy = y - radius + 0.5
    for x in range(irisSize):
      dx = x - radius + 0.5
      distance = math.sqrt(dx * dx + dy * dy)
      if distance >= radius:  # Outside circle
        hexTable.write(128)  # angle = 0, dist = 127
      else:
        angle = math.atan2(dy, dx)  # -pi to +pi
        angle += math.pi  # 0.0 to 2pi
        angle /= (math.pi * 2.0)  # 0.0 to <1.0
        if usePupilMap:
          # Look up polar coordinates in pupil map image
          distance = pixels[x, y] / 255.0
        else:
          distance /= radius  # 0.0 to <1.0
        distance *= 128.0  # 0.0 to <128.0
        if distance > 127:
          distance = 127  # Clip
        a = int(angle * 512.0)  # 0 to 511
        d = 127 - int(distance)  # 127 to 0
        hexTable.write((a << 7) | d)

  print('  const Image polarMap(polar, irisSize, irisSize);')
  print()


def main():
  try:
    eyeName = sys.argv[1]
  except IndexError:
    # No eye name supplied, so use the name of the directory
    eyeName = os.path.basename(os.getcwd()).replace(" ", "")

  print('#include "../eyes.h"')
  print()
  print('namespace {} {{'.format(eyeName))

  outputSclera(2)
  outputIris(3)

  print('#ifdef SYMMETRICAL_EYELID')
  print()
  outputEyelid(4, 'upper-symmetrical.bmp', 'upper')
  outputEyelid(5, 'lower-symmetrical.bmp', 'lower')
  print('#else')
  print()
  outputEyelid(6, 'upper.bmp', 'upper')
  outputEyelid(7, 'lower.bmp', 'lower')
  print('#endif // SYMMETRICAL_EYELID')
  print()

  outputPolar(8, 'pupilMap.png')

  pupilColour = 0
  backColour = 0
  irisMin = 570
  irisMax = 800
  irisRadius = 80
  eyeRadius = 120
  mapRadius = 240
  slitPupilRadius = 60

  outputPolarMaps(mapRadius, eyeRadius, irisRadius, slitPupilRadius)


  print('  EyeParams params(')
  print('    {}, {}, irisSize, {}, {}, {}, {}, irisImage, scleraImage, polarMap, upper, lower'.format(pupilColour, backColour, irisMin, irisMax, eyeRadius, mapRadius))
  print('  );')

  print('}')  # End of namespace block


if __name__ == "__main__":
  main()