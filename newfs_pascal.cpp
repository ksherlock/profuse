#include "../BlockDevice.h"
#include "../Exception.h"
#include "../MappedFile.h"
#include "../DiskCopy42Image.h"

#include "File.h"


#include <memory>
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <unistd.h>


using namespace Pascal;

#define NEWFS_VERSION "0.1"



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


void usage()
{
    std::printf("newfs_pascal %s\n", NEWFS_VERSION);
    std::printf("\n");


    std::printf("newfs_pascal [-v volume_name] [-s size] [-f format] file\n");
    std::printf("\n");
    std::printf("  -v volume_name   specify the volume name.\n"
                "                   Default is Untitled.\n"
                "  -s size          specify size in blocks.\n"
                "                   Default is 1600 blocks (800K)\n"
                "  -f format        specify the disk image format. Valid values are:\n"
                "                   dc42  DiskCopy 4.2 Image\n"
                "                   do    DOS Order Disk Image\n"
                "                   po    ProDOS Order Disk Image (default)\n"
    );


}

int main(int argc, char **argv)
{
    unsigned blocks = 1600;
    std::string volumeName;
    std::string fileName;
    int format = 0;
    const char *fname;
    int c;
    

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
            if (!VolumeEntry::ValidName(optarg))
            {
                std::fprintf(stderr, "Error: `%s' is not a valid Pascal volume name.\n",  optarg);
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
                format = ProFUSE::DiskImage::ImageType(optarg);
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
        if (volumeName.empty() || !VolumeEntry::ValidName(volumeName.c_str()))
            volumeName = "PASCAL";
    }
    
    if (format == 0) format = ProFUSE::DiskImage::ImageType(fname, 'PO__');


    try
    {
        std::auto_ptr<ProFUSE::BlockDevice> device;
        std::auto_ptr<Pascal::VolumeEntry> volume;
        
        // TODO -- check for raw device.
        
        
        switch(format)
        {
        case 'DC42':
            device.reset(ProFUSE::DiskCopy42Image::Create(fname, blocks, volumeName.c_str()));
            break;
            
        case 'PO__':
            device.reset(ProFUSE::ProDOSOrderDiskImage::Create(fname, blocks));
            break;
            
        case 'DO__':
            device.reset(ProFUSE::DOSOrderDiskImage::Create(fname, blocks));
            break;
        
            
        default:
            std::fprintf(stderr, "Error: Unsupported diskimage format.\n");
            return -1;
        }
        
        
        volume.reset(
            new Pascal::VolumeEntry(volumeName.c_str(), device.get())
        );
        device.release();

        /*
        ProFUSE::MappedFile bootBlock("pascal.bootblock", true);
        
        if (bootBlock.fileSize() == 1024)
        {
            uint8_t buffer[512];
            bootBlock.setBlocks(2);
            
            for (unsigned block = 0; block < 2; ++block)
            {
                bootBlock.readBlock(block, buffer);
                volume->writeBlock(block, buffer);
            }         
        }
        */
        
    }
    catch (ProFUSE::POSIXException& e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        std::fprintf(stderr, "%s\n", ::strerror(e.error()));
        return -2;
    }
    catch (ProFUSE::Exception& e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        return -2;
    }
    
    return 0;
}