#!/usr/bin/python

"""
Image converter for 'Uncanny Eyes' project.  This generates lookup tables
for eye data C header files. The following header files will be produced
in the output directory:

  <eye name>.h              - iris, sclera, eyelid lookup tables.
  displacement_[M]_[E}.h    - a displacement mapping lookup table.
  polarAngle_[M].h          - a polar mapping lookup table.
  polarDist_[M]_[E]_[I]_[S] - a polar distance lookup table.

  Where:   [M] = map radius (usually 240)
           [E] = eye radius
           [I] = iris radius
           [S] = slit pupil radius

Requires Pillow imaging and numpy libraries.

Optional command line parameters are:
  1. The directory to write the output header files to.
  2.  The name of a json config file that specifies the eye's settings. Defaults to config.eye.
  3.  The name of the eye. This will become a namespace that wraps the lookup tables. Defaults
      to using the 'name' property in the config.eye file.
  4.  The name of the image file to use as the iris texture.
  5.  The name of the image file to use as the sclera texture.
  6.  The name of the image file to use as the mask for the upper eyelid.
  7.  The name of the image file to use as the mask for the lower eyelid.
"""

import json
import math
import os
import sys
from typing import TextIO, List

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

def checkParamAbsent(params: dict, key: str) -> None:
  parents = key.split(':')
  key = parents.pop()
  node = params
  for p in parents:
    node = params.get(p, None)
  if node is not None and node.get('key') is not None:
    raise Exception(f'Invalid configuration: {key} cannot be specified for a single eye')

def validateSingleEyeParams(params: dict) -> None:
  checkParamAbsent(params, 'radius')
  checkParamAbsent(params, 'pupil:slitRadius')
  checkParamAbsent(params, 'pupil:slitRadius')
  checkParamAbsent(params, 'iris:radius')

def loadEyeConfig(filename: str) -> List[EyeConfig]:
  try:
    f = open(filename)
    params = json.load(f)

    # Use the current directory name for the name if none was specified in the config file
    if params.get('name') is None:
      params['name'] = os.path.basename(os.getcwd()).replace(" ", "")

    # TODO - add support for an arbitrary number of named eyes
    leftConfig = params.pop('left', {})
    rightConfig = params.pop('right', {})

    result = []
    if leftConfig or rightConfig:
      validateSingleEyeParams(leftConfig)
      leftParams = params | leftConfig
      leftParams['name'] = params['name'] + '.left'
      result.append(EyeConfig.fromDict(leftParams))

      validateSingleEyeParams(rightConfig)
      rightParams = params | rightConfig
      rightParams['name'] = params['name'] + '.right'
      result.append(EyeConfig.fromDict(rightParams))
    else:
      params['name'] = params['name'] + '.eye'
      result.append(EyeConfig.fromDict(params))

    return result

  except Exception as e:
    raise Exception(f'Could not read configuration file {filename}: {e}')


def outputImageFile(out: TextIO, filename: str, name: str, maxWidth: int, maxHeight: int) -> None:
  """
  Load an image from disk and output it to a C style array of uint16_t in 565 RGB format
  """
  image = Image.open(filename)
  image = image.convert('RGB')
  width = image.size[0]
  height = image.size[1]

  if width > maxWidth or height > maxHeight:
    raise Exception(f'Texture is {width}x{height} - it must not exceed {maxWidth} pixels wide or {maxHeight} pixels tall')

  out.write(f'  // {filename} - {width}x{height}, 16 bit 565 RGB\n')
  out.write(f'  constexpr uint16_t {name}Width = {width};\n')
  out.write(f'  constexpr uint16_t {name}Height = {height};\n')
  out.write(f'  const uint16_t {name}[{name}Width * {name}Height] PROGMEM = {{\n')

  pixels = image.load()
  hexTable = HexTable(out, width * height, 12, 4, 2)
  # Convert 24-bit image to 16 bits:
  for y in range(height):
    for x in range(width):
      p = pixels[x, y]  # Pixel data (tuple)
      hexTable.write(
        ((p[0] & 0b11111000) << 8) |  # Convert 24-bit RGB
        ((p[1] & 0b11111100) << 3) |  # to 16-bit value with
        (p[2] >> 3))  # 5/6/5-bit packing


