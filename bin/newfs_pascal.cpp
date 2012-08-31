
#include <memory>
#include <new>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdlib>

#include <unistd.h>
#include <sys/stat.h>


#include <Device/BlockDevice.h>
#include <Device/RawDevice.h>


#include <Common/Exception.h>

#include <Pascal/Pascal.h>

#include <File/File.h>
#include <File/MappedFile.h>





using namespace Pascal;
using namespace Device;

#define NEWFS_VERSION "0.1"



bool yes_or_no()
{
	int ch, first;
	(void)fflush(stderr);
    
	first = ch = getchar();
	while (ch != '\n' && ch != EOF)
		ch = getchar();
	return (first == 'y' || first == 'Y');
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


void usage()
{
    std::printf("newfs_pascal %s\n", NEWFS_VERSION);
    std::printf("\n");


    std::printf("newfs_pascal [-v volume_name] [-s size] [-f format] file\n");
    std::printf("\n");
    std::printf("  -v volume_name   Specify the volume name.\n"
                "                   Default is Untitled.\n"
                "  -s size          Specify size in blocks.\n"
                "                   Default is 1600 blocks (800K)\n"
                "  -b bootfile      Specify a file that contains the boot block\n"
                "  -f format        Specify the disk image format. Valid values are:\n"
                "                   2img  Universal Disk Image\n"
                "                   dc42  DiskCopy 4.2 Image\n"
                "                   davex Davex Disk Image\n"
                "                   do    DOS Order Disk Image\n"
                "                   po    ProDOS Order Disk Image (default)\n"
    );


}

int main(int argc, char **argv)
{
    unsigned blocks = 1600;
    std::string volumeName;
    std::string fileName;
    std::string bootFile;
    
    int format = 0;
    const char *fname;
    int c;
    

    while ( (c = ::getopt(argc, argv, "hf:s:v:")) != -1)
    {
        switch(c)
        {
            case 'h':
                default:
                usage();
                return c == 'h' ? 0 :  1;
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
                format = Device::BlockDevice::ImageType(optarg);
                if (format == 0)
                {
                    std::fprintf(stderr, "Error: `%s' is not a supported disk image format.\n", optarg);
                    return -1;
                }
                break;
                
            case 'b':
                bootFile = optarg;
                break;
        
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
    

    


    try
    {
        
        struct stat st;
        bool rawDevice = false;
        
        BlockDevicePointer device;
        VolumeEntryPointer volume;
        
        // Check for block device.  if so, verify.
        // if file exists, verify before overwrite.
        std::memset(&st, 0, sizeof(st));
        
        if (::stat(fname, &st) == 0)
        {
            if (S_ISBLK(st.st_mode))
            {
                fprintf(stderr, "`%s' is a raw device. Are you sure you want to initialize it? ", fname);
                if (!yes_or_no()) return -1;
                
                device = RawDevice::Open(fname, File::ReadWrite);
                blocks = device->blocks();
                rawDevice = true;
                
                if (blocks > 0xffff)
                {
                    std::fprintf(stderr, "Error: device is too large.\n");
                    return 0x5a;                    
                }
            }
            
            else
            {          
                
                // file exists, verify we want to destroy it.
                
                fprintf(stderr, "`%s' already exists.  Are you sure you want to overwrite it? ", fname);
                if (!yes_or_no()) return -1;
            }
            
        }
        
        // generate a filename.
        if (volumeName.empty())
        {
            if (!rawDevice)
                volumeName = filename(fileName);
            if (volumeName.empty() || !VolumeEntry::ValidName(volumeName.c_str()))
                volumeName = "PASCAL";
        }              
        
        if (!rawDevice)
            device = BlockDevice::Create(fname, volumeName.c_str(), blocks, format);
        
        if (!device.get())
        {
            std::fprintf(stderr, "Error: Unsupported diskimage format.\n");
            return -1;
        }
        
        
        if (!bootFile.empty())
        {
            MappedFile bf(bootFile.c_str(), File::ReadOnly, std::nothrow);
            
            if (!bf.isValid())
            {
                std::fprintf(stderr, "Warning: unable to open boot file `%s'.\n", bootFile.c_str());
            }
            else
            {
                size_t length = bf.length();
                // either 1 or 2 blocks.
                if (length == 512)
                {
                    device->write(0, bf.address());
                }
                else if (length == 1024)
                {
                    device->write(0, bf.address());
                    device->write(1, (uint8_t*)bf.address() + 512);
                }
                else
                {
                    std::fprintf(stderr, "Warning: boot file must be 512 or 1024 bytes.\n");
                }
            }
        }
        
        
        volume = VolumeEntry::Create(device, volumeName.c_str());

        
    }
    catch (::Exception& e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        std::fprintf(stderr, "%s\n", ::strerror(e.error()));
        return -2;
    }

    
    return 0;
}
