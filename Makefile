
DEBUG = 0

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
endif
endif

ifeq ($(platform), unix)
   TARGET := libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
   ENDIANNESS_DEFINES := -DLSB_FIRST
   LDFLAGS +=
   FLAGS += -DHAVE_MKDIR
else ifeq ($(platform), osx)
   TARGET := libretro.dylib
   STATICLIB := libretro.a
   fpic := -fPIC -lreadline
   SHARED := -dynamiclib
   ENDIANNESS_DEFINES := -DLSB_FIRST
   LDFLAGS +=
   FLAGS += -DHAVE_MKDIR
else
   TARGET := retro.dll
   CC = gcc
   CXX = g++
   SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
   LDFLAGS += -static-libgcc -static-libstdc++ -lwinmm
   ENDIANNESS_DEFINES := -DLSB_FIRST
   FLAGS += -DHAVE__MKDIR
endif
PROSYSTEM_DIR := core

PROSYSTEM_SOURCES := $(PROSYSTEM_DIR)/afile.c \
	$(PROSYSTEM_DIR)/atari.c \
	$(PROSYSTEM_DIR)/cartridge.c \
	$(PROSYSTEM_DIR)/cassette.c \
	$(PROSYSTEM_DIR)/binload.c \
	$(PROSYSTEM_DIR)/gtia.c \
	$(PROSYSTEM_DIR)/memory.c \
	$(PROSYSTEM_DIR)/pbi.c \
	$(PROSYSTEM_DIR)/pokey.c \
	$(PROSYSTEM_DIR)/pokeysnd.c \
	$(PROSYSTEM_DIR)/sio.c \
	$(PROSYSTEM_DIR)/util.c \
	$(PROSYSTEM_DIR)/sndsave.c \
	$(PROSYSTEM_DIR)/rtime.c \
	$(PROSYSTEM_DIR)/log.c \
	$(PROSYSTEM_DIR)/antic.c \
	$(PROSYSTEM_DIR)/cpu.c \
	$(PROSYSTEM_DIR)/devices.c \
	$(PROSYSTEM_DIR)/esc.c \
	$(PROSYSTEM_DIR)/ide.c \
	$(PROSYSTEM_DIR)/pia.c \
	$(PROSYSTEM_DIR)/cfg.c \
	$(PROSYSTEM_DIR)/pbi_mio.c \
	$(PROSYSTEM_DIR)/pbi_bb.c \
	$(PROSYSTEM_DIR)/pbi_scsi.c \
	$(PROSYSTEM_DIR)/monitor.c \
	$(PROSYSTEM_DIR)/mzpokeysnd.c \
	$(PROSYSTEM_DIR)/compfile.c \
	$(PROSYSTEM_DIR)/remez.c \
	$(PROSYSTEM_DIR)/screen.c \
	$(PROSYSTEM_DIR)/colours_external.c \
	$(PROSYSTEM_DIR)/colours_ntsc.c \
	$(PROSYSTEM_DIR)/colours_pal.c \
	$(PROSYSTEM_DIR)/colours.c \
	$(PROSYSTEM_DIR)/statesav.c \
	$(PROSYSTEM_DIR)/ui_basic.c \
	$(PROSYSTEM_DIR)/ui.c \
	$(PROSYSTEM_DIR)/input.c \
	$(PROSYSTEM_DIR)/sound_oss.c

LIBRETRO_SOURCES := libretro.c

SOURCES := $(LIBRETRO_SOURCES) $(PROSYSTEM_SOURCES)
OBJECTS := $(SOURCES:.c=.o)

all: $(TARGET) $(STATICLIB)

ifeq ($(DEBUG),0)
   FLAGS += -O3 -ffast-math -funroll-loops 
else
   FLAGS += -O0 -g
endif

LDFLAGS += $(fpic) -lz $(SHARED)
FLAGS += -msse -msse2 -Wall $(fpic) -fno-strict-overflow
FLAGS += -I. -Icore -Icore/atari_ntsc

WARNINGS := -Wall \
	-Wno-narrowing \
	-Wno-unused-but-set-variable \
	-Wno-sign-compare \
	-Wno-unused-variable \
	-Wno-unused-function \
	-Wno-uninitialized \
	-Wno-unused-result \
	-Wno-strict-aliasing \
	-Wno-overflow

FLAGS += -DLSB_FIRST -DHAVE_MKDIR -DSIZEOF_DOUBLE=8 $(WARNINGS)

CXXFLAGS += $(FLAGS) -DHAVE_INTTYPES
CFLAGS += $(FLAGS) -std=gnu99

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	
$(STATICLIB): $(OBJECTS)
	ar rcs $(STATICLIB) $(OBJECTS)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(TARGET) $(OBJECTS) $(STATICLIB)

.PHONY: clean