def outputGreyscale(out: TextIO, data, width: int, height: int, name: str) -> None:
  """
  Writes an image file out to a C style array of uint8_t
  """

  out.write(f'#include "{name}.h"\n\n')
  out.write(f'const uint8_t {name}[{width} * {height}] PROGMEM = {{\n')
  hexTable = HexTable(out, width * height, 16, 2)
  for i in range(width * height):
    hexTable.write(data[i])

  # Maybe useful for debugging - write out a greyscale PNG
  # img = Image.frombytes('L', (width, height), data)
  # img.save(f'{name}.png')


def outputGreyscaleCpp(outputDir: str, name: str, data, width: int, height: int) -> None:
  base = f'{outputDir}/{name}'
  print(f'Writing {name} lookup table to {base}.(h, cpp)')
  with open(base + '.h', 'w') as header:
    header.write('#pragma once\n\n')
    header.write('#include <Arduino.h>\n\n')
    header.write(f'extern const uint8_t {name}[];\n')
  with open(base + '.cpp', 'w') as cpp:
    cpp.write(f'// {name} lookup table for the iris and sclera\n')
    outputGreyscale(cpp, data, width, height, name)


def outputNoEyelids(out: TextIO) -> None:
  out.write('  // All zeroes, which results in no upper eyelid\n')
  out.write('  const uint8_t noUpper[screenWidth * 2] PROGMEM = {\n')
  for i in range(30):
    out.write('   ' + ' 0x00,' * 16 + '\n')
  out.write('  };\n\n')
  out.write('  // All 255, which results in no lower eyelid\n')
  out.write('  const uint8_t noLower[screenWidth * 2] PROGMEM = {\n')
  for i in range(30):
    out.write('   ' + ' 0xFF,' * 16 + '\n')
  out.write('  };\n\n')


def outputEyelid(out: TextIO, filename: str, tableName: str) -> None:
  """
  Load, validate and output an eyelid threshold lookup table
  """
  image = Image.open(filename)
  if (image.size[0] != SCREEN_WIDTH) or (image.size[1] != SCREEN_HEIGHT):
    raise Exception(f'{filename} dimensions must match screen size of {SCREEN_WIDTH}x{SCREEN_HEIGHT}')

  image = image.convert('L')
  pixels = image.load()

  out.write(
    f'  // An array of vertical start (inclusive) and end (exclusive) locations for each {filename} eyelid column\n')
  out.write(f'  const uint8_t {tableName}[screenWidth * 2] PROGMEM = {{\n')

  hexTable = HexTable(out, image.size[0] * 2, 16, 2, 2)
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
      raise Exception(f'{filename} doesn\'t have an eyelid in column {y}')
    if not found_end:
      # The eyelid goes all the way to the bottom of this column
      hexTable.write(SCREEN_HEIGHT)


def screenToMap(mapRadius: int, eyeRadius: int, value: int) -> float:
  """
  Scale a measurement in screen pixels to polar map pixels
  """

  return math.atan2(value, math.sqrt(eyeRadius * eyeRadius - value * value)) / M_PI_2 * mapRadius


