# TeensyEyes

My take on "Uncanny Eyes" for the Teensy 4.0 with GC9A01A screens. Work in progress...

Based on the following projects:
  - https://github.com/adafruit/Uncanny_Eyes
  - https://github.com/adafruit/Adafruit_Learning_System_Guides/tree/main/M4_Eyes
  - https://github.com/mjs513/GC9A01A_t3n

This project is developed with PlatformIO and (optionally) CLion.
To build and run it, you will need to first run the following from within the TeensyEyes directory:
```
mkdir lib
cd lib
git clone https://github.com/mjs513/GC9A01A_t3n
```
This will check out the GC9A01A_t3n library, which is required to use the GC9A01 LCD screens.

Running the "PlatformIO Upload | eyes" target from CLion, or alternatively running
```platformio run --target upload -e eyes```
will then compile the firmware and upload it to your Teensy 4.x.

Here's a video of the eyes in action:
[![Teensy Eyes](http://img.youtube.com/vi/Ke1SJ8-6zJw/0.jpg)](https://www.youtube.com/watch?v=Ke1SJ8-6zJw "Teensy Eyes")