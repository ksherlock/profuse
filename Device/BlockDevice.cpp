
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>


#include <Device/BlockDevice.h>
#include <Cache/ConcreteBlockCache.h>

#include <ProFUSE/Exception.h>

#include <Device/DiskImage.h>
#include <Device/UniversalDiskImage.h>
#include <Device/DiskCopy42Image.h>
#include <Device/DavexDiskImage.h>
#include <Device/RawDevice.h>

#ifdef HAVE_NUFX
#include <Device/SDKImage.h>
#endif

using namespace Device;

using ProFUSE::Exception;
using ProFUSE::POSIXException;


unsigned BlockDevice::ImageType(MappedFile *f, unsigned defv)
{
#undef __METHOD__
#define __METHOD__ "BlockDevice::ImageType"
    
    
    if (UniversalDiskImage::Validate(f, std::nothrow))
        return '2IMG';
    
    if (DiskCopy42Image::Validate(f, std::nothrow))
        return 'DC42';
   
#ifdef HAVE_NUFX 
    if (SDKImage::Validate(f, std::nothrow))
        return 'SDK_';
#endif
    
    if (ProDOSOrderDiskImage::Validate(f, std::nothrow))
        return 'PO__';
    
    if (DOSOrderDiskImage::Validate(f, std::nothrow))
        return 'DO__';    
    
    return defv;
    
}

unsigned BlockDevice::ImageType(const char *type, unsigned defv)
{
    const char *tmp;
    
    
    if (type == 0 || *type == 0) return defv;
    
    // type could be a path, eg images/file, disk.images/file
    
    // unix-specifix.
    // basename alters the input string
    tmp = std::strrchr(type, '/');
    if (tmp) type = tmp + 1;
    
    // type could be a filename, in which case we check the extension.
    tmp = std::strrchr(type, '.');
    if (tmp) type = tmp + 1;
    if (*type == 0) return defv;
    
    
    if (::strcasecmp(type, "2mg") == 0)
        return '2IMG';
    if (::strcasecmp(type, "2img") == 0)
        return '2IMG';

    if (::strcasecmp(type, "dc") == 0)
        return 'DC42';
    if (::strcasecmp(type, "dc42") == 0)
        return 'DC42';
    
    if (::strcasecmp(type, "po") == 0)
        return 'PO__';
    if (::strcasecmp(type, "dmg") == 0)
        return 'PO__';
    
    if (::strcasecmp(type, "dsk") == 0)
        return 'DO__';
    if (::strcasecmp(type, "do") == 0)
        return 'DO__';
    
    if (::strcasecmp(type, "dvx") == 0)
        return 'DVX_';        
    if (::strcasecmp(type, "davex") == 0)
        return 'DVX_';
    
   
#ifdef HAVE_NUFX 
    if (::strcasecmp(type, "sdk") == 0)
        return 'SDK_';
    if (::strcasecmp(type, "shk") == 0)
        return 'SDK_';    
#endif 

    return defv;
}

BlockDevicePointer BlockDevice::Open(const char *name, File::FileFlags flags, unsigned imageType)
{
#undef __METHOD__
#define __METHOD__ "BlockDevice::Open"
    
    struct stat st;
    
    std::memset(&st, 0, sizeof(st));
    
    if (::stat(name, &st) != 0)
    {
        throw POSIXException(__METHOD__ ": stat error", errno);       
    }

    // /dev/xxx ignore the type.
    if (S_ISBLK(st.st_mode))
        return RawDevice::Open(name, flags);
    
    MappedFile file(name, flags);

    
    if (!imageType)
    {
        imageType = ImageType(&file, 'PO__');
    }
    

    switch (imageType)
    {
        case '2IMG':
            return UniversalDiskImage::Open(&file);
            
        case 'DC42':
            return DiskCopy42Image::Open(&file);
            
        case 'DO__':
            return DOSOrderDiskImage::Open(&file);
            
        case 'PO__':
            return ProDOSOrderDiskImage::Open(&file);
            
        case 'DVX_':
            return DavexDiskImage::Open(&file);
           
#if HAVE_NUFX 
        case 'SDK_':
            return SDKImage::Open(name);
#endif
                
                    
    }
    
    // throw an error?
    return BlockDevicePointer();
    
}


// return the basename, without an extension.
static std::string filename(const std::string& src)
{
    unsigned start;
    unsigned end;
    
    
    if (src.empty()) return std::string("");
    
    start = end = 0;
    
    for(unsigned i = 0, l = src.length(); i < l; ++i)
    {
        char c = src[i];
        if (c == '/') start = end = i + 1;
        if (c == '.') end = i;
    }
    
    if (start == src.length()) return std::string("");
    
    if (start == end) return src.substr(start);
    return src.substr(start, end - start); 
}


BlockDevicePointer BlockDevice::Create(const char *fname, const char *vname, unsigned blocks, unsigned imageType)
{
    std::string xname;
    
    if (!imageType) imageType = ImageType(fname, 'PO__');
    
    if (vname == NULL)
    {
        xname = filename(std::string(fname));
        vname = xname.c_str();
    }
    

    
    
    switch(imageType)
    {
        case '2IMG':
            return UniversalDiskImage::Create(fname, blocks);
            
        case 'DC42':
            return DiskCopy42Image::Create(fname, blocks, vname);
            
        case 'DO__':
            return DOSOrderDiskImage::Create(fname, blocks);
            
        case 'PO__':
            return ProDOSOrderDiskImage::Create(fname, blocks);
            
        case 'DVX_':
            return DavexDiskImage::Create(fname, blocks, vname);
    }
    
    return BlockDevicePointer();
    
}





BlockDevice::BlockDevice()
{
}
BlockDevice::~BlockDevice()
{
}

void BlockDevice::zeroBlock(unsigned block)
{
    uint8_t bp[512];
    std::memset(bp, 0, 512);
    
    write(block, bp);
}



bool BlockDevice::mapped()
{
    return false;
}

void BlockDevice::sync(unsigned block)
{
    sync();
}

/*
void BlockDevice::sync(TrackSector ts)
{
    sync();
}
*/

BlockCachePointer BlockDevice::createBlockCache()
{
    unsigned b = blocks();
    unsigned size = std::max(16u, b / 16);
    return ConcreteBlockCache::Create(shared_from_this(), size);
}
