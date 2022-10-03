#!/usr/bin/python

"""
Image converter for 'Uncanny Eyes' project.  Generates tables for
eyeData.h file.  Requires Python Imaging Library.  Expects six image
files: sclera, iris, upper and lower eyelid map (symmetrical), upper
and lower eyelid map (asymmetrical L/R) -- defaults will be used for
each if not specified.  Also generates polar coordinate map for iris
rendering (pass diameter -- must be an even value -- as 7th argument),
pupil is assumed round unless pupilMap.png image is present.
Output is to stdout; should be redirected to file for use.
"""

# This is kinda some horrible copy-and-paste code right now for each of
# the images...could be improved, but basically does the thing.

import sys
import math
from PIL import Image
from hextable import HexTable

# OPEN AND VALIDATE SCLERA IMAGE FILE --------------------------------------

try:
    FILENAME = sys.argv[1]
except IndexError:
    FILENAME = 'sclera.png' # Default filename if argv 1 not provided
IMAGE = Image.open(FILENAME)
IMAGE = IMAGE.convert('RGB')
PIXELS = IMAGE.load()

# SCREEN_WIDTH = 128
# SCREEN_HEIGHT = 128
# DEFAULT_IRIS_SIZE = 80  # Default size if argv 7 not provided
SCREEN_WIDTH = 240
SCREEN_HEIGHT = 240
DEFAULT_IRIS_SIZE = 150  # Default size if argv 7 not provided

# GENERATE SCLERA ARRAY ----------------------------------------------------

print('#define SCLERA_WIDTH  ' + str(IMAGE.size[0]))
print('#define SCLERA_HEIGHT ' + str(IMAGE.size[1]))
print('')

sys.stdout.write('const uint16_t sclera[SCLERA_HEIGHT][SCLERA_WIDTH] PROGMEM = {')
HEX = HexTable(IMAGE.size[0] * IMAGE.size[1], 8, 4)

# Convert 24-bit image to 16 bits:
for y in range(IMAGE.size[1]):
    for x in range(IMAGE.size[0]):
        p = PIXELS[x, y] # Pixel data (tuple)
        HEX.write(
            ((p[0] & 0b11111000) << 8) | # Convert 24-bit RGB
            ((p[1] & 0b11111100) << 3) | # to 16-bit value w/
            (p[2] >> 3))                 # 5/6/5-bit packing

# OPEN AND VALIDATE IRIS IMAGE FILE ----------------------------------------

try:
    FILENAME = sys.argv[2]
except IndexError:
    FILENAME = 'iris.png' # Default filename if argv 2 not provided
IMAGE = Image.open(FILENAME)
if (IMAGE.size[0] > 512) or (IMAGE.size[1] > 128):
    sys.stderr.write('Iris image can\'t exceed 512 pixels wide or 128 pixels tall')
    exit(1)
IMAGE = IMAGE.convert('RGB')
PIXELS = IMAGE.load()

# GENERATE IRIS ARRAY ------------------------------------------------------

print('')
print('#define IRIS_MAP_WIDTH  ' + str(IMAGE.size[0]))
print('#define IRIS_MAP_HEIGHT ' + str(IMAGE.size[1]))
print('')

sys.stdout.write('const uint16_t iris[IRIS_MAP_HEIGHT][IRIS_MAP_WIDTH] PROGMEM = {')
HEX.reset(IMAGE.size[0] * IMAGE.size[1])

for y in range(IMAGE.size[1]):
    for x in range(IMAGE.size[0]):
        p = PIXELS[x, y] # Pixel data (tuple)
        HEX.write(
            ((p[0] & 0b11111000) << 8) | # Convert 24-bit RGB
            ((p[1] & 0b11111100) << 3) | # to 16-bit value w/
            (p[2] >> 3))                 # 5/6/5-bit packing


