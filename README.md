YUSB
====

YUSB is a Yorick plug-in to implement USB control into Yorick.


Installation
------------

Prior to installation, make sure you have
[USB library](http://sourceforge.net/projects/libusb) installed on your
system with development files.  For instance, on a Debian (or Ubuntu) based
system:
````{.sh}
sudo apt-get install libusb-1.0-0-dev
````

In short, building and installing the plug-in can be as quick as:
````{.sh}
cd $BUILD_DIR
$SRC_DIR/configure
make
make install
````
where `$BUILD_DIR` is the build directory (at your convenience) and
`$SRC_DIR` is the source directory of the plug-in code.  The build and
source directories can be the same in which case, call `./configure` to
configure for building.

If the plug-in has been properly installed, it is sufficient to use any
of its function to automatically load the plug-in.  You may force the
loading of the plug-in by something like:
````{.cpp}
#include "usb.i"
````
or
````{.cpp}
require, "usb.i";
````
in your code.

More detailled installation explanations are given below.

1. You must have Yorick and the USB library (libusb-1.0) installed on your
   machine.  (See the *"Links"* section below.)

2. Unpack the plug-in code somewhere.

3. Configure for compilation.  There are two possibilities:

   For an **in-place build**, go to the source directory, say `$SRC_DIR`, of
   the plug-in code and run the configuration script:
   ````{.sh}
   cd $SRC_DIR
   ./configure
   ````
   To see the configuration options, type:
   ````{.sh}
   ./configure --help
   ````

   To compile in a **different build directory**, say `$BUILD_DIR`, create the
   build directory, go to the build directory and run the configuration
   script:
   ````{.sh}
   mkdir -p $BUILD_DIR
   cd $BUILD_DIR
   $SRC_DIR/configure
   ````
   where `$SRC_DIR` is the path to the source directory of the plug-in code.
   To see the configuration options, type:
   ````{.sh}
   $SRC_DIR/configure --help
   ````

4. Compile the code:
   ````{.sh}
   make
   ````

4. Install the plug-in in Yorick directories:
   ````{.sh}
   make install
   ````


License
-------

YUSB is open source sofware released under the MIT license.


Links
-----

 * Yorick: <http://github.com/LLNL/yorick/>;
 * USB library: <http://sourceforge.net/projects/libusb>
