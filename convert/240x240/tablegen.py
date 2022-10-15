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

import json
import math
import os
import sys
import numpy as np
from PIL import Image
from config import EyeConfig, IrisConfig, ScleraConfig
from hextable import HexTable

SCREEN_WIDTH = 240
SCREEN_HEIGHT = 240

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


def loadEyeConfig(filename: str) -> EyeConfig:
  try:
    f = open(filename)
    return EyeConfig.fromDict(json.load(f))
  except Exception as e:
    sys.stderr.write(f'Could not read configuration file {filename}: {e}')
    exit(1)


def outputImage(filename: str, image: Image, name: str) -> None:
  """
  Writes an image file out to a C style array of uint16_t in 565 RGB format
  """
  width = image.size[0]
  height = image.size[1]

  print(f'  // {filename} - {width}x{height}, 16 bit 565 RGB')
  print(f'  constexpr uint16_t {name}Width = {width};')
  print(f'  constexpr uint16_t {name}Height = {height};')
  print(f'  const uint16_t {name}[{name}Width * {name}Height] PROGMEM = {{')

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


def outputSinglePixelImage(color565: int, name: str) -> None:
  c = (color565 >> 11, (color565 >> 5) & 0b00111111, color565 & 0b00011111)
  outputImage("Single color", Image.new('RGB', (1, 1), c), name)


def outputGreyscale(data, width: int, height: int, name: str) -> None:
  """
  Writes an image file out to a C style array of uint8_t, and also a PNG and raw binary file, in greyscale format
  """

  print(f'  const uint8_t {name}[{width} * {height}] PROGMEM = {{')
  hexTable = HexTable(width * height, 16, 2, 2)
  for i in range(width * height):
    hexTable.write(data[i])

  img = Image.frombytes('L', (width, height), data)
  img.save(f'{name}.png')


def outputSclera(arg: int, sclera: ScleraConfig) -> None:
  """
  Load, validate and output the sclera configuration and texture map
  """
  filename = getParam(arg, sclera.filename)
  if filename is not None:
    image = Image.open(filename)
    image = image.convert('RGB')
    outputImage(filename, image, 'sclera')
  else:
    outputSinglePixelImage(sclera.color, 'sclera')


def outputIris(arg: int, iris: IrisConfig) -> None:
  """
  Load, validate and output the iris configuration and texture map
  """
  filename = getParam(arg, iris.filename)
  if filename is not None:
    image = Image.open(filename)
    image = image.convert('RGB')
    width = image.size[0]
    height = image.size[1]

    if width > 512 or height > 128:
      raise Exception('Iris image can\'t exceed 512 pixels wide or 128 pixels tall')

    outputImage(filename, image, 'iris')
  else:
    outputSinglePixelImage(iris.color, 'iris')


def outputEyelid(arg: int, defaultFilename: str, tableName: str) -> None:
  """
  Load, validate and output an eyelid threshold lookup table
  """
  filename = getParam(arg, defaultFilename)

  if filename is None:
    # Make an all white image which results in no eyelids
    image = Image.new('L', (SCREEN_WIDTH, SCREEN_HEIGHT), 0x1)
  else:
    image = Image.open(filename)
    if (image.size[0] != SCREEN_WIDTH) or (image.size[1] != SCREEN_HEIGHT):
      raise Exception('{} dimensions must match screen size of {}x{}'.format(filename, SCREEN_WIDTH, SCREEN_HEIGHT))

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
    sys.stderr.write(f'slitPupilRadius must be a value between 0 and {irisRadius}')
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
        polarDist[distIndex] = 255
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
          # This point is in the iris. We use values in the range 128-254 to indicate this.
          if slitPupilRadius == 0:
            d = (iRad - d) / iRad * 127.0
            polarDist[distIndex] = int(d) + 128
          else:
            xp = x + 0.5

            # Figure out a sensible starting point based on neighbouring pixels that we've already calculated.
            # This results in a massive speedup compared to the original brute-force M4_Eyes approach.
            start = 129 if x == 0 and y == 0 else polarDist[(y - 1) * mapRadius] if x == 0 else polarDist[y * mapRadius + x - 1]
            start = 255 - start
            for i in range(start, -1, -1):
              ratio = i / 128.0    # 0.0 (open) to just-under-1.0 (slit) (>= 1.0 will cause trouble)
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
                polarDist[distIndex] = 255 - i # Set to distance 'i'
                break

          if polarDist[distIndex] < 128:
            sys.stderr.write(f"polarDist - iris value out of [128, 255] range at {x},{y} -> {d}\n")
            # exit(1)

      angleIndex += 1
      distIndex += 1

  print('  // Polar coordinate mappings for the iris and sclera')
  outputGreyscale(polarAngle, mapRadius, mapRadius, 'polarAngle')
  outputGreyscale(polarDist, mapRadius, mapRadius, 'polarDist')


