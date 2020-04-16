#
# Makefile for Unix 2.0
#
# Nov '97 by Zoid <zoid@idsoftware.com>
#
# ELF only
#
# Modified by QuDos at http://qudos.quakedev.com

# Check OS type.
OSTYPE=    $(shell uname -s | tr A-Z a-z)
ARCH=      $(shell uname -m | sed -e s/i.86/i386/)
MARCH=     $(shell uname -m)
OP_SYSTEM= $(shell uname -sr)

MOUNT_DIR=.
VERSION=$(shell grep "\#define Q2PVERSION" $(MOUNT_DIR)/qcommon/qcommon.h | sed -e 's/.*".* \([^ ]*\)"/\1/')
Q2P_VERSION=Q2P $(VERSION)
Q2P_VERSION_BZ2=Q2P-$(VERSION)
BINDIR=quake2

BUILD_DEBUG_DIR:=build_debug
BUILD_RELEASE_DIR:=build_release

#===========================#
# Set to YES or NO for a    #
# verbose compilation       #
# process.                  #
#===========================#
VERBOSE=NO

#===========================#
# Q2P Client                #
#===========================#
BUILD_Q2P=YES

#===========================#
# Enable Optimizations.     #
# Release build only.       #
#===========================#
OPTIMIZE=NO

#===========================#
# Remove symbols from       #
# binaries, reduce size.    #
# Release build only.       #
#===========================#
STRIP=YES

#===========================#
# Sound Drivers             #
#===========================#
ifeq ($(OSTYPE),linux)
BUILD_ALSA_SND=YES
endif
BUILD_OSS_SND=YES
BUILD_SDL_SND=YES

#===========================#
# Quake 2 dedicated server. #
#===========================#
BUILD_DEDICATED=YES

#===========================#
# GLX, SDL-GLX              #
# render.                   #
#===========================#
BUILD_GLX=YES
BUILD_SDLGL=YES

#===========================#
# Linked To GLU library.    #
#===========================#
WITH_GLULIB=YES

#===========================#
# Built in DGA mouse        #
# support.                  #
#===========================#
WITH_XF86_DGA=NO

#===========================#
# Read from $(DATADIR) and  #
# write to "~/.quake2"      #
#===========================#
WITH_DATADIR=NO

#===========================#
# Load renderers            #
# from $(LIBDIR)            #
#===========================#
WITH_LIBDIR=NO

#===========================#
# Built in CD Audio         #
# support.                  #
#===========================#
WITH_CDAUDIO=NO
ifeq ($(OSTYPE),freebsd)
CD_API=cd_freebsd.c
endif
ifeq ($(OSTYPE),linux)
CD_API=cd_linux.c
endif

#===========================#
# Enable XMMS/Audacious     #
# support. Only one of them #
# at same time.             #
# No yet for FreeBSD :(     #
#===========================#
ifeq ($(OSTYPE),linux)
WITH_AUDACIOUS=YES
WITH_XMMS=NO
endif

#===========================#
# Quake 2 game library.     #
#===========================#
BUILD_GAME=YES

#===========================#
# CTF Library.              #
#===========================#
BUILD_CTF=YES


CLIENT_DIR=$(MOUNT_DIR)/client
SERVER_DIR=$(MOUNT_DIR)/server
REF_GL_DIR=$(MOUNT_DIR)/ref_gl
COMMON_DIR=$(MOUNT_DIR)/qcommon
UNIX_DIR=$(MOUNT_DIR)/unix
GAME_DIR=$(MOUNT_DIR)/game
CTF_DIR=$(MOUNT_DIR)/ctf

CC=gcc
CC_VERSION=$(shell $(CC) -dumpversion)

X11BASE= /usr/X11R6
LOCALBASE= /usr/local
GAMEBASE= /usr/local
SYSBINDIR=$(LOCALBASE)/bin

ifeq ($(OSTYPE),freebsd)
 DATADIR= $(GAMEBASE)/share/quake2
 LIBDIR= $(GAMEBASE)/lib/quake2
else
 DATADIR= $(GAMEBASE)/games/quake2
 LIBDIR= $(GAMEBASE)/games/quake2
endif

SHLIB_EXT=so
GAME_NAME=game$(ARCH).$(SHLIB_EXT)

BASE_CFLAGS+=-I$(LOCALBASE)/include \
            -I$(X11BASE)/include \
            -DOP_SYSTEM='\"$(OP_SYSTEM)\"' \
            -DGAME_NAME='\"$(GAME_NAME)\"' \
            -DQ2P_VERSION='\"$(VERSION)\"' \
            -DCC_VERSION='\"$(CC_VERSION)\"' \
	    -Wall \
	    -pipe #-Werror

RELEASE_CFLAGS+=$(BASE_CFLAGS) \
               -ffloat-store \
	       -fno-strict-aliasing \
	       -DNDEBUG

ifeq ($(strip $(OPTIMIZE)),YES)
 RELEASE_CFLAGS+=-O3 \
                 -march=$(MARCH) \
                 -funroll-loops \
                 -fstrength-reduce \
                 -fexpensive-optimizations \
                 -falign-loops=2 \
                 -falign-jumps=2 \
                 -falign-functions=2
else
 RELEASE_CFLAGS+=-O2
endif

ifeq ($(strip $(STRIP)),YES)
 RELEASE_CFLAGS+=-s
endif

OGG_LDFLAGS= -lvorbisfile -lvorbis -logg

ifeq ($(strip $(WITH_GLULIB)),YES)
 GLU_CFLAGS+=-DUSE_GLU
endif

ifeq ($(strip $(WITH_XF86_DGA)),YES)
 BASE_CFLAGS+=-DUSE_XF86_DGA
endif

ifeq ($(ARCH),x86_64)
 ARCH_LIBDIR=64
endif

ifeq ($(strip $(WITH_DATADIR)),YES)
 BASE_CFLAGS+=-DDATADIR='\"$(DATADIR)\"'
endif

ifeq ($(strip $(WITH_LIBDIR)),YES)
 BASE_CFLAGS+=-DLIBDIR='\"$(LIBDIR)\"'
endif

ifeq ($(strip $(WITH_CDAUDIO)),YES)
 CDAUDIO_CFLAGS+=-DCDAUDIO
endif

GLIB_CFLAGS=$(shell glib-config --cflags)

ifeq ($(strip $(WITH_AUDACIOUS)),YES)
 AUDACIOUS_CFLAGS=$(GLIB_CFLAGS) -DWITH_AUDACIOUS
endif

ifeq ($(strip $(WITH_XMMS)),YES)
 XMMS_CFLAGS=$(GLIB_CFLAGS) -DWITH_XMMS
endif

DEBUG_CFLAGS=$(BASE_CFLAGS) -O0 -g

