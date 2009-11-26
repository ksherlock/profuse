CC = g++
CPPFLAGS += -Wall -O2

newfs_prodos: \
 newfs_prodos.o \
 Exception.o \
 BlockDevice.o \
 UniversalDiskImage.o \
 DiskCopy42Image.o \
 DavexDiskImage.o \
 RawDevice.o \
 MappedFile.o \
 Buffer.o \
 Entry.o \
 Directory.o \
 VolumeDirectory.o \
 Bitmap.o \
 DateTime.o 
 
 
 