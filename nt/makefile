# This is the makefile for WinNT MICROEMACS, using Borland C++.

# Subirectory containing ttykbd.c, tty.c, ttyio.c, and ttydef.h.  The two
# possibilities are Z29 and the current directory (OS2 or NT).
# Z29 is for Zenith Z29, Z19, nd DEC ANSI terminals.
# OS2 or NT is for standard IBM PC video adapters.

TTY = .

# Tools to use

CC = bcc32
LINK = tlink32
MAKE = make

# Where to find the Borland C libraries and header files.
# You'll probably have to edit this line.

BCB = c:\bcb\borland\cbuilder5

# You shouldn't have to modify anything after this point.

TCLIB = $(BCB)\lib
TCINC = $(BCB)\include
NTINC = $(BCB)\include

conlibs = $(TCLIB)\import32.lib

# How to compile modules.

DEBUG = -v
LINKDEBUG = -v
CFLAGS = -w-par -w-pro -Di386 $(DEBUG) $(DEFINES) -I..;$(TCINC)
# ;$(NTINC) 
COMPILE = $(CC) -c $(CFLAGS) { $<}

!if $d(SYSTEM)
.PATH.c   = .
!else
.PATH.c   = ..
!endif

.c.obj :
	$(COMPILE)

# List of object files.

OBJ1 = basic.obj buffer.obj cinfo.obj echo.obj extend.obj file.obj tags.obj
OBJ2 = kbd.obj line.obj main.obj paragraph.obj random.obj region.obj search.obj
OBJ3 = symbol.obj version.obj window.obj word.obj regexp.obj display.obj ring.obj
SYSOBJ = fileio.obj spawn.obj ttyio.obj tty.obj ttykbd.obj
!if $d(SYSTEM)
OBJ  = $(SYSOBJ)
!else
OBJ  = $(OBJ1) $(OBJ2) $(OBJ3)
!endif

# Link MicroEMACS.

!if $d(SYSTEM)

pe.exe : $(OBJ)

!else

pe.exe : $(OBJ) system
	$(CC) -epe -L$(TCLIB) $(LINKDEBUG) $(OBJ) $(SYSOBJ)

system : $(OBJ)
	$(MAKE) -DSYSTEM

!endif

# All C objects depend on three header files.  But it's such a nuisance
# recompiling everything when something tiny changes.  So we'll
# only recompile a few terminal-dependent things when ttydef.h changes.

#$(OBJ) :   def.h sysdef.h $(TTY)\ttydef.h

# Dependencies for system-independent files.

basic.obj:  ..\basic.c
buffer.obj: ..\buffer.c
cinfo.obj:  ..\cinfo.c
display.obj:    ..\display.c $(TTY)\ttydef.h
echo.obj:   ..\echo.c
extend.obj: ..\extend.c
file.obj:   ..\file.c
kbd.obj:    ..\kbd.c
line.obj:   ..\line.c
main.obj:   ..\main.c
paragraph.obj:   ..\paragraph.c
random.obj: ..\random.c
region.obj: ..\region.c
regexp.obj: ..\regexp.c
ring.obj: ..\ring.c
search.obj: ..\search.c
symbol.obj: ..\symbol.c
tags.obj: ..\tags.c
version.obj:    ..\version.c
window.obj: ..\window.c
word.obj:   ..\word.c

# Dependencies for OS-specific files.

fileio.obj: fileio.c
spawn.obj:  spawn.c
ttykbd.obj: $(TTY)\ttykbd.c $(TTY)\ttydef.h
tty.obj:    $(TTY)\tty.c $(TTY)\ttydef.h
ttyio.obj:  $(TTY)\ttyio.c $(TTY)\ttydef.h

# Remove objects

clean:
	-del *.obj
	-del pe.exe
	-del pe.tds

# Create pe.zip archive on the TMP directory.
# The TMP environment variable must be set before running make,
# and defines the temporary file path (typically c:\tmp or c:\temp).

zip :
	-del $(TMP)\pe.zip
	cd ..
	zip $(TMP)\pe *.c *.h *.asm *.mak *.pro
	zip $(TMP)\pe nt\*.c nt\*.h nt\makefile
	cd nt
