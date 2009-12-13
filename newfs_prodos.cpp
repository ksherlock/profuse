#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <memory>

#include <unistd.h>

#include "BlockDevice.h"
#include "DavexDiskImage.h"
#include "DiskCopy42Image.h"
#include "Entry.h"
#include "Exception.h"
#include "RawDevice.h"
#include "UniversalDiskImage.h"


#define NEWFS_VERSION "0.1"

using namespace ProFUSE;

void usage()
{
    std::printf("newfs_prodos %s\n", NEWFS_VERSION);
    std::printf("\n");
    
    std::printf("newfs_prodos [-v volume_name] [-s size] [-f format] file\n");
    std::printf("\n");
    std::printf("  -v volume_name   specify the volume name.\n"
                "                   Default is Untitled.\n"
                "  -s size          specify size in blocks.\n"
                "                   Default is 1600 blocks (800K)\n"
                "  -f format        specify the disk image format. Valid values are:\n"
                "                   2mg   Universal Disk Image (default)\n"
                "                   dc42  DiskCopy 4.2 Image\n"
                "                   po    ProDOS Order Disk Image\n"
                "                   do    DOS Order Disk Image\n"
                "                   davex Davex Disk Image\n"
    );

}

/*
 * \d+ by block
 * \d+[Kk] by kilobyte
 * \d+[Mm] by megabyte
 */
unsigned parseBlocks(const char *cp)
{
    unsigned long blocks = 0;
    char *mod;
    
    errno = 0; 
    blocks = std::strtoul(cp, &mod, 10);
    if (errno) return -1;
    
    if (mod == cp) return -1;
    if (blocks  > 0xffff) return -1;
    
    if (mod)
    {
        switch(*mod)
        {
        case 0:
            break;
            
        case 'm': // 1m = 1024*1024b = 2048 blocks
        case 'M':
            blocks *= 2048;
            break;
            
        case 'k':  // 1k = 1024b = 2 blocks
        case 'K':
            blocks *= 2;
            break;
        }
        if (blocks > 0xffff) return -1;
    }


    return (unsigned)blocks;
}


// return the basename, without an extension.
std::string filename(const std::string& src)
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


bool ValidName(const char *cp)
{
    
    unsigned i = 0;
    if (!cp || !*cp) return false;
    
    if (!isalpha(*cp)) return false;
    
    for (i = 1; i <17; ++i)
    {
        unsigned char c = cp[i];
        if (c == 0) break;
        
        if (c == '.') continue;
        if (isalnum(c)) continue;
        
        return false;
    }
    
    return i < 16;
}

int main(int argc, char **argv)
{
    unsigned blocks = 1600;
    std::string volumeName;
    std::string fileName;
    int format = 0;
    const char *fname;
    int c;
    
    // ctype uses ascii only.
    ::setlocale(LC_ALL, "C");
    
    while ( (c = ::getopt(argc, argv, "hf:s:v:")) != -1)
    {
        switch(c)
        {
        case '?':
        case 'h':
            usage();
            return 0;
            break;
        
        case 'v':
            volumeName = optarg;
            // make sure it's legal.
            if (!ValidName(optarg))
            {
                std::fprintf(stderr, "Error: `%s' is not a valid ProDOS volume name.\n",  optarg);
                return 0x40;
            }
            break;
            
        case 's':
            blocks = parseBlocks(optarg);
            if (blocks > 0xffff)
            {
                std::fprintf(stderr, "Error: `%s' is not a valid disk size.\n", optarg);
                return 0x5a;
            }
            break;
        
        case 'f':
            {
                format = DiskImage::ImageType(optarg);
                if (format == 0)
                {
                    std::fprintf(stderr, "Error: `%s' is not a supported disk image format.\n", optarg);
                    return -1;
                }
            }
        
        }
    }
    
    argc -= optind;
    argv += optind;
    
    if (argc != 1)
    {
        usage();
        return -1;
    }
    
    fname = argv[0];
    fileName = argv[0];
    
    // generate a filename.
    if (volumeName.empty())
    {
        volumeName = filename(fileName);
        if (volumeName.empty() || !ValidName(volumeName.c_str()))
            volumeName = "Untitled";
    }
    
    if (format == 0) format = DiskImage::ImageType(fname, '2IMG');
    
    
    try
    {
        std::auto_ptr<BlockDevice> device;
        std::auto_ptr<VolumeDirectory> volume;
                
        // todo -- check if path matches /dev/xxx; if so, use RawDevice.
        // todo -- check if file exists at path?
        
        switch(format)
        {
        case 'DC42':
            device.reset(DiskCopy42Image::Create(fname, blocks, volumeName.c_str()));
            break;
            
        case 'PO__':
            device.reset(ProDOSOrderDiskImage::Create(fname, blocks));
            break;
            
        case 'DO__':
            device.reset(DOSOrderDiskImage::Create(fname, blocks));
            break;
        
        case 'DVX_':
            device.reset(DavexDiskImage::Create(fname, blocks, volumeName.c_str()));
            break;
            
        case '2IMG':
        default:
            device.reset(UniversalDiskImage::Create(fname, blocks));
        }
    
        // VolumeDirectory assumes ownership of device,
        // but doesn't release it on exception.
        volume.reset(VolumeDirectory::Create(volumeName.c_str(), device.get()));
        device.release();
    
    }
    catch (Exception e)
    {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return -1;
    }
    return 0;
}