LDFLAGS=-L$(LOCALBASE)/lib -lm -lpthread
ifeq ($(OSTYPE),linux)
 LDFLAGS+=-ldl
endif

Z_LDFLAGS=-L$(LOCALBASE)/lib -lz

X_LDFLAGS=-L$(X11BASE)/lib$(ARCH_LIBDIR) -lX11 -lXext -lXxf86vm
ifeq ($(strip $(WITH_XF86_DGA)),YES)
 X_LDFLAGS+=-lXxf86dga
endif

GLX_LDFLAGS+=-L$(LOCALBASE)/lib -ljpeg `libpng-config --libs`
ifeq ($(strip $(WITH_GLULIB)),YES)
GLX_LDFLAGS+=-L$(X11BASE)/lib$(ARCH_LIBDIR) -lGLU
endif

SDL_CONFIG=sdl-config
SDL_CFLAGS=$(shell $(SDL_CONFIG) --cflags)
SDL_LDFLAGS=$(shell $(SDL_CONFIG) --libs)
SDL_GL_CFLAGS=$(SDL_CFLAGS)
SDL_GL_LDFLAGS=$(SDL_LDFLAGS)

SHLIB_CFLAGS=-fPIC -DPIC
SHLIB_LDFLAGS= $(LDFLAGS) -shared

ifeq ($(VERBOSE),NO)
CC_OUTPUT=echo "  Compiling >> $< ..." &&
endif

DO_CC=$(CC_OUTPUT) $(CC) $(CFLAGS) -o $@ -c $<
DO_SHLIB_CC=$(CC_OUTPUT) $(CC) $(CFLAGS) $(SHLIB_CFLAGS) -o $@ -c $<

#############################################################################
# SETUP AND BUILD
#############################################################################

ifeq ($(strip $(BUILD_Q2P)),YES)
 TARGETS += $(BINDIR)/q2p
endif

ifeq ($(strip $(BUILD_DEDICATED)),YES)
 TARGETS += $(BINDIR)/q2p-dedicated
 DED_CFLAGS += -DDEDICATED_ONLY
endif

ifeq ($(OSTYPE),linux)
 ifeq ($(strip $(BUILD_ALSA_SND)),YES)
  ALSA_LDFLAGS +=-lasound
  TARGETS += $(BINDIR)/snd_alsa.$(SHLIB_EXT)
 endif
endif

ifeq ($(strip $(BUILD_OSS_SND)),YES)
 TARGETS += $(BINDIR)/snd_oss.$(SHLIB_EXT)
endif

ifeq ($(strip $(BUILD_SDL_SND)),YES)
 TARGETS += $(BINDIR)/snd_sdl.$(SHLIB_EXT)
endif


ifeq ($(strip $(BUILD_GLX)),YES)
 TARGETS += $(BINDIR)/vid_gl.$(SHLIB_EXT)
endif

ifeq ($(strip $(BUILD_SDLGL)),YES)
 TARGETS += $(BINDIR)/vid_sdl.$(SHLIB_EXT)
endif
	
ifeq ($(strip $(BUILD_GAME)),YES)
 TARGETS+=$(BINDIR)/baseq2/game$(ARCH).$(SHLIB_EXT)
endif
	
ifeq ($(strip $(BUILD_CTF)),YES)
 TARGETS+=$(BINDIR)/ctf/game$(ARCH).$(SHLIB_EXT)
endif

all:
	@echo 
	@echo "Operative system: $(OP_SYSTEM)"
	@echo 
	@echo Set to YES or NO at the top of this file the possible options to build by the makefile.
	@echo By default, it will build q2p, q2p dedicated and glx renderer.
	@echo 
	@echo Possible targets:
	@echo
	@echo "   "">> Add VERBOSE=YES to a verbose output, defaults to NO"
	@echo "   "">> WARNING: -ffast-math and/or -fomit-frame-pointer flags can cause crashes."
	@echo "   "">> make/gmake release"
	@echo "   "">> make/gmake debug"
	@echo "   "">> make/gmake install (quake2 home dir)."
	@echo "   "">> make/gmake install_root (required when was built with DATADIR/LIBDIR" 
	@echo "   ""                      options enabled, you must gain root privileges)."
	@echo "   "">> make/gmake clean (clean objects)."
	@echo "   "">> make/gmake clean_bin (clean executables)."
	@echo "   "">> make/gmake distclean (clean objects, executables and modified files)."
	@echo "   "">> make/gmake bz2 (create a tar.bz2 package with the full release distribution)."
	@echo
	
debug:
	@printf "** Debug Build **\n"
	@-mkdir -p $(BUILD_DEBUG_DIR) \
		$(BINDIR) \
		$(BUILD_DEBUG_DIR)/client
		
ifeq ($(strip $(BUILD_DEDICATED)),YES)
	@-mkdir -p $(BUILD_DEBUG_DIR) \
		$(BUILD_DEBUG_DIR)/ded 
endif
		
ifeq ($(strip $(BUILD_GLX)),YES)
	@-mkdir -p $(BUILD_DEBUG_DIR) \
		$(BUILD_DEBUG_DIR)/ref_gl
endif
		
ifeq ($(strip $(BUILD_SDLGL)),YES)
	@-mkdir -p $(BUILD_DEBUG_DIR) \
		$(BUILD_DEBUG_DIR)/ref_gl
endif

ifeq ($(strip $(BUILD_GAME)),YES)
	@-mkdir -p $(BUILD_DEBUG_DIR) \
		$(BINDIR)/baseq2 \
		$(BUILD_DEBUG_DIR)/game 
endif

ifeq ($(strip $(BUILD_CTF)),YES)
	@-mkdir -p $(BUILD_DEBUG_DIR) \
		$(BINDIR)/ctf \
		$(BUILD_DEBUG_DIR)/ctf
endif

ifeq ($(VERBOSE),YES)
	$(MAKE) targets BUILDDIR=$(BUILD_DEBUG_DIR) CFLAGS="$(DEBUG_CFLAGS)";
else
	@-$(MAKE) --silent targets BUILDDIR=$(BUILD_DEBUG_DIR) CFLAGS="$(DEBUG_CFLAGS)";
endif
	@printf "Done \n"

release:
	@printf "** Release Build **\n"
	@-mkdir -p $(BUILD_RELEASE_DIR) \
		$(BINDIR) \
		$(BUILD_RELEASE_DIR)/client
		
ifeq ($(strip $(BUILD_DEDICATED)),YES)
	@-mkdir -p $(BUILD_RELEASE_DIR) \
		$(BUILD_RELEASE_DIR)/ded 
endif
		
ifeq ($(strip $(BUILD_GLX)),YES)
	@-mkdir -p $(BUILD_RELEASE_DIR) \
		$(BUILD_RELEASE_DIR)/ref_gl 
endif
		
