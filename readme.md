SFML-Pi
=======

This library allows the use of SFML on Linux-based systems (including the Raspberry Pi) with hardware
accelerated graphics and no X11 dependency.  The library can be compiled to use DRM/KMS (for RPi 4 and non-Pi)
or the legacy DISPMANX libraries (for RPi 0,1,2,3) to display graphics.  The library uses UDEV to handle keyboard
input.

This software has a number of known limitations.  It should work if your program uses a single fullscreen
window and does not need to show the mouse cursor.  It is not intended to handle multiple windows.


Installing (DRM/KMS version)
-------
**Step 1:** Install the SFML dependencies

- See: <http://www.sfml-dev.org/tutorials/2.4/compile-with-cmake.php>
- You need to install the libraries required for building SFML on Linux (other than the
  x11 ones... x11, xrandr, xcb*).
- You also need to install the following additional libraries: "drm-dev", "gbm-dev" and "EGL-dev"

- The command to do this on a debian-based system could look something like this:

`sudo apt-get install cmake libflac-dev libogg-dev libvorbis-dev libopenal-dev libjpeg8-dev libfreetype6-dev libudev-dev libdrm-dev libgbm-dev libegl1-mesa-dev`

**Step 2:** Build the "DRM" version of SFML-Pi

- From the source directory for sfml-pi, run cmake with the '-DSFML_DRM=1' parameter.
- Once cmake runs successfully, compile using 'sudo make install' and then 'sudo ldconfig'
- The commands could look something like this:

```bash
mkdir build
cd build
cmake .. -DSFML_DRM=1
sudo make install
sudo ldconfig
```

OpenGL will be used by default with this mode.  If you want to use OpenGL ES instead, you need to also add the
'-DSFML_OPENGL_ES=1' parameter when you run cmake.

Installing (RPi 0,1,2,3 - DISPMANX version):
-------

**Step 1:** Install the SFML dependencies

- See: <http://www.sfml-dev.org/tutorials/2.4/compile-with-cmake.php>
- You need to install the libraries required for building SFML on Linux (other than the
  x11 ones... x11, xrandr, xcb*).
- The command on rasbian should look something like this:

`sudo apt-get install cmake libflac-dev libogg-dev libvorbis-dev libopenal-dev libjpeg8-dev libfreetype6-dev libudev-dev libraspberrypi-dev`

**Step 2:** Build the "DISPMANX" version of SFML-Pi

- From the source directory for sfml-pi, run cmake with the '-DSFML_RPI=1' parameter.
- Once cmake runs successfully, compile using 'sudo make install' and then 'sudo ldconfig'
- The commands should look something like this:

```bash
mkdir build
cd build
cmake .. -DSFML_RPI=1 -DEGL_INCLUDE_DIR=/opt/vc/include -DEGL_LIBRARY=/opt/vc/lib/libbrcmEGL.so -DGLES_INCLUDE_DIR=/opt/vc/include -DGLES_LIBRARY=/opt/vc/lib/libbrcmGLESv2.so
sudo make install
sudo ldconfig
```

Note: if you are using a Pi firmware older than 1.20160921-1, please replace "libbrcmEGL.so" and "libbrcmGLESv2" with the old names, "libEGL.so" and "libGLESv2".

This mode uses OpenGL ES

More Info
---------
Please consult the SFML readme for more info: [readme-original.md][]
