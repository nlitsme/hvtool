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

Either using the more traditional `Makefile`:

    make

or

    make -f Makefile.win32

Or using the cmake build system:

    make -f Makefile.cm

`cmake` will result in a binary in the `build` subdirectory, while the traditional make will 
create a binary in the current directory.

Author
======

(C) 2011-2012 Willem Hengeveld <itsme@xs4all.nl>