ifeq ($(strip $(BUILD_SDLGL)),YES)
	@-mkdir -p $(BUILD_RELEASE_DIR) \
		$(BUILD_RELEASE_DIR)/ref_gl 
endif
		
ifeq ($(strip $(BUILD_GAME)),YES)
	@-mkdir -p $(BUILD_RELEASE_DIR) \
		$(BINDIR)/baseq2 \
		$(BUILD_RELEASE_DIR)/game
endif
		
ifeq ($(strip $(BUILD_CTF)),YES)
	@-mkdir -p $(BUILD_RELEASE_DIR) \
		$(BINDIR)/ctf \
		$(BUILD_RELEASE_DIR)/ctf
endif

ifeq ($(VERBOSE),YES)
	$(MAKE) targets BUILDDIR=$(BUILD_RELEASE_DIR) CFLAGS="$(RELEASE_CFLAGS)";
else
	@-$(MAKE) --silent targets BUILDDIR=$(BUILD_RELEASE_DIR) CFLAGS="$(RELEASE_CFLAGS)";
endif
	@printf "Done \n"
        
targets: $(TARGETS)
	@echo "Q2P compilation finished succesfully ;)"
	@echo 
	@echo "'make/gmake install' to install in your quake2 home dir."
	@echo
	@echo "'make/gmake install_root' if was built with DATADIR/LIBDIR options enabled, you must gain root privileges."
	@echo
#	@xmessage -center -bg black -fg green -file Q2P_readme.txt &

#############################################################################
# CLIENT/SERVER
#############################################################################

Q2P_OBJS = \
$(BUILDDIR)/client/cl_cin.o   $(BUILDDIR)/client/cl_ents.o    $(BUILDDIR)/client/cl_events.o \
$(BUILDDIR)/client/cl_flash.o $(BUILDDIR)/client/cl_fx.o      $(BUILDDIR)/client/cl_input.o \
$(BUILDDIR)/client/cl_inv.o   $(BUILDDIR)/client/cl_lights.o  $(BUILDDIR)/client/cl_main.o \
$(BUILDDIR)/client/cl_newfx.o $(BUILDDIR)/client/cl_parse.o   $(BUILDDIR)/client/cl_pred.o \
$(BUILDDIR)/client/cl_tent.o  $(BUILDDIR)/client/cl_scrn.o    $(BUILDDIR)/client/cl_view.o \
$(BUILDDIR)/client/console.o  $(BUILDDIR)/client/keys.o       $(BUILDDIR)/client/menu.o \
$(BUILDDIR)/client/snd_dma.o  $(BUILDDIR)/client/snd_mem.o    $(BUILDDIR)/client/snd_mix.o \
$(BUILDDIR)/client/snd_ogg.o  $(BUILDDIR)/client/qmenu.o      $(BUILDDIR)/client/m_flash.o \
$(BUILDDIR)/client/cmd.o      $(BUILDDIR)/client/cmodel.o     $(BUILDDIR)/client/common.o \
$(BUILDDIR)/client/crc.o      $(BUILDDIR)/client/cvar.o       $(BUILDDIR)/client/files.o \
$(BUILDDIR)/client/md4.o      $(BUILDDIR)/client/net_chan.o \
$(BUILDDIR)/client/unzip.o    $(BUILDDIR)/client/ioapi.o \
$(BUILDDIR)/client/sv_ccmds.o $(BUILDDIR)/client/sv_ents.o    $(BUILDDIR)/client/sv_game.o \
$(BUILDDIR)/client/sv_init.o  $(BUILDDIR)/client/sv_main.o    $(BUILDDIR)/client/sv_send.o \
$(BUILDDIR)/client/sv_user.o  $(BUILDDIR)/client/sv_world.o \
$(BUILDDIR)/client/sh_unix.o  $(BUILDDIR)/client/vid_so.o     $(BUILDDIR)/client/sys_unix.o \
$(BUILDDIR)/client/net_udp.o  $(BUILDDIR)/client/menu_video.o $(BUILDDIR)/client/cl_think.o \
$(BUILDDIR)/client/audacious_player.o \
$(BUILDDIR)/client/xmms_player.o \
$(BUILDDIR)/client/cl_locs.o \
$(BUILDDIR)/client/q_shared.o $(BUILDDIR)/client/pmove.o

CD_OBJS = \
	$(BUILDDIR)/client/cd_unix.o 
	
ifeq ($(OSTYPE),linux)
 ifeq ($(strip $(BUILD_ALSA_SND)),YES)
  ALSA_SND_OBJS= $(BUILDDIR)/client/snd_alsa.o
 endif
endif

ifeq ($(strip $(BUILD_OSS_SND)),YES)
 OSS_SND_OBJS= $(BUILDDIR)/client/snd_oss.o
endif

ifeq ($(strip $(BUILD_SDL_SND)),YES)
 SDL_SND_OBJS= $(BUILDDIR)/client/snd_sdl.o
endif

$(BINDIR)/q2p : $(Q2P_OBJS) $(CD_OBJS)
	@echo
	@echo "**Built Q2P client with cflags:"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(AUDACIOUS_CFLAGS)";
	@echo "$(XMMS_CFLAGS)";
	@echo "$(CDAUDIO_CFLAGS)";
	@echo
	@echo "**Linking Client with flags:"
	@echo "$(LDFLAGS)";
	@echo "$(Z_LDFLAGS)";
	@echo "$(OGG_LDFLAGS)";
	@echo
	$(CC) $(CFLAGS) $(GLIB_CFLAGS) $(AUDACIOUS_CFLAGS) $(XMMS_CFLAGS) $(CDAUDIO_CFLAGS) -o $@ $(Q2P_OBJS) $(CD_OBJS) $(LDFLAGS) $(Z_LDFLAGS) $(OGG_LDFLAGS)

ifeq ($(OSTYPE),linux)
$(BINDIR)/snd_alsa.$(SHLIB_EXT) : $(ALSA_SND_OBJS)
	@echo
	@echo "**Built Alsa sound driver with cflags:"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(SHLIB_CFLAGS)";
	@echo
	@echo "**Linking Alsa sound driver with flags:"
	@echo "$(SHLIB_LDFLAGS)";
	@echo "$(ALSA_LDFLAGS)";
	@echo
	$(CC) $(CFLAGS) $(SHLIB_LDFLAGS) -o $@ $(ALSA_SND_OBJS) $(LDFLAGS) $(ALSA_LDFLAGS)
endif
	
$(BINDIR)/snd_oss.$(SHLIB_EXT) : $(OSS_SND_OBJS)
	@echo
	@echo "**Built OSS sound driver with cflags:"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(SHLIB_CFLAGS)";
	@echo
	@echo "**Linking OSS sound driver with flags:"
	@echo "$(SHLIB_LDFLAGS)";
	@echo
	$(CC) $(CFLAGS) $(SHLIB_LDFLAGS) -o $@ $(OSS_SND_OBJS) $(LDFLAGS)

