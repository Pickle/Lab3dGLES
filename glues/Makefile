
ifeq ($(BUILD),PANDORA)
PREFIX	= $(PNDSDK)
TOOLS	= bin
TARGET	= arm-none-linux-gnueabi
else ifeq ($(BUILD),WIZ)
PREFIX	= $(WIZSDK)
TOOLS	= bin
TARGET	= arm-openwiz-linux-gnu
else ifeq ($(BUILD),CAANOO)
PREFIX	= $(CAANOOSDK)
TOOLS	= tools/gcc-4.2.4-glibc-2.7-eabi/bin
TARGET	= arm-gph-linux-gnueabi
else ifeq ($(BUILD),GCW)
PREFIX	= $(GCWSDK)
TOOLS	= bin
TARGET	= mipsel-gcw0-linux-uclibc
endif

CC = $(PREFIX)/$(TOOLS)/$(TARGET)-gcc
CPP = $(PREFIX)/$(TOOLS)/$(TARGET)-g++
LD = $(PREFIX)/$(TOOLS)/$(TARGET)-gcc
AS = $(PREFIX)/$(TOOLS)/$(TARGET)-as
AR = $(PREFIX)/$(TOOLS)/$(TARGET)-ar

CFLAGS = -O3 -I. -I$(PREFIX)/usr/include
 
OBJS = \
	glues_error.o \
	glues_mipmap.o \
	glues_project.o \
	glues_quad.o \
	glues_registry.o \
	libtess/geom.o \
	libtess/memalloc.o \
	libtess/mesh.o \
	libtess/normal.o \
	libtess/priorityq.o \
	libtess/render.o \
	libtess/sweep.o \
	libtess/tess.o \
	libtess/tessmono.o \
	libtess/dict.o

all:    $(OBJS)
	$(AR) rcs libglues.a $(OBJS)

clean:
	rm -f *.o *~ libtess/*.o libglues.a

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<	
