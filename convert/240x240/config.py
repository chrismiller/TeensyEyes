def _toInt(value) -> int:
  if isinstance(value, str) and value.startswith(('0x', '0X')):
    return int(value, 16)
  return int(value)


class PupilConfig:
  def __init__(self, color: int = 0, slitRadius: int = 0, _min: float = 0.0, _max: float = 1.0):
    """
    Creates a pupil configuration instance.
    :param color: the color of the pupil, in 16 bit 565 RGB format.
    :param slitRadius: the radius in pixels of a slit-shaped pupil. Must be less than or equal to the
    iris radius. Set to zero for a round pupil.
    :param _min: the minimum fraction of the iris size that the pupil can be.
    :param _max: the maximum fraction of the iris size that the pupil can be.
    """
    self.color = color
    self.slitRadius = slitRadius
    self.min = _min
    self.max = _max


class IrisConfig:
  def __init__(self, filename: str = None, color: int = 0, radius: int = 60, angle: int = 0, spin: int = 0):
    """
    Creates an iris configuration instance.
    :param filename: the name of the iris image file, or None to use the iris color property instead.
    :param color: the color of the iris, 1n 16 bit 565 RGB format. This only takes effect when filename is None.
    :param radius: the radius of the iris, in pixels.
    :param spin: the amount of spin to apply to the iris on each frame. Set to zero to disable iris spinning.
    """
    self.filename = filename
    self.color = color
    self.radius = radius
    self.angle = angle
    self.spin = spin


class ScleraConfig:
  def __init__(self, filename: str = None, color: int = 0, angle: int = 0, spin: int = 0):
    """
    Creates a sclera configuration instance.
    :param filename: the name of the sclera image file, or None to use the color property instead.
    :param color: the color of the sclera, in 16 bit 565 RGB format. This only takes effect when filename is None.
    :param spin: the amount of spin to apply to the sclera on each frame. Set to zero to disable sclera spinning.
    """
    self.filename = filename
    self.color = color
    self.angle = angle
    self.spin = spin


class EyelidConfig:
  def __init__(self, upperFilename: str, lowerFilename: str, color: int):
    """
    Creates an eyelid configuration instance.
    :param upperFilename: the filename of the upper eyelid, or None if there is no upper eyelid.
    :param lowerFilename: the filename of the lower eyelid, or None if there is no lower eyelid.
    @param color: the color to draw the eyelid, in 16 bit 565 RGB format.
    """
    self.upperFilename = upperFilename
    self.lowerFilename = lowerFilename
    self.color = color


class EyeConfig:
  """
  A class that holds the configuration for a single eye.
  """

  def __init__(self, radius: int, tracking: bool, backColor: int, squint: float, pupil: PupilConfig,
               iris: IrisConfig, sclera: ScleraConfig, eyelid: EyelidConfig):
    self.radius = radius
    self.tracking = tracking
    self.backColor = backColor
    self.squint = squint
    self.pupil = pupil
    self.iris = iris
    self.sclera = sclera
    self.eyelid = eyelid

  @classmethod
  def fromDict(cls, params: dict):
    try:
      eyeRadius = _toInt(params.get('radius', 120))
      tracking = bool(params.get('tracking', True))
      backColor = _toInt(params.get('backColor', 0))
      squint = float(params.get('squint', 0.0))

      pupilDict = params.get('pupil', {})
      pupil = PupilConfig(_toInt(pupilDict.get('color', 0)), _toInt(pupilDict.get('slitRadius', 0)),
                          float(pupilDict.get('min', 0.0)), float(pupilDict.get('max', 1.0)))

      irisDict = params.get('iris', {})
      iris = IrisConfig(irisDict.get('filename'), _toInt(irisDict.get('color', 0)),
                        _toInt(irisDict.get('radius', 60)), _toInt(irisDict.get('angle', 0)),
                        _toInt(irisDict.get('spin', 0)))

      scleraDict = params.get('sclera', {})
      sclera = ScleraConfig(scleraDict.get('filename'), _toInt(scleraDict.get('color', 0)),
                            float(scleraDict.get('spin', 0.0)))

      eyelidDict = params.get('eyelid', {})
      eyelid = EyelidConfig(eyelidDict.get('upperFilename'), eyelidDict.get('lowerFilename'), eyelidDict.get('color', 0))

      return cls(eyeRadius, tracking, backColor, squint, pupil, iris, sclera, eyelid)

    except Exception as e:
      raise Exception(f'Unable to convert dictionary into an EyeConfig object: {e}\n{params}')