$(BINDIR)/snd_sdl.$(SHLIB_EXT) : $(SDL_SND_OBJS)
	@echo
	@echo "**Built SDL sound driver with cflags:"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(SHLIB_CFLAGS)";
	@echo
	@echo "**Linking SDL sound driver with flags:"
	@echo "$(SHLIB_LDFLAGS)";
	@echo "$(SDL_LDFLAGS)";
	@echo
	$(CC) $(CFLAGS) $(SHLIB_LDFLAGS) -o $@ $(SDL_SND_OBJS) $(LDFLAGS) $(SDL_LDFLAGS)


$(BUILDDIR)/client/cl_cin.o :      $(CLIENT_DIR)/cl_cin.c;      $(DO_CC) $(CDAUDIO_CFLAGS)
$(BUILDDIR)/client/cl_ents.o :     $(CLIENT_DIR)/cl_ents.c;     $(DO_CC)
$(BUILDDIR)/client/cl_events.o :   $(CLIENT_DIR)/cl_events.c;   $(DO_CC)
$(BUILDDIR)/client/cl_fx.o :       $(CLIENT_DIR)/cl_fx.c;       $(DO_CC)
$(BUILDDIR)/client/cl_think.o :    $(CLIENT_DIR)/cl_think.c;    $(DO_CC)
$(BUILDDIR)/client/cl_flash.o :    $(CLIENT_DIR)/cl_flash.c;    $(DO_CC)
$(BUILDDIR)/client/cl_input.o :    $(CLIENT_DIR)/cl_input.c;    $(DO_CC)
$(BUILDDIR)/client/cl_inv.o :      $(CLIENT_DIR)/cl_inv.c;      $(DO_CC)
$(BUILDDIR)/client/cl_lights.o :   $(CLIENT_DIR)/cl_lights.c;   $(DO_CC)
$(BUILDDIR)/client/cl_locs.o :     $(CLIENT_DIR)/cl_locs.c;     $(DO_CC)
$(BUILDDIR)/client/cl_main.o :     $(CLIENT_DIR)/cl_main.c;     $(DO_CC) $(CDAUDIO_CFLAGS) $(AUDACIOUS_CFLAGS) $(XMMS_CFLAGS)
$(BUILDDIR)/client/cl_newfx.o :    $(CLIENT_DIR)/cl_newfx.c;    $(DO_CC)
$(BUILDDIR)/client/cl_parse.o :    $(CLIENT_DIR)/cl_parse.c;    $(DO_CC) $(CDAUDIO_CFLAGS)
$(BUILDDIR)/client/cl_pred.o :     $(CLIENT_DIR)/cl_pred.c;     $(DO_CC)
$(BUILDDIR)/client/cl_tent.o :     $(CLIENT_DIR)/cl_tent.c;     $(DO_CC)
$(BUILDDIR)/client/cl_scrn.o :     $(CLIENT_DIR)/cl_scrn.c;     $(DO_CC) $(CDAUDIO_CFLAGS)
$(BUILDDIR)/client/cl_view.o :     $(CLIENT_DIR)/cl_view.c;     $(DO_CC) $(CDAUDIO_CFLAGS)
$(BUILDDIR)/client/console.o :     $(CLIENT_DIR)/console.c;     $(DO_CC)
$(BUILDDIR)/client/keys.o :        $(CLIENT_DIR)/keys.c;        $(DO_CC)
$(BUILDDIR)/client/menu.o :        $(CLIENT_DIR)/menu.c;        $(DO_CC)
$(BUILDDIR)/client/menu_video.o :  $(CLIENT_DIR)/menu_video.c;  $(DO_CC)
$(BUILDDIR)/client/snd_dma.o :     $(CLIENT_DIR)/snd_dma.c;     $(DO_CC) $(AUDACIOUS_CFLAGS) $(XMMS_CFLAGS)
$(BUILDDIR)/client/snd_mem.o :     $(CLIENT_DIR)/snd_mem.c;     $(DO_CC)
$(BUILDDIR)/client/snd_mix.o :     $(CLIENT_DIR)/snd_mix.c;     $(DO_CC)
$(BUILDDIR)/client/snd_ogg.o :     $(CLIENT_DIR)/snd_ogg.c;     $(DO_CC)
$(BUILDDIR)/client/qmenu.o :       $(CLIENT_DIR)/qmenu.c;       $(DO_CC)
$(BUILDDIR)/client/cmd.o :         $(COMMON_DIR)/cmd.c;         $(DO_CC)
$(BUILDDIR)/client/cmodel.o :      $(COMMON_DIR)/cmodel.c;      $(DO_CC)
$(BUILDDIR)/client/common.o :      $(COMMON_DIR)/common.c;      $(DO_CC)
$(BUILDDIR)/client/crc.o :         $(COMMON_DIR)/crc.c;         $(DO_CC)
$(BUILDDIR)/client/cvar.o :        $(COMMON_DIR)/cvar.c;        $(DO_CC)
$(BUILDDIR)/client/files.o :       $(COMMON_DIR)/files.c;       $(DO_CC) $(CDAUDIO_CFLAGS)
$(BUILDDIR)/client/md4.o :         $(COMMON_DIR)/md4.c;         $(DO_CC)
$(BUILDDIR)/client/net_chan.o :    $(COMMON_DIR)/net_chan.c;    $(DO_CC)
$(BUILDDIR)/client/unzip.o :       $(COMMON_DIR)/unzip/unzip.c; $(DO_CC)
$(BUILDDIR)/client/ioapi.o :       $(COMMON_DIR)/unzip/ioapi.c; $(DO_CC)
$(BUILDDIR)/client/pmove.o :       $(COMMON_DIR)/pmove.c;       $(DO_CC)
$(BUILDDIR)/client/m_flash.o :     $(GAME_DIR)/m_flash.c;       $(DO_CC)
$(BUILDDIR)/client/q_shared.o :    $(GAME_DIR)/q_shared.c;      $(DO_CC)
$(BUILDDIR)/client/sv_ccmds.o :    $(SERVER_DIR)/sv_ccmds.c;    $(DO_CC)
$(BUILDDIR)/client/sv_ents.o :     $(SERVER_DIR)/sv_ents.c;     $(DO_CC)
$(BUILDDIR)/client/sv_game.o :     $(SERVER_DIR)/sv_game.c;     $(DO_CC)
$(BUILDDIR)/client/sv_init.o :     $(SERVER_DIR)/sv_init.c;     $(DO_CC)
$(BUILDDIR)/client/sv_main.o :     $(SERVER_DIR)/sv_main.c;     $(DO_CC)
$(BUILDDIR)/client/sv_send.o :     $(SERVER_DIR)/sv_send.c;     $(DO_CC)
$(BUILDDIR)/client/sv_user.o :     $(SERVER_DIR)/sv_user.c;     $(DO_CC)
$(BUILDDIR)/client/sv_world.o :    $(SERVER_DIR)/sv_world.c;    $(DO_CC)
$(BUILDDIR)/client/cd_unix.o :     $(UNIX_DIR)/$(CD_API);       $(DO_CC) $(CDAUDIO_CFLAGS)
$(BUILDDIR)/client/sh_unix.o :     $(UNIX_DIR)/sh_unix.c;       $(DO_CC)
$(BUILDDIR)/client/vid_so.o :      $(UNIX_DIR)/vid_so.c;        $(DO_CC)
$(BUILDDIR)/client/snd_alsa.o :    $(UNIX_DIR)/snd_alsa.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/client/snd_oss.o :     $(UNIX_DIR)/snd_oss.c;       $(DO_SHLIB_CC)
$(BUILDDIR)/client/snd_sdl.o :     $(UNIX_DIR)/snd_sdl.c;       $(DO_SHLIB_CC) $(SDL_CFLAGS)
$(BUILDDIR)/client/sys_unix.o :    $(UNIX_DIR)/sys_unix.c;      $(DO_CC)
$(BUILDDIR)/client/net_udp.o :     $(UNIX_DIR)/net_udp.c;       $(DO_CC)
$(BUILDDIR)/client/audacious_player.o : $(UNIX_DIR)/audacious_player.c; $(DO_CC) $(AUDACIOUS_CFLAGS)
$(BUILDDIR)/client/xmms_player.o : $(UNIX_DIR)/xmms_player.c;   $(DO_CC) $(XMMS_CFLAGS)

