CC = g++
CPPFLAGS += -Wall -O2 -g -fexceptions \
 -D__FreeBSD__=10 \
 -D_FILE_OFFSET_BITS=64 \
 -D__DARWIN_64_BIT_INO_T=1 \
 -DHAVE_STAT_BIRTHTIME
 
LDFLAGS += -L/usr/local/lib -lfuse_ino64


newfs_pascal: \
 newfs_pascal.o \
 Date.o \
 File.o \
 ../Exception.o \
 ../BlockDevice.o \
 ../BlockCache.o \
 ../DiskCopy42Image.o \
 ../RawDevice.o \
 ../MappedFile.o \
 ../Buffer.o 
 

fileman : \
 FileMan.o \
 Date.o \
 File.o \
 ../Exception.o \
 ../BlockDevice.o \
 ../BlockCache.o \
 ../DiskCopy42Image.o \
 ../RawDevice.o \
 ../MappedFile.o \
 ../Buffer.o  

 
profuse_pascal : \
 profuse_pascal.o \
 profuse_pascal_ops.o \
 Date.o \
 File.o \
 ../Exception.o \
 ../BlockDevice.o \
 ../BlockCache.o \
 ../DiskCopy42Image.o \
 ../RawDevice.o \
 ../MappedFile.o \
 ../Buffer.o  