def outputDisplacement(mapRadius: int, eyeRadius: int) -> None:
  """
  Generate the displacement mapping table
  """

  size = SCREEN_WIDTH // 2

  displacementMap = np.full(size * size, 0, dtype=np.uint8)
  eyeRadius2 = eyeRadius * eyeRadius
  for y in range(size):
    dy = y + 0.5
    dy2 = dy * dy
    for x in range (size):
      # Get distance to origin point. Pixel centers are at +0.5, this is
      # normal, desirable and by design -- screen center at (120.0,120.0)
      # falls between pixels and allows numerically-correct mirroring.
      dx = x + 0.5
      d2 = dx * dx + dy2
      if d2 <= eyeRadius2:
        d = math.sqrt(d2)
        h = math.sqrt(eyeRadius2 - d2)
        a = math.atan2(d, h)
        pa = a / M_PI_2 * mapRadius
        dx /= d
        displacementMap[y * size + x] = int(dx * pa - x)
      else:
        displacementMap[y * size + x] = 255

  print('  constexpr uint8_t dispSize = {};'.format(size))
  outputGreyscale(displacementMap, size, size, "displacementMap")



def main():
  config = loadEyeConfig(getParam(1, 'config.eye'))

  try:
    eyeName = sys.argv[2]
  except IndexError:
    # No eye name supplied, so use the name of the directory
    eyeName = os.path.basename(os.getcwd()).replace(" ", "")

  print('#include "../eyes.h"')
  print()
  print(f'namespace {eyeName} {{')

  outputIris(3, config.iris)
  outputSclera(4, config.sclera)
  outputEyelid(5, config.eyelid.upperFilename, 'upper')
  outputEyelid(6, config.eyelid.lowerFilename, 'lower')

  mapRadius = 240

  outputPolarMaps(mapRadius, config.radius, config.iris.radius, config.pupil.slitRadius)

  outputDisplacement(mapRadius, config.radius)


  """
  EyeParams e = {
      radius, backColor, tracking, squint, displacementMap,
      {color, slitRadius, min, max},
      {irisRadius, {irisTexture, irisWidth, irisHeight}, irisColor, irisSpin},
      {{scleraTexture, scleraWidth, scleraHeight}, scleraColor, scleraSpin},
      {upper, lower, color},
      {mapRadius, polarAngle, polarDist}
  };
"""


  print('  EyeParams params = {')
  print(f'      {config.radius}, {config.backColor}, {config.squint}, {str(config.tracking).lower()}, displacementMap, ')
  print(f'      {{ {config.pupil.color}, {config.pupil.slitRadius}, {config.pupil.min}, {config.pupil.max} }},')
  print(f'      {{ {config.iris.radius}, {{ iris, irisWidth, irisHeight }}, {config.iris.color}, {config.iris.angle}, {config.iris.spin} }},')
  print(f'      {{ {{ sclera, scleraWidth, scleraHeight }}, {config.sclera.color}, {config.sclera.spin} }},')
  print(f'      {{ upper, lower, {config.eyelid.color} }},')
  print(f'      {{ {mapRadius}, polarAngle, polarDist }}')
  print('  };')

  print('}')  # End of namespace block


if __name__ == "__main__":
  main()