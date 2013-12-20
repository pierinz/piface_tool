piface_tool
===========

This program reads & sets pins of the piface board, without requiring additional dependencies
It includes a slightly modified version of the C library in https://github.com/thomasmacpherson/piface

Installation
------------

The PiFace board communicates with the Raspberry Pi using the SPI interface.
Ensure the SPI module is loaded: modprobe spi-bcm2708.
Refer to https://github.com/thomasmacpherson/piface/blob/master/README.md#installation-and-setup for additional details about SPI module.

The program itself is installed as usual:
- download the sources
- cd to the source directory
- launch "make"
- launch "make install"

piface_tool is now available at /usr/local/sbin/piface_tool. You can change the "prefix" to install it elsewhere:
- launch "make prefix=/usr install" to install in /usr/sbin

How to use it
-------------

Usage: piface_tool [options] <pin number> [in|out [<value>]]
Read/Set the piface pins.
Pin '8' is a shortcut for all pins.

- -n              Force no init (use if other programs are using the piface)
- -v              Verbose
- -h              Show this message
