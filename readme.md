SFML-Pi
=======

This library allows the use of SFML on Linux-based systems (including Raspberry Pis) *WITHOUT* the use of X11.
It supports two options for achieving this: One is to use direct rendering manager/kernel mode setting (DRM/KMS).
The other is specific to Raspberry Pi 0-3 systems and involves using the RPi's legacy "DISPMANX" drivers.

This software should be considered experimental.  It has a number of known limitations.  It might work if you
only need a single fullscreen window.  UDEV is used for the handling of keyboard, mouse and touch input, and
appropriate permissions to /dev/input/event* are required for input handling to work correctly.


Instructions for the DRM/KMS version:
-------
**Step 1:** Install the SFML dependencies on your system

- See: <http://www.sfml-dev.org/tutorials/2.4/compile-with-cmake.php>
- Install the libraries required for building SFML on Linux (other than the x11 ones... x11, xrandr, xcb*).
- Also install the following additional libraries: "drm-dev", "gbm-dev" and "EGL-dev"

- The command to do this on a debian-based system will look something like this:

`sudo apt-get install cmake libflac-dev libogg-dev libvorbis-dev libopenal-dev libjpeg8-dev libfreetype6-dev libudev-dev libdrm-dev libgbm-dev libegl1-mesa-dev`

**Step 2:** Build the "DRM" version of SFML-Pi

- From the source directory for sfml-pi, run cmake with the '-DSFML_DRM=1' parameter.
- Once cmake runs successfully, compile using 'make' and then 'sudo make install' and 'sudo ldconfig'
- The commands will look something like this:

```bash
mkdir build
cd build
cmake .. -DSFML_DRM=1
make -j4
sudo make install
sudo ldconfig
```

This version uses OpenGL by default.  If you want to use OpenGL ES instead, add the '-DSFML_OPENGL_ES=1' parameter
when you run cmake in the instructions above.

SFML-Pi will autodetect the display device, mode and refresh rate to use with drm.  You can override the default
behaviour with the the following environment variables:

- Set "SFML_DRM_DEVICE" to indicate the device you want to use.
- Set "SFML_DRM_MODE" to indicate the video mode you want to use.
- Set "SFML_DRM_RATE" to request a specific refresh rate.
- Set "SFML_DRM_DEBUG" for a console print out of the mode and rate settings used.


Instructions for the (RPi 0-3) DISPMANX version:
-------

**Step 1:** Install the SFML dependencies on your system

- See: <http://www.sfml-dev.org/tutorials/2.4/compile-with-cmake.php>
- Install the libraries required for building SFML on Linux (other than the x11 ones... x11, xrandr, xcb*).
- The command on rasbian should look something like this:

`sudo apt-get install cmake libflac-dev libogg-dev libvorbis-dev libopenal-dev libjpeg8-dev libfreetype6-dev libudev-dev libraspberrypi-dev`

**Step 2:** Build the "DISPMANX" version of SFML-Pi

- From the source directory for sfml-pi, run cmake with the '-DSFML_RPI=1' parameter.
- Once cmake runs successfully, compile using 'make' and then 'sudo make install' and 'sudo ldconfig'
- The commands will look something like this:

```bash
mkdir build
cd build
cmake .. -DSFML_RPI=1 -DEGL_INCLUDE_DIR=/opt/vc/include -DEGL_LIBRARY=/opt/vc/lib/libbrcmEGL.so -DGLES_INCLUDE_DIR=/opt/vc/include -DGLES_LIBRARY=/opt/vc/lib/libbrcmGLESv2.so
make -j4
sudo make install
sudo ldconfig
```

Note: if you are using a Pi firmware older than 1.20160921-1, please replace "libbrcmEGL.so" and "libbrcmGLESv2" with the old names, "libEGL.so" and "libGLESv2".

This mode uses OpenGL ES

More Info
---------
Please consult the SFML readme for more info: [readme-original.md](readme-original.md)
