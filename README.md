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


Author
======

(C) 2011-2012 Willem Hengeveld <itsme@xs4all.nl>