#############################################################################
# DEDICATED SERVER
#############################################################################

Q2PDED_OBJS = \
$(BUILDDIR)/ded/cmd.o      $(BUILDDIR)/ded/cmodel.o   $(BUILDDIR)/ded/common.o \
$(BUILDDIR)/ded/crc.o      $(BUILDDIR)/ded/cvar.o     $(BUILDDIR)/ded/files.o \
$(BUILDDIR)/ded/md4.o      $(BUILDDIR)/ded/net_chan.o \
$(BUILDDIR)/ded/unzip.o    $(BUILDDIR)/ded/ioapi.o \
$(BUILDDIR)/ded/sv_ccmds.o $(BUILDDIR)/ded/sv_ents.o  $(BUILDDIR)/ded/sv_game.o \
$(BUILDDIR)/ded/sv_init.o  $(BUILDDIR)/ded/sv_main.o  $(BUILDDIR)/ded/sv_send.o \
$(BUILDDIR)/ded/sv_user.o  $(BUILDDIR)/ded/sv_world.o \
$(BUILDDIR)/ded/sh_unix.o  $(BUILDDIR)/ded/sys_unix.o $(BUILDDIR)/ded/net_udp.o \
$(BUILDDIR)/ded/q_shared.o $(BUILDDIR)/ded/pmove.o \
$(BUILDDIR)/ded/cl_null.o  $(BUILDDIR)/ded/cd_null.o

$(BINDIR)/q2p-dedicated : $(Q2PDED_OBJS)
	@echo
	@echo "**Built Q2P dedicated client with cflags:"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(DED_CFLAGS)";
	@echo "$(CDAUDIO_CFLAGS)";
	@echo
	@echo "**Linking Dedicated client with flags:"
	@echo "$(LDFLAGS)";
	@echo "$(Z_LDFLAGS)";
	@echo
	$(CC) $(CFLAGS) $(DED_CFLAGS) $(CDAUDIO_CFLAGS) -o $@ $(Q2PDED_OBJS) $(LDFLAGS) $(Z_LDFLAGS)

$(BUILDDIR)/ded/cmd.o :        $(COMMON_DIR)/cmd.c;         $(DO_CC)
$(BUILDDIR)/ded/cmodel.o :     $(COMMON_DIR)/cmodel.c;      $(DO_CC)
$(BUILDDIR)/ded/common.o :     $(COMMON_DIR)/common.c;      $(DO_CC) $(DED_CFLAGS)
$(BUILDDIR)/ded/crc.o :        $(COMMON_DIR)/crc.c;         $(DO_CC)
$(BUILDDIR)/ded/cvar.o :       $(COMMON_DIR)/cvar.c;        $(DO_CC)
$(BUILDDIR)/ded/files.o :      $(COMMON_DIR)/files.c;       $(DO_CC)
$(BUILDDIR)/ded/md4.o :        $(COMMON_DIR)/md4.c;         $(DO_CC)
$(BUILDDIR)/ded/net_chan.o :   $(COMMON_DIR)/net_chan.c;    $(DO_CC)
$(BUILDDIR)/ded/unzip.o :      $(COMMON_DIR)/unzip/unzip.c; $(DO_CC)
$(BUILDDIR)/ded/ioapi.o :      $(COMMON_DIR)/unzip/ioapi.c; $(DO_CC)
$(BUILDDIR)/ded/q_shared.o :   $(GAME_DIR)/q_shared.c;      $(DO_CC)
$(BUILDDIR)/ded/pmove.o :      $(COMMON_DIR)/pmove.c;       $(DO_CC)
$(BUILDDIR)/ded/sv_ccmds.o :   $(SERVER_DIR)/sv_ccmds.c;    $(DO_CC)
$(BUILDDIR)/ded/sv_ents.o :    $(SERVER_DIR)/sv_ents.c;     $(DO_CC)
$(BUILDDIR)/ded/sv_game.o :    $(SERVER_DIR)/sv_game.c;     $(DO_CC)
$(BUILDDIR)/ded/sv_init.o :    $(SERVER_DIR)/sv_init.c;     $(DO_CC)
$(BUILDDIR)/ded/sv_main.o :    $(SERVER_DIR)/sv_main.c;     $(DO_CC)
$(BUILDDIR)/ded/sv_send.o :    $(SERVER_DIR)/sv_send.c;     $(DO_CC)
$(BUILDDIR)/ded/sv_user.o :    $(SERVER_DIR)/sv_user.c;     $(DO_CC)
$(BUILDDIR)/ded/sv_world.o :   $(SERVER_DIR)/sv_world.c;    $(DO_CC)
$(BUILDDIR)/ded/sh_unix.o :    $(UNIX_DIR)/sh_unix.c;       $(DO_CC)
$(BUILDDIR)/ded/sys_unix.o :   $(UNIX_DIR)/sys_unix.c;      $(DO_CC) $(DED_CFLAGS)
$(BUILDDIR)/ded/net_udp.o :    $(UNIX_DIR)/net_udp.c;       $(DO_CC)
$(BUILDDIR)/ded/cd_null.o :    $(CLIENT_DIR)/cd_null.c;     $(DO_CC) $(CDAUDIO_CFLAGS)
$(BUILDDIR)/ded/cl_null.o :    $(CLIENT_DIR)/cl_null.c;     $(DO_CC) $(DED_CFLAGS)
        
