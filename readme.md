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

`sudo apt-get install cmake libflac-dev libogg-dev libvorbis-dev libopenal-dev libjpeg8-dev libfreetype6-dev libudev-dev libraspberrypi-dev`

**Step 2:** Build SFML-Pi

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

That's it!  You should now be able to use SFML on your raspberry pi with hardware accelerated
graphics and no need to be running X.

More Info
---------
Please consult the SFML readme for more info: [readme.txt][]
=======
![SFML logo](http://www.sfml-dev.org/images/logo.png)

# SFML — Simple and Fast Multimedia Library

SFML is a simple, fast, cross-platform and object-oriented multimedia API. It provides access to windowing, graphics, audio and network. It is written in C++, and has bindings for various languages such as C, .Net, Ruby, Python.

## Authors

  - Laurent Gomila — main developer (laurent@sfml-dev.org)
  - Marco Antognini — OS X developer (hiura@sfml-dev.org)
  - Jonathan De Wachter — Android developer (dewachter.jonathan@gmail.com)
  - Jan Haller (bromeon@sfml-dev.org)
  - Stefan Schindler (tank@sfml-dev.org)
  - Lukas Dürrenberger (eXpl0it3r@sfml-dev.org)
  - binary1248 (binary1248@hotmail.com)
  - Artur Moreira (artturmoreira@gmail.com)
  - Mario Liebisch (mario@sfml-dev.org)

## Download

You can get the latest official release on [SFML's website](http://www.sfml-dev.org/download.php). You can also get the current development version from the [Git repository](https://github.com/SFML/SFML).

## Install

Follow the instructions of the [tutorials](http://www.sfml-dev.org/tutorials/), there is one for each platform/compiler that SFML supports.

## Learn

There are several places to learn SFML:

  * The [official tutorials](http://www.sfml-dev.org/tutorials/)
  * The [online API documentation](http://www.sfml-dev.org/documentation/)
  * The [community wiki](https://github.com/SFML/SFML/wiki/)
  * The [community forum](http://en.sfml-dev.org/forums/) ([French](http://fr.sfml-dev.org/forums/))

## Contribute

SFML is an open-source project, and it needs your help to go on growing and improving. If you want to get involved and suggest some additional features, file a bug report or submit a patch, please have a look at the [contribution guidelines](http://www.sfml-dev.org/contribute.php).
