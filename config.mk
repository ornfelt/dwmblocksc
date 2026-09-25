# dwmblocksc version (dwmblocksr's)
VERSION = 1.0.0

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# includes and libs
INCS = -I${X11INC}
LIBS = -L${X11LIB} -lX11

# flags
# -Wno-missing-field-initializers: blocks.h leaves a block's battery field
# out when the block is always used
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700L -DVERSION=\"${VERSION}\"
CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Os ${INCS} ${CPPFLAGS}
LDFLAGS  = ${LIBS}

# the unit tests and dwmblocksc-debug: sanitized, for finding memory errors and leaks
DEBUGCFLAGS = -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer

# compiler and linker
CC = cc