#############################################################################
# REF_GL
#############################################################################

REF_GL_OBJS = \
$(BUILDDIR)/ref_gl/gl_decals.o $(BUILDDIR)/ref_gl/gl_draw.o  $(BUILDDIR)/ref_gl/gl_image.o \
$(BUILDDIR)/ref_gl/gl_light.o  $(BUILDDIR)/ref_gl/gl_mesh.o  $(BUILDDIR)/ref_gl/gl_model.o \
$(BUILDDIR)/ref_gl/gl_refl.o   $(BUILDDIR)/ref_gl/gl_rmain.o $(BUILDDIR)/ref_gl/gl_rmisc.o \
$(BUILDDIR)/ref_gl/gl_rsurf.o  $(BUILDDIR)/ref_gl/gl_warp.o  $(BUILDDIR)/ref_gl/qgl.o \
$(BUILDDIR)/ref_gl/sh_unix.o   $(BUILDDIR)/ref_gl/q_shared.o

REF_GL_GLX_OBJS = \
$(BUILDDIR)/ref_gl/gl_glx.o
	
REF_SDL_GLX_OBJS = \
$(BUILDDIR)/ref_gl/gl_sdl.o

$(BINDIR)/vid_gl.$(SHLIB_EXT) : $(REF_GL_OBJS) $(REF_GL_GLX_OBJS)
	@echo
	@echo "**Built GLX video driver with cflags:"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(GLU_CFLAGS)";
	@echo
	@echo "**Linking GLX video driver with flags:"
	@echo "$(X_LDFLAGS)";
	@echo "$(GLX_LDFLAGS)";
	@echo "$(SHLIB_LDFLAGS)";
	@echo
	$(CC) $(CFLAGS) $(GLU_CFLAGS) $(SHLIB_LDFLAGS) -o $@ $(REF_GL_OBJS) $(REF_GL_GLX_OBJS) $(X_LDFLAGS) $(GLX_LDFLAGS)

$(BINDIR)/vid_sdl.$(SHLIB_EXT) : $(REF_GL_OBJS) $(REF_SDL_GLX_OBJS)
	@echo
	@echo "**Built SDL video driver with cflags:"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(GLU_CFLAGS)";
	@echo
	@echo "**Linking SDL video driver with flags:"
	@echo "$(SDL_GL_LDFLAGS)";
	@echo "$(GLX_LDFLAGS)";
	@echo "$(SHLIB_LDFLAGS)";
	@echo
	$(CC) $(CFLAGS) $(GLU_CFLAGS) $(SHLIB_LDFLAGS) -o $@ $(REF_GL_OBJS) $(REF_SDL_GLX_OBJS) $(GLX_LDFLAGS) $(SDL_GL_LDFLAGS)
        
$(BUILDDIR)/ref_gl/gl_decals.o : $(REF_GL_DIR)/gl_decals.c; $(DO_SHLIB_CC)
$(BUILDDIR)/ref_gl/gl_draw.o :   $(REF_GL_DIR)/gl_draw.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/ref_gl/gl_image.o :  $(REF_GL_DIR)/gl_image.c;  $(DO_SHLIB_CC) $(GLU_CFLAGS)
$(BUILDDIR)/ref_gl/gl_light.o :  $(REF_GL_DIR)/gl_light.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/ref_gl/gl_mesh.o :   $(REF_GL_DIR)/gl_mesh.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/ref_gl/gl_model.o :  $(REF_GL_DIR)/gl_model.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/ref_gl/gl_refl.o :   $(REF_GL_DIR)/gl_refl.c;   $(DO_SHLIB_CC) $(GLU_CFLAGS)
$(BUILDDIR)/ref_gl/gl_rmain.o :  $(REF_GL_DIR)/gl_rmain.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/ref_gl/gl_rmisc.o :  $(REF_GL_DIR)/gl_rmisc.c;  $(DO_SHLIB_CC) $(GLU_CFLAGS)
$(BUILDDIR)/ref_gl/gl_rsurf.o :  $(REF_GL_DIR)/gl_rsurf.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/ref_gl/gl_warp.o :   $(REF_GL_DIR)/gl_warp.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/ref_gl/qgl.o :       $(REF_GL_DIR)/qgl.c;       $(DO_SHLIB_CC) $(GLU_CFLAGS)
$(BUILDDIR)/ref_gl/gl_glx.o :    $(UNIX_DIR)/gl_glx.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/ref_gl/gl_sdl.o :    $(UNIX_DIR)/gl_sdl.c;      $(DO_SHLIB_CC) $(SDL_GL_CFLAGS)
$(BUILDDIR)/ref_gl/sh_unix.o :   $(UNIX_DIR)/sh_unix.c;     $(DO_SHLIB_CC)		
$(BUILDDIR)/ref_gl/q_shared.o :  $(GAME_DIR)/q_shared.c;    $(DO_SHLIB_CC)
        
#############################################################################
# GAME
#############################################################################

GAME_OBJS = \
$(BUILDDIR)/game/q_shared.o   $(BUILDDIR)/game/g_ai.o        $(BUILDDIR)/game/p_client.o \
$(BUILDDIR)/game/g_cmds.o     $(BUILDDIR)/game/g_svcmds.o    $(BUILDDIR)/game/g_chase.o \
$(BUILDDIR)/game/g_combat.o   $(BUILDDIR)/game/g_func.o      $(BUILDDIR)/game/g_items.o \
$(BUILDDIR)/game/g_main.o     $(BUILDDIR)/game/g_misc.o      $(BUILDDIR)/game/g_monster.o \
$(BUILDDIR)/game/g_phys.o     $(BUILDDIR)/game/g_save.o      $(BUILDDIR)/game/g_spawn.o \
$(BUILDDIR)/game/g_target.o   $(BUILDDIR)/game/g_trigger.o   $(BUILDDIR)/game/g_turret.o \
$(BUILDDIR)/game/g_utils.o    $(BUILDDIR)/game/g_weapon.o    $(BUILDDIR)/game/m_actor.o \
$(BUILDDIR)/game/m_berserk.o  $(BUILDDIR)/game/m_boss2.o     $(BUILDDIR)/game/m_boss3.o \
$(BUILDDIR)/game/m_boss31.o   $(BUILDDIR)/game/m_boss32.o    $(BUILDDIR)/game/m_brain.o \
$(BUILDDIR)/game/m_chick.o    $(BUILDDIR)/game/m_flipper.o   $(BUILDDIR)/game/m_float.o \
$(BUILDDIR)/game/m_flyer.o    $(BUILDDIR)/game/m_gladiator.o $(BUILDDIR)/game/m_gunner.o \
$(BUILDDIR)/game/m_hover.o    $(BUILDDIR)/game/m_infantry.o  $(BUILDDIR)/game/m_insane.o \
$(BUILDDIR)/game/m_medic.o    $(BUILDDIR)/game/m_move.o      $(BUILDDIR)/game/m_mutant.o \
$(BUILDDIR)/game/m_parasite.o $(BUILDDIR)/game/m_soldier.o   $(BUILDDIR)/game/m_supertank.o \
$(BUILDDIR)/game/m_tank.o     $(BUILDDIR)/game/p_hud.o       $(BUILDDIR)/game/p_trail.o \
$(BUILDDIR)/game/p_view.o     $(BUILDDIR)/game/p_weapon.o    $(BUILDDIR)/game/m_flash.o

