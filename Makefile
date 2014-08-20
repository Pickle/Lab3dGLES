
ifeq ($(BUILDTYPE),PANDORA)
PREFIX	= $(PNDSDK)
TOOLS	= bin
TARGET	= arm-none-linux-gnueabi-
else ifeq ($(BUILDTYPE),WIZ)
PREFIX	= $(WIZSDK)
TOOLS	= bin
TARGET	= arm-openwiz-linux-gnu-
else ifeq ($(BUILDTYPE),CAANOO)
PREFIX	= $(CAANOOSDK)
TOOLS	= tools/gcc-4.2.4-glibc-2.7-eabi/bin
TARGET	= arm-gph-linux-gnueabi-
else ifeq ($(BUILDTYPE),GCW)
PREFIX	= $(GCWSDK)
TOOLS	= bin
TARGET	= mipsel-gcw0-linux-uclibc-
endif

CC = $(PREFIX)/$(TOOLS)/$(TARGET)gcc

OBJS=subs.o init.o graphx.o lab3d.o setup.o adlibemu.o oldlab3d.o eglport.o

CFLAGS=-g -Wall -O3 -fsigned-char -ffast-math -fomit-frame-pointer -fstrict-aliasing -DOPENGLES -DUSE_GLES1

ifeq ($(BUILDTYPE),PANDORA)
CFLAGS += `$(PREFIX)/usr/bin/sdl-config --cflags` -I$(PREFIX)/usr/include \
          -DPANDORA -DUSE_EGL_RAW -mcpu=cortex-a8 -mtune=cortex-a8 -march=armv7-a -mfloat-abi=softfp -mfpu=neon -ftree-vectorize \
          -fno-strict-aliasing -fsingle-precision-constant
LDFLAGS += -L$(PREFIX)/usr/lib `$(PREFIX)/usr/bin/sdl-config --libs` \
           -lSDL_image -lX11 -lXau -lXdmcp -lGLES_CM -lEGL -lIMGegl -lsrv_um -lm
else ifeq ($(BUILDTYPE),WIZ)
OBJS += wizcaanoo.o
CFLAGS += `$(PREFIX)/bin/sdl-config --cflags` -I$(PREFIX)/include \
	  -DWIZ -DWIZ_TIMER
LDFLAGS += -L$(PREFIX)/lib `$(PREFIX)/bin/sdl-config --libs` \
           -lSDL_image -lopengles_lite -lglport -lpng12 -lz -lstdc++
else ifeq ($(BUILDTYPE),CAANOO)
OBJS += wizcaanoo.o
CFLAGS += -DCAANOO \
	 -I$(PREFIX)/DGE/include -I$(PREFIX)/DGE/include/SDL
LDFLAGS += -L$(PREFIX)/DGE/lib/target -lSDL_image -lSDL -lts -lopengles_lite -lglport -lpng12 -lz -lstdc++
else ifeq ($(BUILDTYPE),GCW)
CFLAGS += -DGCW \
	  -I$(PREFIX)/mipsel-gcw0-linux-uclibc/sysroot/usr/include -I$(PREFIX)/mipsel-gcw0-linux-uclibc/sysroot/usr/include/SDL
LDFLAGS += -L$(PREFIX)/mipsel-gcw0-linux-uclibc/sysroot/usr/lib -lSDL_image -lSDL -lEGL -lGLESv1_CM -ldrm -lglapi -lm
endif

all : libglues ken3d

%.o: %.c lab3d.h
	$(CC) $(CFLAGS) -o $@ -c $<
	
libglues:
	cd glues; make BUILD=$(BUILDTYPE);

ken3d: $(OBJS)
	$(CC) -o ken3d $(OBJS) glues/libglues.a $(LDFLAGS)

clean:
	cd glues; make clean
	rm -f *.o
	rm ken3d
