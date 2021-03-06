NAME = dwmstatus
VERSION = 1.0

# Customize below to fit your system

# paths
PREFIX = /usr
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# includes and libs
INCS = -I. -I/usr/include -I${X11INC}
#LIBS = -L/usr/lib -lc -L${X11LIB} -lX11

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_DEFAULT_SOURCE
#CFLAGS = -g -Wall -O0 ${INCS} ${CPPFLAGS}
CFLAGS = -g -std=c99 -pedantic -Wall -O0 ${INCS} ${CPPFLAGS}
#CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
LDFLAGS = -g ${LIBS}
#LDFLAGS = -s ${LIBS}

# Solaris
#CFLAGS = -fast ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}

# compiler and linker
CC = cc

# mpd & alsa
MPDLIB   =  -lmpdclient
MPDFLAG  =  -DMPD

#ALSALIB   =  -lasound
PULSELIB   =  -lpulse
PULSEFLAG   =  -libs

#LIBS = -L/usr/lib -lc -L${X11LIB} -lX11 ${MPDLIB} ${ALSALIB}
LIBS = -L/usr/lib -lc -L${X11LIB} -lX11 ${MPDLIB} ${PULSELIB}

CPPFLAGS = ${MPDFLAG} ${PULSEFLAG} -DVERSION=\"${VERSION}\"