$(BINDIR)/baseq2/game$(ARCH).$(SHLIB_EXT) : $(GAME_OBJS)
	@echo
	@echo "**Built $@ with cflags:"
	@echo "$(CC) $(CC_VERSION) $(CFLAGS)";
	@echo "$(SHLIB_CFLAGS)";
	@echo
	@echo "**Linking $@ with flags:"
	@echo "$(SHLIB_LDFLAGS)";
	@echo
	$(CC) $(CFLAGS) $(SHLIB_LDFLAGS) -o $@ $(GAME_OBJS)

$(BUILDDIR)/game/g_ai.o :        $(GAME_DIR)/g_ai.c;        $(DO_SHLIB_CC)
$(BUILDDIR)/game/p_client.o :    $(GAME_DIR)/p_client.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_cmds.o :      $(GAME_DIR)/g_cmds.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_svcmds.o :    $(GAME_DIR)/g_svcmds.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_chase.o :     $(GAME_DIR)/g_chase.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_combat.o :    $(GAME_DIR)/g_combat.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_func.o :      $(GAME_DIR)/g_func.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_items.o :     $(GAME_DIR)/g_items.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_main.o :      $(GAME_DIR)/g_main.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_misc.o :      $(GAME_DIR)/g_misc.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_monster.o :   $(GAME_DIR)/g_monster.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_phys.o :      $(GAME_DIR)/g_phys.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_save.o :      $(GAME_DIR)/g_save.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_spawn.o :     $(GAME_DIR)/g_spawn.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_target.o :    $(GAME_DIR)/g_target.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_trigger.o :   $(GAME_DIR)/g_trigger.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_turret.o :    $(GAME_DIR)/g_turret.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_utils.o :     $(GAME_DIR)/g_utils.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/g_weapon.o :    $(GAME_DIR)/g_weapon.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_actor.o :     $(GAME_DIR)/m_actor.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_berserk.o :   $(GAME_DIR)/m_berserk.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_boss2.o :     $(GAME_DIR)/m_boss2.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_boss3.o :     $(GAME_DIR)/m_boss3.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_boss31.o :    $(GAME_DIR)/m_boss31.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_boss32.o :    $(GAME_DIR)/m_boss32.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_brain.o :     $(GAME_DIR)/m_brain.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_chick.o :     $(GAME_DIR)/m_chick.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_flipper.o :   $(GAME_DIR)/m_flipper.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_float.o :     $(GAME_DIR)/m_float.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_flyer.o :     $(GAME_DIR)/m_flyer.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_gladiator.o : $(GAME_DIR)/m_gladiator.c; $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_gunner.o :    $(GAME_DIR)/m_gunner.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_hover.o :     $(GAME_DIR)/m_hover.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_infantry.o :  $(GAME_DIR)/m_infantry.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_insane.o :    $(GAME_DIR)/m_insane.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_medic.o :     $(GAME_DIR)/m_medic.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_move.o :      $(GAME_DIR)/m_move.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_mutant.o :    $(GAME_DIR)/m_mutant.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_parasite.o :  $(GAME_DIR)/m_parasite.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_soldier.o :   $(GAME_DIR)/m_soldier.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_supertank.o : $(GAME_DIR)/m_supertank.c; $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_tank.o :      $(GAME_DIR)/m_tank.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/game/p_hud.o :       $(GAME_DIR)/p_hud.c;       $(DO_SHLIB_CC)
$(BUILDDIR)/game/p_trail.o :     $(GAME_DIR)/p_trail.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/game/p_view.o :      $(GAME_DIR)/p_view.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/game/p_weapon.o :    $(GAME_DIR)/p_weapon.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/q_shared.o :    $(GAME_DIR)/q_shared.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/game/m_flash.o :     $(GAME_DIR)/m_flash.c;     $(DO_SHLIB_CC)

#############################################################################
# CTF
#############################################################################

CTF_OBJS = \
$(BUILDDIR)/ctf/g_ai.o      $(BUILDDIR)/ctf/g_chase.o  $(BUILDDIR)/ctf/g_cmds.o \
$(BUILDDIR)/ctf/g_combat.o  $(BUILDDIR)/ctf/g_ctf.o    $(BUILDDIR)/ctf/g_func.o \
$(BUILDDIR)/ctf/g_items.o   $(BUILDDIR)/ctf/g_main.o   $(BUILDDIR)/ctf/g_misc.o \
$(BUILDDIR)/ctf/g_monster.o $(BUILDDIR)/ctf/g_phys.o   $(BUILDDIR)/ctf/g_save.o \
$(BUILDDIR)/ctf/g_spawn.o   $(BUILDDIR)/ctf/g_svcmds.o $(BUILDDIR)/ctf/g_target.o \
$(BUILDDIR)/ctf/g_trigger.o $(BUILDDIR)/ctf/g_utils.o  $(BUILDDIR)/ctf/g_weapon.o \
$(BUILDDIR)/ctf/m_move.o    $(BUILDDIR)/ctf/p_client.o $(BUILDDIR)/ctf/p_hud.o \
$(BUILDDIR)/ctf/p_menu.o    $(BUILDDIR)/ctf/p_trail.o  $(BUILDDIR)/ctf/p_view.o \
$(BUILDDIR)/ctf/p_weapon.o  $(BUILDDIR)/ctf/q_shared.o

$(BINDIR)/ctf/game$(ARCH).$(SHLIB_EXT) : $(CTF_OBJS)
	@echo
	@echo "**Built $@ with cflags:"
	@echo "$(CC) $(CC_VERSION) $(CFLAGS)";
	@echo "$(SHLIB_CFLAGS)";
	@echo
	@echo "**Linking $@ with flags:"
	@echo "$(SHLIB_LDFLAGS)";
	@echo
	$(CC) $(CFLAGS) $(SHLIB_LDFLAGS) -o $@ $(CTF_OBJS)

