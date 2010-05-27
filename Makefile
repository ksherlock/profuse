CC = g++
CPPFLAGS += -Wall -W -I. -O2 -g 
LDFLAGS += -lpthread
fuse_pascal_LDFLAGS += -lfuse

xattr: xattr.o
	$(CC) $^ -o $@

newfs_pascal: newfs_pascal.o \
  Cache/BlockCache.o Cache/ConcreteBlockCache.o Cache/MappedBlockCache.o \
  Device/Adaptor.o Device/BlockDevice.o Device/DavexDiskImage.o \
  Device/DiskCopy42Image.o Device/DiskImage.o Device/RawDevice.o \
  Device/UniversalDiskImage.o \
  Endian/Endian.o \
  File/File.o File/MappedFile.o \
  ProFUSE/Exception.o ProFUSE/Lock.o \
  Pascal/Date.o Pascal/Entry.o Pascal/FileEntry.o Pascal/VolumeEntry.o 

apfm: apfm.o \
  Cache/BlockCache.o Cache/ConcreteBlockCache.o Cache/MappedBlockCache.o \
  Device/Adaptor.o Device/BlockDevice.o Device/DavexDiskImage.o \
  Device/DiskCopy42Image.o Device/DiskImage.o Device/RawDevice.o \
  Device/UniversalDiskImage.o \
  Endian/Endian.o \
  File/File.o File/MappedFile.o \
  ProFUSE/Exception.o ProFUSE/Lock.o \
  Pascal/Date.o Pascal/Entry.o Pascal/FileEntry.o Pascal/VolumeEntry.o 


fuse_pascal: fuse_pascal.o fuse_pascal_ops.o \
  Cache/BlockCache.o Cache/ConcreteBlockCache.o Cache/MappedBlockCache.o \
  Device/Adaptor.o Device/BlockDevice.o Device/DavexDiskImage.o \
  Device/DiskCopy42Image.o Device/DiskImage.o Device/RawDevice.o \
  Device/UniversalDiskImage.o \
  Endian/Endian.o \
  File/File.o File/MappedFile.o \
  ProFUSE/Exception.o ProFUSE/Lock.o \
  Pascal/Date.o Pascal/Entry.o Pascal/FileEntry.o Pascal/VolumeEntry.o 
	$(CC) -lfuse $(LDFLAGS) $^ -o $@


xattr.o: xattr.cpp
 
newfs_pascal.o: newfs_pascal.cpp Device/BlockDevice.h ProFUSE/Exception.h \
  Device/TrackSector.h Cache/BlockCache.h Device/RawDevice.h File/File.h \
  Pascal/File.h Pascal/Date.h

fuse_pascal.o: fuse_pascal.cpp Pascal/File.h Pascal/Date.h \
  ProFUSE/Exception.h Device/BlockDevice.h Device/TrackSector.h \
  Cache/BlockCache.h

fuse_pascal_ops.o: fuse_pascal_ops.cpp Pascal/File.h Pascal/Date.h \
  ProFUSE/auto.h ProFUSE/Exception.h

apfm.o: apfm.cpp Pascal/File.h Pascal/Date.h Device/BlockDevice.h \
  ProFUSE/Exception.h Device/TrackSector.h Cache/BlockCache.h

File/File.o: File/File.cpp File/File.h ProFUSE/Exception.h

File/MappedFile.o: File/MappedFile.cpp File/MappedFile.h File/File.h \
  ProFUSE/Exception.h

Device/Adaptor.o: Device/Adaptor.cpp Device/Adaptor.h Device/TrackSector.h \
  ProFUSE/Exception.h

Device/BlockDevice.o: Device/BlockDevice.cpp Device/BlockDevice.h \
  ProFUSE/Exception.h Device/TrackSector.h Cache/BlockCache.h \
  Cache/ConcreteBlockCache.h Device/DiskImage.h Device/Adaptor.h \
  File/MappedFile.h File/File.h Device/UniversalDiskImage.h \
  Device/DiskCopy42Image.h Device/DavexDiskImage.h Device/RawDevice.h

