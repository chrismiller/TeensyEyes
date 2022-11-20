from __future__ import annotations

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

  @classmethod
  def fromDict(cls: type[PupilConfig], params: dict[str, object]) -> PupilConfig:
    """
    Creates and returns a PupilConfig instance by extracting configuration details from the supplied dictionary.
    :param params: a dictionary containing details of the eye's configuration.
    :return:
    """
    return cls(_toInt(params.get('color', 0)), _toInt(params.get('slitRadius', 0)),
               float(params.get('min', 0.3)), float(params.get('max', 0.7)))


class IrisConfig:
  def __init__(self, filename: str = None, color: int = 0, radius: int = 60, angle: int = 0, spin: int = 0, iSpin: int = 0, mirror = False):
    """
    Creates an iris configuration instance.
    :param filename: the name of the iris image file, or None to use the iris color property instead.
    :param color: the color of the iris, 1n 16 bit 565 RGB format. This only takes effect when filename is None.
    :param radius: the radius of the iris, in pixels.
    :param spin: the amount of spin to apply to the iris, in rpm. Set to zero to disable iris spinning.
    :param iSpin: the amount of spin to apply to the iris on each frame. This overrides the spin value.
    :param mirror: if true, the sclera will be flipped left to right.
    """
    self.filename = filename
    self.color = color
    self.radius = radius
    self.angle = angle
    self.spin = spin
    self.iSpin = iSpin
    self.mirror = mirror

  @classmethod
  def fromDict(cls: type[IrisConfig], params: dict[str, object]) -> IrisConfig:
    return cls(params.get('texture'), _toInt(params.get('color', 0)), _toInt(params.get('radius', 60)),
               _toInt(params.get('angle', 0)), _toInt(params.get('spin', 0)), _toInt(params.get('iSpin', 0)),
               bool(params.get('mirror', False)))


class ScleraConfig:
  def __init__(self, filename: str = None, color: int = 0, angle: int = 0, spin: int = 0, iSpin: int = 0, mirror = False):
    """
    Creates a sclera configuration instance.
    :param filename: the name of the sclera image file, or None to use the color property instead.
    :param color: the color of the sclera, in 16 bit 565 RGB format. This only takes effect when filename is None.
    :param spin: the amount of spin to apply to the sclera, in rpm. Set to zero to disable sclera spinning.
    :param iSpin: the amount of spin to apply to the sclera on each frame. Set to zero to disable sclera spinning.
    :param mirror: if true, the sclera will be flipped left to right.
    """
    self.filename = filename
    self.color = color
    self.angle = angle
    self.spin = spin
    self.iSpin = iSpin
    self.mirror = mirror

  @classmethod
  def fromDict(cls: type[ScleraConfig], params: dict[str, object]) -> ScleraConfig:
    return cls(params.get('texture'), _toInt(params.get('color', 0)), _toInt(params.get('angle', 0)),
               _toInt(params.get('spin', 0)), _toInt(params.get('iSpin', 0)), bool(params.get('mirror', False)))


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

  @classmethod
  def fromDict(cls: type[EyelidConfig], params: dict[str, object]) -> EyelidConfig:
    return cls(params.get('upperFilename'), params.get('lowerFilename'), params.get('color', 0))


class EyeConfig:
  """
  A class that holds the configuration for a single eye.
  """

  def __init__(self, name: str, radius: int, tracking: bool, backColor: int, squint: float,
               pupil: PupilConfig, iris: IrisConfig, sclera: ScleraConfig, eyelid: EyelidConfig):
    self.name = name
    self.radius = radius
    self.tracking = tracking
    self.backColor = backColor
    self.squint = squint
    self.pupil = pupil
    self.iris = iris
    self.sclera = sclera
    self.eyelid = eyelid

  @classmethod
  def fromDict(cls: type[EyeConfig], params: dict[str, object]) -> EyeConfig:
    """
    Creates and returns an EyeConfig instance by extracting configuration details from the supplied dictionary.
    :param params: a dictionary containing details of the eye's configuration.
    :return: An EyeConfig instance.
    """
    try:
      name = str(params.get('name'))
      eyeRadius = _toInt(params.get('radius', 120))
      tracking = bool(params.get('tracking', True))
      backColor = _toInt(params.get('backColor', 0))
      squint = float(params.get('squint', 0.5))

      pupilDict = params.get('pupil', {})
      pupil = PupilConfig.fromDict(pupilDict)

      irisDict = params.get('iris', {})
      iris = IrisConfig.fromDict(irisDict)

      scleraDict = params.get('sclera', {})
      sclera = ScleraConfig.fromDict(scleraDict)

      eyelidDict = params.get('eyelid', {})
      eyelid = EyelidConfig.fromDict(eyelidDict)

      return cls(name, eyeRadius, tracking, backColor, squint, pupil, iris, sclera, eyelid)

    except Exception as e:
      raise Exception(f'Unable to convert dictionary into an EyeConfig object: {e}\n{params}')