def outputPolarMaps(outputDir: str, angleName: str, distName: str, mapRadius: int,
                    eyeRadius: int, irisRadius: int, slitPupilRadius: int = 0) -> None:
  """
  Generates one quadrant of a polar coordinate map radius x radius in size, suitable for
  mapping iris and sclera images into polar coordinates for display.

  :param outputDir:       the output directory o write the polar angle files to.
  :param angleName:       the name to give the polar angle lookup table in the generated C code.
  :param distName:        the name to give the polar distance lookup table in the generated C code.
  :param mapRadius:       the radius of the polar maps to generate, in pixels.
  :param eyeRadius:       the radius of the eye (sclera), in pixels.
  :param irisRadius:      the radius of the eye's iris, in pixels.
  :param slitPupilRadius: the radius of the slit pupil. Zero will result in a round pupil,
                          larger values (between 1 and irisRadius) make a taller/thinner pupil.
  """

  if slitPupilRadius < 0 or slitPupilRadius > irisRadius:
    raise Exception(f'slitPupilRadius must be a value between 0 and {irisRadius}')

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
    dy = y + 0.5  # Y distance to map center
    dy2 = dy * dy
    for x in range(mapRadius):
      dx = x + 0.5  # X distance to map center
      d2 = dx * dx + dy2  # Distance to center of map, squared
      if d2 > mapRadius2:
        # The point is outside the bounds of the eye, mark it as such
        polarAngle[angleIndex] = 0
        polarDist[distIndex] = 255
      else:
        # This point is within the eye area
        angle = math.atan2(dy, dx)  # -pi to +pi (0 to +pi/2 in first quadrant)
        angle = M_PI_2 - angle  # Clockwise, 0 at top
        angle *= 512.0 / M_PI  # 0 to <256 in 1st quadrant
        polarAngle[angleIndex] = int(angle)

        d = math.sqrt(d2)
        if d2 > irisRadius2:
          # This point is in the sclera area
          d = (mapRadius - d) / (mapRadius - iRad) * 127.0
          polarDist[distIndex] = int(d)  # 0 to 127, 0 = outer edge of sclera
        else:
          # This point is in the iris. We use values in the range 128-254 to indicate this.
          if slitPupilRadius == 0:
            d = (iRad - d) / iRad * 127.0
            polarDist[distIndex] = int(d) + 128
          else:
            xp = x + 0.5

            # Figure out a sensible starting point based on neighbouring pixels that we've already calculated.
            # This results in a massive speedup compared to the original brute-force M4_Eyes approach.
            start = 129 if x == 0 and y == 0 else polarDist[(y - 1) * mapRadius] if x == 0 else polarDist[
              y * mapRadius + x - 1]
            start = 255 - start
            for i in range(start, -1, -1):
              ratio = i / 128.0  # 0.0 (open) to just-under-1.0 (slit) (>= 1.0 will cause trouble)
              # Interpolate a point between top of iris and top of slit pupil, based on ratio
              y1 = iRad - (iRad - slitPupilRadius) * ratio
              # x1 is 0 and so is dropped from equation below
              # And another point between right of iris and center of eye, inverse ratio
              x2 = iRad * (1.0 - ratio)
              # y2 is zero too so is also dropped
              # Find X coordinate of center of circle that crosses above two points and has Y at 0.0
              xc = (x2 * x2 - y1 * y1) / (2 * x2)
              dx = x2 - xc  # Distance from center of circle to right edge
              r2 = dx * dx  # center-to-right distance squared
              dx = xp - xc  # X component
              d2 = dx * dx + dy2  # Distance from pixel to left 'xc' point
              if d2 <= r2:
                # The point is within the circle
                polarDist[distIndex] = 255 - i  # Set to distance 'i'
                break

          if polarDist[distIndex] < 128:
            sys.stderr.write(f"{distName} - iris value out of [128, 255] range at {x},{y} -> {d}\n")
            # exit(1)

      angleIndex += 1
      distIndex += 1

  outputGreyscaleCpp(outputDir, angleName, polarAngle, mapRadius, mapRadius)
  outputGreyscaleCpp(outputDir, distName, polarDist, mapRadius, mapRadius)


def outputDisplacement(outputDir: str, name: str, mapRadius: int, eyeRadius: int) -> None:
  """
  Writes out the displacement mapping table.

  :param outputDir: the output directory o write the displacement map files to.
  :param name:      the name to give the displacement map in the C code.
  :param mapRadius: the "map radius", as used by the polar angle/distance lookups.
  :param eyeRadius: the radius of the eye to generate a map for.
  """
  size = SCREEN_WIDTH // 2
  displacementMap = np.full(size * size, 0, dtype=np.uint8)
  eyeRadius2 = eyeRadius * eyeRadius
  for y in range(size):
    dy = y + 0.5
    dy2 = dy * dy
    for x in range(size):
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

  outputGreyscaleCpp(outputDir, name, displacementMap, size, size)