$(BUILDDIR)/ctf/g_ai.o :      $(CTF_DIR)/g_ai.c;      $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_chase.o :   $(CTF_DIR)/g_chase.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_cmds.o :    $(CTF_DIR)/g_cmds.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_combat.o :  $(CTF_DIR)/g_combat.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_ctf.o :     $(CTF_DIR)/g_ctf.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_func.o :    $(CTF_DIR)/g_func.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_items.o :   $(CTF_DIR)/g_items.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_main.o :    $(CTF_DIR)/g_main.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_misc.o :    $(CTF_DIR)/g_misc.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_monster.o : $(CTF_DIR)/g_monster.c; $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_phys.o :    $(CTF_DIR)/g_phys.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_save.o :    $(CTF_DIR)/g_save.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_spawn.o :   $(CTF_DIR)/g_spawn.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_svcmds.o :  $(CTF_DIR)/g_svcmds.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_target.o :  $(CTF_DIR)/g_target.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_trigger.o : $(CTF_DIR)/g_trigger.c; $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_utils.o :   $(CTF_DIR)/g_utils.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/g_weapon.o :  $(CTF_DIR)/g_weapon.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/m_move.o :    $(CTF_DIR)/m_move.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/p_client.o :  $(CTF_DIR)/p_client.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/p_hud.o :     $(CTF_DIR)/p_hud.c;     $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/p_menu.o :    $(CTF_DIR)/p_menu.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/p_trail.o :   $(CTF_DIR)/p_trail.c;   $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/p_view.o :    $(CTF_DIR)/p_view.c;    $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/p_weapon.o :  $(CTF_DIR)/p_weapon.c;  $(DO_SHLIB_CC)
$(BUILDDIR)/ctf/q_shared.o :  $(CTF_DIR)/q_shared.c;  $(DO_SHLIB_CC)

#############################################################################
# MISC
#############################################################################

DATE=`date +%F`

clean:
	@echo
	@echo Cleaning objects...
	@rm -rf \
	$(BUILD_DEBUG_DIR) \
	$(BUILD_RELEASE_DIR)
	@echo ...................
	@echo .... Done
	
clean_bin:
	@echo
	@echo Cleaning binaries...
	@-rm -rf $(BINDIR)
	@echo ....................
	@echo .... Done
	
clean_cl:
	@echo
	@echo Cleaning client objects...
	@-rm -f $(BINDIR)/snd_* \
	@-rm -f $(BINDIR)/q2p* \
	@-rm -f $(BUILD_RELEASE_DIR)/client/* \
	@-rm -f $(BUILD_DEBUG_DIR)/client/* 
	@echo ....................
	@echo .... Done
	
clean_gl:
	@echo
	@echo Cleaning gl objects...
	@-rm -f $(BINDIR)/vid_* \
	@-rm -f $(BUILD_RELEASE_DIR)/ref_gl/* \
	@-rm -f $(BUILD_DEBUG_DIR)/ref_gl/* 
	@echo ....................
	@echo .... Done
	
distclean:
	@echo
	@echo Cleaning objects and binaries...
	@-rm -rf $(BUILD_DEBUG_DIR) $(BUILD_RELEASE_DIR) $(BINDIR)
	@-rm -f `find . \( -not -type d \) -and \
		\( -name '*~' \) -type f -print`
	@echo ................................
	@echo .... Done
	
dist:distclean
	@echo
	@printf "Creating bzip2 for source files...\n"
	@mkdir -p ../$(Q2P_VERSION_BZ2)-$(DATE)
	@cp -r * ../$(Q2P_VERSION_BZ2)-$(DATE)
	@tar cjvf ../$(Q2P_VERSION_BZ2)-$(DATE).tar.bz2 \
		  ../$(Q2P_VERSION_BZ2)-$(DATE)
	@rm -rf ../$(Q2P_VERSION_BZ2)-$(DATE)
	@printf ".... Done.\n"

install:
	@echo
	@printf "Copying files to your home dir\n"
	@echo ......
	@cp -rv $(BINDIR) $(HOME)
	@cp -rv gnu.txt Q2P_readme.txt Ogg_readme.txt unix/q2p.run $(HOME)/$(BINDIR)
	@chmod +x $(HOME)/$(BINDIR)/q2p.run
	@cp -rv $(MOUNT_DIR)/data/baseq2/q2p.q2z $(HOME)/$(BINDIR)/baseq2
	@mkdir -pv $(HOME)/bin
	@cd $(HOME)/bin && ln -svf $(HOME)/$(BINDIR)/q2p.run q2p
	@printf "Symlinking executable to $(HOME)/bin\n"
	@echo
	@printf "You must to set the PATH $(HOME)/bin to the executable search path in order to run q2p.\n" 
	@printf "See the Q2P_readme.txt\n"
	@echo
	@echo Done 
install_root:
	@echo
	@mkdir -pv $(DATADIR)
	@printf "Copying files to $(DATADIR)\n"
ifeq ($(OSTYPE),freebsd)
	@cp -rv $(BINDIR) $(LOCALBASE)/share
	@rm -fv $(DATADIR)/vid_* $(DATADIR)/snd_*
	@mkdir -pv $(LIBDIR)
	@cp -rv $(BINDIR)/*.so $(LIBDIR)
else
	@cp -rv $(BINDIR) $(LOCALBASE)/games
endif
	@cp -rv $(MOUNT_DIR)/unix/q2p.run unix/q2p.png $(DATADIR)
	@chmod +x $(DATADIR)/q2p.run
	@cp -rv $(MOUNT_DIR)/data/baseq2/q2p.q2z $(DATADIR)/baseq2
	@cp -rv $(MOUNT_DIR)/unix/Q2P.desktop /home/*/Desktop/Q2P
	@ln -sfv $(DATADIR)/q2p.run $(SYSBINDIR)/q2p
	@printf "Symlinking executable to $(SYSBINDIR)\n"
	@echo 
	@printf "Copy or link into $(DATADIR)/baseq2 the required pak files and players folder\n" 
	@printf "from your quake2 cdrom in order to run. Also get CTF files from id ftp.\n"
	@echo 
	@printf "Type q2p as user to start.\n"
ifeq ($(strip $(BUILD_DEDICATED)),YES)
	@printf "Type q2p +set dedicated +exec <your_server_config_file>.cfg as user to start a dedicated server.\n"
endif
	@echo 
	@echo Done 
bz2:
	@echo
	@printf "Creating bzip2 compressed file ...\n"
	@cp -r gnu.txt Q2P_readme.txt Ogg_readme.txt unix/q2p.run $(BINDIR)
	@-mkdir -p $(BINDIR)/baseq2
	@cp -r $(MOUNT_DIR)/data/baseq2/q2p.q2z $(BINDIR)/baseq2
	@tar cjvf $(Q2P_VERSION_BZ2)-$(OSTYPE)-$(DATE).tar.bz2 $(BINDIR)
	@printf ".... Done.\n"
	
	