Device/DavexDiskImage.o: Device/DavexDiskImage.cpp \
  Device/DavexDiskImage.h \
  Device/BlockDevice.h ProFUSE/Exception.h Device/TrackSector.h \
  Cache/BlockCache.h Device/DiskImage.h Device/Adaptor.h \
  File/MappedFile.h File/File.h Endian/Endian.h Endian/IOBuffer.h \
  Endian/IOBuffer.cpp.h Cache/MappedBlockCache.h

Device/DiskCopy42Image.o: Device/DiskCopy42Image.cpp \
  Device/DiskCopy42Image.h \
  Device/BlockDevice.h ProFUSE/Exception.h Device/TrackSector.h \
  Cache/BlockCache.h Device/DiskImage.h Device/Adaptor.h \
  File/MappedFile.h File/File.h Endian/Endian.h Endian/IOBuffer.h \
  Endian/IOBuffer.cpp.h Cache/MappedBlockCache.h

Device/DiskImage.o: Device/DiskImage.cpp Device/DiskImage.h \
  ProFUSE/Exception.h \
  Device/BlockDevice.h Device/TrackSector.h Cache/BlockCache.h \
  Device/Adaptor.h File/MappedFile.h File/File.h Cache/MappedBlockCache.h

Device/RawDevice.o: Device/RawDevice.cpp Device/RawDevice.h \
  Device/BlockDevice.h \
  ProFUSE/Exception.h Device/TrackSector.h Cache/BlockCache.h File/File.h

Device/UniversalDiskImage.o: Device/UniversalDiskImage.cpp \
  Device/UniversalDiskImage.h Device/BlockDevice.h ProFUSE/Exception.h \
  Device/TrackSector.h Cache/BlockCache.h Device/DiskImage.h \
  Device/Adaptor.h File/MappedFile.h File/File.h Endian/Endian.h \
  Endian/IOBuffer.h Endian/IOBuffer.cpp.h Cache/MappedBlockCache.h \
  Cache/ConcreteBlockCache.h

Endian/Endian.o: Endian/Endian.cpp Endian/Endian.h

Cache/BlockCache.o: Cache/BlockCache.cpp Cache/BlockCache.h \
  Device/BlockDevice.h ProFUSE/Exception.h Device/TrackSector.h \
  ProFUSE/auto.h

Cache/ConcreteBlockCache.o: Cache/ConcreteBlockCache.cpp \
  Device/BlockDevice.h \
  ProFUSE/Exception.h Device/TrackSector.h Cache/BlockCache.h \
  Cache/ConcreteBlockCache.h ProFUSE/auto.h

Cache/MappedBlockCache.o: Cache/MappedBlockCache.cpp \
  Cache/MappedBlockCache.h \
  Cache/BlockCache.h Device/BlockDevice.h ProFUSE/Exception.h \
  Device/TrackSector.h

ProFUSE/Exception.o: ProFUSE/Exception.cpp ProFUSE/Exception.h

ProFUSE/Lock.o: ProFUSE/Lock.cpp ProFUSE/Lock.h


Pascal/Date.o: Pascal/Date.cpp Pascal/Date.h

Pascal/Entry.o: Pascal/Entry.cpp Pascal/Entry.h Pascal/Date.h \
  ProFUSE/Exception.h Endian/Endian.h Endian/IOBuffer.h \
  Endian/IOBuffer.cpp.h Device/BlockDevice.h Device/TrackSector.h \
  Cache/BlockCache.h

Pascal/FileEntry.o: Pascal/FileEntry.cpp Pascal/Pascal.h Pascal/Date.h \
  Pascal/Entry.h Pascal/FileEntry.h Pascal/VolumeEntry.h ProFUSE/auto.h \
  ProFUSE/Exception.h Endian/Endian.h Endian/IOBuffer.h \
  Endian/IOBuffer.cpp.h Device/BlockDevice.h Device/TrackSector.h \
  Cache/BlockCache.h

Pascal/VolumeEntry.o: Pascal/VolumeEntry.cpp Pascal/Pascal.h Pascal/Date.h \
  Pascal/Entry.h Pascal/FileEntry.h Pascal/VolumeEntry.h ProFUSE/auto.h \
  ProFUSE/Exception.h Endian/Endian.h Endian/IOBuffer.h \
  Endian/IOBuffer.cpp.h Device/BlockDevice.h Device/TrackSector.h \
  Cache/BlockCache.h

