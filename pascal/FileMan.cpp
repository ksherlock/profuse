/*
 * FileMan utilities.
 *
 * L - list dir
 * E - 
 */


#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <algorithm>
#include <memory>

#include <unistd.h>
#include <strings.h>

#include "File.h"
#include "DateRec.h"
#include "../BlockDevice.h"
#include "../DiskCopy42Image.h"


unsigned parseFormat(const char *type, unsigned defv = 0)
{
    if (type == 0 || *type == 0) return defv;
    
    if (::strcasecmp(type, "2mg") == 0)
        return '2IMG';
    if (::strcasecmp(type, "2img") == 0)
        return '2IMG';
    if (::strcasecmp(type, "dc42") == 0)
        return 'DC42';
    if (::strcasecmp(type, "po") == 0)
        return 'PO__';
    if (::strcasecmp(type, "do") == 0)
        return 'DO__';
    if (::strcasecmp(type, "dsk") == 0)
        return 'DO__';
    if (::strcasecmp(type, "davex") == 0)
        return 'DVX_';
                    
    return defv;
}

// return the filename extension, NULL if none.
const char *extname(const char *src)
{
    if (!src) return NULL;
    unsigned l = std::strlen(src);
    
    for (unsigned i = 0; i < l; ++i)
    {
        char c = src[l - 1 - i];
        if (c == '/') return NULL;
        if (c == '.') return src + l - i;
    }
    
    return NULL;
}


const char *MonthName(unsigned m)
{
    static const char *months[] = {
        "",
        "Jan",
        "Feb",
        "Mar",
        "Apr",
        "May",
        "Jun",
        "Jul",
        "Aug",
        "Sep",
        "Oct",
        "Nov",
        "Dec"
    };

    if (m > 12) return "";
    return months[m];
}

const char *FileType(unsigned ft)
{
    static const char *types[] = {
        "Unknown",
        "Badblocks",
        "Codefile",
        "Textfile",
        "Infofile",
        "Datafile",
        "Graffile",
        "Fotofile",
        "SecureDir"
    
    };
    
    if (ft < 8) return types[ft];
    
    return "";
}

void printUnusedEntry(unsigned block, unsigned size)
{
    std::printf("< UNUSED >      %4u            %4u\n", size, block);
}

void printFileEntry(Pascal::FileEntry *e, bool extended)
{
    Pascal::DateRec dt = e->modification();

    if (extended)
    {
         std::printf("%-15s %4u %2u-%s-%2u %5u %5u  %s\n",
            e->name(), 
            e->blocks(),
            dt.day(),
            MonthName(dt.month()),
            dt.year() % 100,
            e->firstBlock(),
            e->lastByte(),
            FileType(e->fileKind())
        ); 
   
    }
    else
    {
        std::printf("%-15s %4u %2u-%s-%2u\n",
            e->name(), 
            e->blocks(),
            dt.day(),
            MonthName(dt.month()),
            dt.year() % 100
        ); 
    }

}

int list(Pascal::VolumeEntry *volume, bool extended)
{
    unsigned fileCount = volume->fileCount();
    unsigned used = volume->blocks();
    unsigned max = 0;
    unsigned volumeSize = volume->volumeBlocks();
    unsigned lastBlock = volume->lastBlock();
    
    std::fprintf(stdout, "%s:\n", volume->name()); 
    
    for (unsigned i = 0; i < fileCount; ++i)
    {
        Pascal::FileEntry *e = volume->fileAtIndex(i);
        if (!e) continue;
        
        
        if (lastBlock != e->firstBlock())
        {
            unsigned size = e->firstBlock() - lastBlock;
            max = std::max(max, size);
        
            if (extended)
            {
                printUnusedEntry(lastBlock, size);
            }
        }
        
        printFileEntry(e, extended);
        
        lastBlock = e->lastBlock();
        used += e->blocks();
    }

    if (extended && (lastBlock != volumeSize))
    {
        unsigned size = volumeSize - lastBlock;
        max = std::max(max, size);    
        printUnusedEntry(lastBlock, size);
    }    
    
    
    std::fprintf(stdout, 
        "%u/%u files<listed/in-dir>, "
        "%u blocks used, "
        "%u unused, "
        "%u in largest\n",
        fileCount, fileCount,
        used,
        volumeSize - used,
        max
    );
 
    return 0;
}


void usage()
{
    std::printf(
        "Pascal File Manager v 0.0\n\n"
        "Usage: fileman [-h] [-f format] action diskimage\n"
        "Options:\n"
        "  -h            Show usage information.\n"
        "  -f format     Specify disk format.  Valid values are:\n"
        "                 po: ProDOS order disk image\n"
        "                 do: DOS Order disk image\n"
        "\n"
        "Actions:\n"
        "  L            List files\n"
        "  E            List files (extended)\n"  
    );

}

int main(int argc, char **argv)
{
    std::auto_ptr<Pascal::VolumeEntry> volume;
    std::auto_ptr<ProFUSE::BlockDevice> device;
  
    const char *format = NULL;
    unsigned fmt = 0;
    
    int c;
    
    
    
    while ((c = ::getopt(argc, argv, "f:h")) != -1)
    {
        std::printf("%c\n", c);
        switch(c)
        {
        case 'f':
            format = optarg;
            break;
            
        case 'h':
        case '?':
            usage();
            std::exit(0);
        }
    }
    

    argc -= optind;
    argv += optind;
    
    if (argc != 2)
    {
        usage();
        std::exit(1);
    }
    
    
    const char *file = argv[1];
    const char *action = argv[0];
    
    if (format == NULL) format = extname(file);
    
    fmt = parseFormat(format, 'PO__');
    
    
    try {
    
        switch(fmt)
        {
        case 'DO__':
            device.reset( new ProFUSE::DOSOrderDiskImage(file, true) );
            break;
        case 'PO__':
            device.reset( new ProFUSE::ProDOSOrderDiskImage(file, true) );
            break;
        case 'DC42':
            device.reset( new ProFUSE::DiskCopy42Image(file, true) );
            break;
            
        default:
            std::fprintf(stderr, "Unable to determine format.  Please use -f flag.\n");
            exit(2);
        }
    
        
        volume.reset( new Pascal::VolumeEntry(device.get()));
        
        device.release();
    
        if (!::strcasecmp("E", action)) return list(volume.get(), true);
        if (!::strcasecmp("L", action)) return list(volume.get(), false);
        
    }
    catch (ProFUSE::Exception& e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        std::fprintf(stderr, "%s\n", strerror(e.error()));
    }
    

}