def outputConfig(out: TextIO, config: EyeConfig, mapRadius: int, dispMapName: str,
                 angleMapName: str, distMapName: str, filenameMappings: dict[str, str]) -> None:
  """
  Writes out the C++ EyeDefinition
  EyeDefinition {configName} = {
      radius, backColor, tracking, squint, dispMapName,
      {color, slitRadius, min, max},
      {irisRadius, {irisTexture, irisWidth, irisHeight}, irisColor, irisSpin},
      {{scleraTexture, scleraWidth, scleraHeight}, scleraColor, scleraSpin},
      {upper, lower, color},
      {mapRadius, angleMapName, dispMapName}
  };
  """

  configName = config.name.split('.', 1)[-1]

  upper = filenameMappings.get(config.eyelid.upperFilename, 'noUpper')
  lower = filenameMappings.get(config.eyelid.lowerFilename, 'noLower')

  out.write(f'  const EyeDefinition {configName} PROGMEM = {{\n')
  out.write(f'      {config.radius}, {config.backColor}, {str(config.tracking).lower()}, {config.squint}, {dispMapName}, \n')
  out.write(f'      {{ {config.pupil.color}, {config.pupil.slitRadius}, {config.pupil.min}, {config.pupil.max} }},\n')
  if config.iris.filename is None:
    irisDef = 'nullptr, 0, 0'
  else:
    irisDef = f'{configName}Iris, {configName}IrisWidth, {configName}IrisHeight'
  mirror = 'true' if config.iris.mirror else 'false'
  out.write(f'      {{ {config.iris.radius}, {{ {irisDef} }}, {config.iris.color}, {config.iris.angle}, {config.iris.spin}, {mirror} }},\n')
  if config.sclera.filename is None:
    scleraDef = 'nullptr, 0, 0'
  else:
    scleraDef = f'{configName}Sclera, {configName}ScleraWidth, {configName}ScleraHeight'
  mirror = 'true' if config.sclera.mirror else 'false'
  out.write(f'      {{ {{ {scleraDef} }}, {config.sclera.color}, {config.sclera.angle}, {config.sclera.spin}, {mirror} }},\n')
  out.write(f'      {{ {upper}, {lower}, {config.eyelid.color} }},\n')
  out.write(f'      {{ {mapRadius}, {angleMapName}, {distMapName} }}\n')
  out.write('  };\n')


def main():
  outputDir = getParam(1, '.')
  if not os.path.exists(outputDir):
    sys.stderr.write(f'The path {outputDir} does not exist')
    exit(1)
  if not os.path.isdir(outputDir):
    sys.stderr.write(f'{outputDir} is not a directory')
    exit(1)

  configFile = getParam(2, 'config.eye')
  print(f'Loading eye configuration from {configFile}')
  configs = loadEyeConfig(configFile)

  mapRadius = 240

  eyeName = configs[0].name.split('.', 1)[0]
  outputFilename = f'{outputDir}/{eyeName}.h'
  angleMapName = f'polarAngle_{mapRadius}'
  distMapName = f'polarDist_{mapRadius}_{configs[0].radius}_{configs[0].iris.radius}_{configs[0].pupil.slitRadius}'
  dispMapName = f'disp_{mapRadius}_{configs[0].radius}'

  outputPolarMaps(outputDir, angleMapName, distMapName, mapRadius, configs[0].radius,
                  configs[0].iris.radius, configs[0].pupil.slitRadius)
  outputDisplacement(outputDir, dispMapName, mapRadius, configs[0].radius)

  print(f'Writing iris/sclera/eyelid data to {outputFilename}')
  with open(outputFilename, 'w') as eyeFile:
    eyeFile.write('#include "../eyes.h"\n')
    eyeFile.write(f'#include "{angleMapName}.h"\n')
    eyeFile.write(f'#include "{distMapName}.h"\n')
    eyeFile.write(f'#include "{dispMapName}.h"\n\n')
    eyeFile.write(f'namespace {eyeName} {{\n')

    filenameMappings = {}
    for config in configs:
      configName = config.name.split('.', 1)[-1]

      if config.iris.filename is not None and  config.iris.filename not in filenameMappings:
        irisName = configName + 'Iris'
        outputImageFile(eyeFile, config.iris.filename, irisName, 512, 128)
        filenameMappings[config.iris.filename] = irisName

      if config.sclera.filename is not None and config.sclera.filename not in filenameMappings:
          scleraName = configName + 'Sclera'
          outputImageFile(eyeFile, config.sclera.filename, scleraName, 800, 200)
          filenameMappings[config.sclera.filename] = scleraName

      if config.eyelid.upperFilename is None:
        with open(f'{outputDir}/noeyelids.h', 'w') as noEyelidsFile:
          eyeFile.write('#include "noeyelids.h"\n\n')
          outputNoEyelids(noEyelidsFile)
      else:
        if config.eyelid.upperFilename not in filenameMappings:
          outputEyelid(eyeFile, config.eyelid.upperFilename, configName + 'Upper')
          filenameMappings[config.eyelid.upperFilename] = configName + 'Upper'
        if config.eyelid.lowerFilename not in filenameMappings:
          outputEyelid(eyeFile, config.eyelid.lowerFilename, configName + 'Lower')
          filenameMappings[config.eyelid.lowerFilename] = configName + 'Lower'

      outputConfig(eyeFile, config, mapRadius, dispMapName, angleMapName, distMapName, filenameMappings)

    eyeFile.write('}\n')  # End of namespace block

  print("All done!")

if __name__ == "__main__":
  main()
