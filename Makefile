# $Header$

CXX     := c++
AR      := ar cru
RANLIB  := ranlib
MKDIR   := mkdir -p
ECHO    := echo -n
CAT     := cat
RM      := rm -f
RM_REC  := $(RM) -r
ZIP     := zip -q
CP      := cp

#######################################################################
# Default compilation parameters. Normally don't edit these           #
#######################################################################

DEFINES := -DHAVE_CONFIG_H
LDFLAGS :=
INCLUDES:= -I. -Icommon
LIBS	:=
OBJS	:=

MODULES :=
MODULE_DIRS :=

EXECUTABLE := scummvm$(EXEEXT)

# Load the make rules generated by configure
include config.mak

# Uncomment this for stricter compile time code verification
# CXXFLAGS+= -Wshadow -Werror

CXXFLAGS:= -O -Wall -Wuninitialized -Wno-long-long -Wno-multichar -Wno-unknown-pragmas $(CXXFLAGS)
# Even more warnings...
CXXFLAGS+= -pedantic -Wpointer-arith -Wcast-qual -Wcast-align -Wconversion
CXXFLAGS+= -Wshadow -Wimplicit -Wundef -Wnon-virtual-dtor
CXXFLAGS+= -Wno-reorder -Wwrite-strings -fcheck-new -Wctor-dtor-privacy 

#######################################################################
# Misc stuff - you should normally never have to edit this            #
#######################################################################

include Makefile.common

# check if configure has been run or has been changed since last run
config.mak: configure
	@echo "You need to run ./configure before you can run make"
	@echo "Either you haven't run it before or it has changed."
	@echo "If you cannot run configure, use 'make -f Makefile.noconf'"
	@exit 1

dist:
	$(RM) $(ZIPFILE)
	$(ZIP) $(ZIPFILE) $(DISTFILES)

deb:
	ln -sf dists/debian;
	debian/prepare
	fakeroot debian/rules binary


# Special target to create a application wrapper for Mac OS X
bundle_name = ScummVM.app
bundle: scummvm-static
	mkdir -p $(bundle_name)/Contents/MacOS
	mkdir -p $(bundle_name)/Contents/Resources
	echo "APPL????" > $(bundle_name)/Contents/PkgInfo
	cp Info.plist $(bundle_name)/Contents/
	cp scummvm.icns $(bundle_name)/Contents/Resources/
	cp scummvm-static $(bundle_name)/Contents/MacOS/scummvm
	strip $(bundle_name)/Contents/MacOS/scummvm

# Special target to create a static linked binary for Mac OS X
scummvm-static: $(OBJS)
	$(CXX) $(LDFLAGS) -o scummvm-static $(OBJS) \
		/sw/lib/libSDLmain.a /sw/lib/libSDL.a \
		/sw/lib/libmad.a \
		/sw/lib/libvorbisfile.a /sw/lib/libvorbis.a /sw/lib/libogg.a \
		-framework Cocoa -framework Carbon -framework IOKit \
		-framework OpenGL -framework AGL -framework QuickTime \
		-framework AudioUnit -framework AudioToolbox

.PHONY: deb bundle