def processLid(arg: int, default_filename: str, table_name: str):
    # OPEN AND VALIDATE EYELID THRESHOLD LOOKUP TABLE ---------------
    try:
        filename = sys.argv[arg]
    except IndexError:
        # Use the provided default if an argument wasn't provided
        filename = default_filename

    image = Image.open(filename)
    if (image.size[0] != SCREEN_WIDTH) or (image.size[1] != SCREEN_HEIGHT):
        sys.stderr.write('{} dimensions must match screen size of {}x{}'.format(filename, SCREEN_WIDTH, SCREEN_HEIGHT))
        exit(1)
    image = image.convert('L')
    pixels = image.load()

    sys.stdout.write('// An array of vertical start/stop locations for each {} eyelid column\n'.format(filename))
    sys.stdout.write('const uint8_t {}[SCREEN_WIDTH][2] PROGMEM = {{'.format(table_name))
    table = HexTable(image.size[0] * 2, 16, 2)

    for x in range(image.size[0]):
        found_start = False
        found_end = False
        y = 0
        for y in range(image.size[1]):
            if not found_start:
                if pixels[x, y] > 0:
                    table.write(y)
                    found_start = True
            elif not found_end:
                if pixels[x, y] == 0:
                    table.write(y)
                    found_end = True
                    break
        if not found_start:
            sys.stderr.write('{} doesn''t have an eyelid in column {}'.format(filename, y))
            exit(1)
        if not found_end:
            # The eyelid goes all the way to the bottom of this column
            table.write(SCREEN_HEIGHT)


print('')
print('#define SCREEN_WIDTH  ' + str(SCREEN_WIDTH))
print('#define SCREEN_HEIGHT ' + str(SCREEN_HEIGHT))
print('')
print('#ifdef SYMMETRICAL_EYELID')
print('')
processLid(3, "upper-symmetrical.bmp", "upper")
print('')
processLid(4, "lower-symmetrical.bmp", "lower")
print('')
print('#else')
print('')
processLid(5, "upper.bmp", "upper")
print('')
processLid(6, "lower.bmp", "lower")


# GENERATE POLAR COORDINATE TABLE ------------------------------------------

try:
    IRIS_SIZE = int(sys.argv[7])
except IndexError:
    IRIS_SIZE = DEFAULT_IRIS_SIZE
if IRIS_SIZE % 2 != 0:
    sys.stderr.write('Iris diameter must be even value')
    exit(1)
RADIUS = IRIS_SIZE / 2
# For unusual-shaped pupils (dragon, goat, etc.), a precomputed image
# provides polar distances.  Optional 8th argument is filename (or file
# 'pupilMap.png' in the local directory) is what's used, otherwise a
# regular round iris is calculated.
try:
    FILENAME = sys.argv[8]
except IndexError:
    FILENAME = 'pupilMap.png' # Default filename if argv 8 not provided
USE_PUPIL_MAP = True
try:
    IMAGE = Image.open(FILENAME)
    if (IMAGE.size[0] != IRIS_SIZE) or (IMAGE.size[0] != IMAGE.size[1]):
        sys.stderr.write('Iris size must match pupil map size - setting iris size to %s' % IMAGE.size[0])
        IRIS_SIZE = IMAGE.size[0]
    IMAGE = IMAGE.convert('L')
    PIXELS = IMAGE.load()
except IOError:
    USE_PUPIL_MAP = False

print('')
print('#endif // SYMMETRICAL_EYELID')
print('')
print('#define IRIS_WIDTH  ' + str(IRIS_SIZE))
print('#define IRIS_HEIGHT ' + str(IRIS_SIZE))

# One element per screen pixel, 16 bits per element -- high 9 bits are
# angle relative to center point (fixed point, 0-511) low 7 bits are
# distance from circle perimeter (fixed point, 0-127, pixels outsize circle
# are set to 127).

sys.stdout.write('\nconst uint16_t polar[IRIS_HEIGHT][IRIS_WIDTH] PROGMEM = {')
HEX = HexTable(IRIS_SIZE * IRIS_SIZE, 8, 4)

for y in range(IRIS_SIZE):
    dy = y - RADIUS + 0.5
    for x in range(IRIS_SIZE):
        dx = x - RADIUS + 0.5
        distance = math.sqrt(dx * dx + dy * dy)
        if distance >= RADIUS: # Outside circle
            HEX.write(127) # angle = 0, dist = 127
        else:
            angle = math.atan2(dy, dx) # -pi to +pi
            angle += math.pi           # 0.0 to 2pi
            angle /= (math.pi * 2.0)   # 0.0 to <1.0
            if USE_PUPIL_MAP:
                # Look up polar coordinates in pupil map image
                distance = PIXELS[x, y] / 255.0
            else:
                distance /= RADIUS         # 0.0 to <1.0
            distance *= 128.0              # 0.0 to <128.0
            if distance > 127:
                distance = 127 # Clip
            a = int(angle * 512.0)    # 0 to 511
            d = 127 - int(distance)   # 127 to 0
            HEX.write((a << 7) | d)
