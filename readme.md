SFML-Pi
=======

SFML on the Raspberry Pi with hardware graphics and no X11 dependency.

Install
-------

**Step 1:** Install the SFML dependencies

- See: <http://www.sfml-dev.org/tutorials/2.3/compile-with-cmake.php>
- You need to install the libraries required for building SFML on Linux (other than the
  x11 ones... x11, xrandr, xcb*).
- The command on rasbian should look something like this:

`sudo apt-get install cmake libflac-dev libogg-dev libvorbis-dev libopenal-dev libjpeg8-dev libfreetype6-dev libudev-dev`

**Step 2:** Build SFML-Pi

- From the source directory for sfml-pi, run cmake with the '-DSFML_RPI=1' parameter.
- Once cmake runs successfully, compile using 'sudo make install' and then 'sudo ldconfig'
- The commands should look something like this:

```bash
mkdir build
cd build
cmake .. -DSFML_RPI=1 -DEGL_INCLUDE_DIR=/opt/vc/include -DEGL_LIBRARY=/opt/vc/lib/libEGL.so -DGLES_INCLUDE_DIR=/opt/vc/include -DGLES_LIBRARY=/opt/vc/lib/libGLESv1_CM.so
sudo make install
sudo ldconfig
```

That's it!  You should now be able to use SFML on your raspberry pi with hardware accelerated
graphics and no need to be running X.

More Info
---------
Please consult the SFML readme for more info: [readme.txt][]
