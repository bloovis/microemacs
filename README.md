# MicroEMACS (Dave Conroy version with enhancements)

This is a small version of MicroEMACS that is little changed from
the version originally posted by Dave Conroy to USENET in 1986.  It
predates the very popular Daniel Lawrence version of MicroEMACS, which
is much larger.

I have added a few features over the years, most recently support for
etags, cscope, Rails, undo, UTF-8, and extensions written in Ruby.  In
the past I ported it to numerous operating systems, but currently this
source tree supports only Linux and NT.  I seem to have lost the source
for the DOS and OS/2 versions, but they are unlikely to be useful in
the future.

To build a non-debug version with no Ruby support on Linux:

    mkdir obj
    cd obj
    ../configure
    make

To build on NT, you must have BCB 5 installed and have Borland make
in your path.  Then do the following:

    cd nt
    edit makefile and change the line that defines the BCB directory
    make

Note that the NT version will probably not build correctly any more,
and I have no way to test it.

Dave Conroy released his source code into the public domain.  I have
changed my version to use the GNU General Public License Version 3.

--Mark Alexander (marka@pobox.com)
