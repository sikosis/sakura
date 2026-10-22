## Haiku Generic Makefile v2.6 ##

NAME = Sakura
TYPE = APP

ARCHITECTURES = all !x86_gcc2
SECONDARY_ARCHITECTURES = x86

APP_MIME_SIG = application/x-vnd.sikosis-Sakura

TAGLIB_CONFIG := $(shell command -v taglib-config 2>/dev/null)
ifeq ($(TAGLIB_CONFIG),)
$(error TagLib development files are missing. Run: pkgman install taglib_devel)
endif

SRCS = \
	src/AboutWindow.cpp \
	src/AudioPlayer.cpp \
	src/LocalLibraryScanner.cpp \
	src/MainWindow.cpp \
	src/SakuraApp.cpp \
	src/TrackListItem.cpp

RDEFS = resources/Sakura.rdef
RSRCS =

LIBS = $(STDCPPLIBS) be media tag translation localestub tracker
LIBPATHS =
SYSTEM_INCLUDE_PATHS =
LOCAL_INCLUDE_PATHS = src

OPTIMIZE = FULL
LOCALES = en_AU
DEFINES =
WARNINGS = ALL
SYMBOLS :=
DEBUGGER :=
COMPILER_FLAGS = -std=c++17 $(shell $(TAGLIB_CONFIG) --cflags)
LINKER_FLAGS =
APP_VERSION = 0.1

DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
