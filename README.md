hvtool
======

`hvtool` is a tool which can convert Windows CE `.hv` files to `.reg`, and back.

It was originally part of the itsutils distribution.

Usage
=====

Dump contents of a registry hive file:

    hvtool user.hv

Create a registry hive file from a utf-8 encoded `.reg` file:

    hvtool -o user.hv user.reg


Install
=======

windows
 * expect openssl to be installed in c:/local/openssl-Win64
 * expect boost to be installed in `c:/local/boost_1_74_0`

freebsd
 * pkg install boost-all

osx
 * brew install openssl


Building
========

The default target in `Makefile` will uild the project using `cmake`.

    make

Alternatively you can use the old, currently unmainted, makefiles:

    make -f Makefile.win32
    make -f Makefile.linux


`cmake` will result in a binary in the `build/tools` subdirectory, while the traditional make will 
create a binary in the current directory.


Author
======

(C) 2011-2012 Willem Hengeveld <itsme@xs4all.nl>

