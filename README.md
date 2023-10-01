# MicroEMACS (Dave Conroy version with enhancements)

This is a small version of MicroEMACS that is little changed from
the version originally posted by Dave Conroy to USENET in 1986.  It
predates the very popular Daniel Lawrence version of MicroEMACS, which
is much larger.

I have added a few features over the years, most recently support for
etags, cscope, ispell, Rails, undo, UTF-8, regular expression search and
replace, and extensions written in Ruby.  In the past I ported it to
numerous operating systems, but currently this source tree supports
only Linux and Windows (using MinGW).  I seem to have lost the source
for the DOS and OS/2 versions, but they are unlikely to be useful in
the future.

To build a non-debug version with no Ruby support on Linux,
FreeBSD, or Windows with MinGW or Cygwin:

    mkdir obj
    cd obj
    ../configure
    make # gmake on FreeBSD

Dave Conroy released his source code into the public domain.  I have
changed my version to use the GNU General Public License Version 3.

There is a web version of the MicroEMACS manual [here](https://www.bloovis.com/meguide/).

--Mark Alexander (marka@pobox.com)
