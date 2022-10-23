#!/usr/bin/python

"""
Image converter for 'Uncanny Eyes' project.  This generates lookup tables
for eye data C header files, for all available eyes.

Optional command line parameters are:
  1. The directory to write the output header files to.
  2. The base source directory to find eye definition subdirectories in.
"""

import os
import sys
from pathlib import Path
from tablegen import getParam, generateEyeCode


def main():
  try:
    outputDir = Path(sys.argv[1]).resolve()
  except IndexError:
    raise Exception('A valid output directory must be supplied as the first command line parameter')

  if not outputDir.is_dir():
    raise Exception(f'The parameter {outputDir} is not a directory')

  sourceDir = Path(getParam(2, '.'))
  try:
    sourceDir = sourceDir.resolve()
  except:
    raise Exception(f'The source directory {sourceDir} is not a valid directory')

  subdirs = [f.path for f in os.scandir(sourceDir) if f.is_dir()]

  for subdir in subdirs:
    configFile = Path(subdir).joinpath('config.eye')
    if configFile.exists():
      generateEyeCode(str(outputDir), str(configFile))

if __name__ == "__main__":
  main()
