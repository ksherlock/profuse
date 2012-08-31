#include "BlockDevice.h"
#include "Exception.h"
#include "DavexDiskImage.h"
#include "UniversalDiskImage.h"
#include "DiskCopy42Image.h"

#include <cstdio>

int main(int argc, char **argv)
{
  if (argc == 2)
  {
    try {
      //ProFUSE::BlockDevice *dev = ProFUSE::ProDOSOrderDiskImage::Create(argv[1], 1600);
      //ProFUSE::BlockDevice *dev = ProFUSE::DavexDiskImage::Create(argv[1], 1600);
      //ProFUSE::BlockDevice *dev = ProFUSE::UniversalDiskImage::Create(argv[1], 1600);
      ProFUSE::BlockDevice *dev = ProFUSE::DiskCopy42Image::Create(argv[1], 1600);
      
      delete dev;
    } catch (ProFUSE::Exception e) {
      if (e.error()) printf("%s %s", e.what(), strerror(e.error()));
      else printf("%s\n", e.what());
    }

  }
  return 0;
}
