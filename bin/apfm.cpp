/*
 * FileMan utilities.
 *
 * L - list dir
 * E - 
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cerrno>

#include <algorithm>
#include <memory>

#include <unistd.h>
#include <strings.h>
#include <sys/stat.h>

#include <Pascal/Pascal.h>
#include <Pascal/Date.h>
#include <Pascal/TextWriter.h>

#include <Device/Device.h>
#include <Device/BlockDevice.h>

#include <File/File.h>
#include <File/MappedFile.h>


enum commands {
    kCommandLS = 1,
    kCommandCAT,
    kCommandCP,
    kCommandMV,
    kCommandRM,
    kCommandGET,
    kCommandPUT,
    kCommandKRUNCH
};


void usage()
{
    std::fputs(
        "Pascal File Manager v 0.0\n\n"
        "Usage: apfm [-h] [-f format] diskimage action ...\n"
        "Options:\n"
        "  -h            Show usage information.\n"
        "  -f format     Specify disk format.  Valid values are:\n"
        "                    dc42  DiskCopy 4.2 Image\n"
        "                    davex Davex Disk Image\n"
        "                    2img  Universal Disk Image\n"
#ifdef HAVE_NUFX
        "                    sdk   ShrinkIt Disk Image\n"
#endif
        "                    do    DOS Order Disk Image\n"
        "                    po    ProDOS Order Disk Image (default)\n"  
        "\n"
        "Actions:\n"
        "  cat\n"
        "  krunch\n"
        "  ls\n"
        "  cp\n"
        "  mv\n"
        "  rm\n"
        "  get\n"
        "  put\n",
        stdout
        );
    
}

void commandUsage(unsigned command)
{
    const char *text = "";
    
    switch(command)
    {
        case kCommandLS:
            text =
            "List the volume contents.\n\n"
            "apfm file ls [-l]\n"
            "Options:\n"
            "  -l            Extended listing\n"
            ;
            break;
            
            
        case kCommandKRUNCH:
            text =
            "Move free blocks to the end of the volume.\n\n" 
            "apfm krunch [-fi]\n"
            "Options:\n"
            "  -f            Force\n"
            "  -i            Interactive\n"
            ;
            break;
            
        case kCommandCAT:
            text =
            "Print the contents of a file to stdout.\n\n"
            "apfm cat file ...\n"
            ;
            break;

        case kCommandCP:
            text =
            "Copy a file.\n\n"
            "apfm cp [-fi] source target\n"
            "Options:\n"
            "  -f            Force\n"
            "  -i            Interactive\n"
            ;
            break;            
            
        case kCommandMV:
            text =
            "Rename a file.\n\n"
            "apfm mv [-fi] source target\n"
            "Options:\n"
            "  -f            Force\n"
            "  -i            Interactive\n"
            ;
            break;
            
        case kCommandGET:
            text =
            "Copy a file from the Pascal volume to the native file system.\n\n"
            "apfm get [-fi] source [target]\n"
            "Options:\n"
            "  -f            Force\n"
            "  -i            Interactive\n"
            ;
            break;
            
        case kCommandPUT:
            text =
            "Copy a file from the native file system to the Pascal volume.\n\n"
            "apfm put [-fi] source [target]\n"
            "Options:\n"
            "  -f            Force\n"
            "  -i            Interactive\n"
            "  -t type       Set the file type.  Valid values:\n"
            "                  code\n"
            "                  data\n"
            "                  foto\n"
            "                  graf\n"
            "                  info\n"
            "                  text\n"
            ;
            break;
    }
    
    std::fputs(text, stdout);
}


unsigned command(const char *command)
{
    if (!::strcasecmp(command, "ls")) return kCommandLS;
    if (!::strcasecmp(command, "list")) return kCommandLS;
    
    if (!::strcasecmp(command, "cat")) return kCommandCAT;
    
    if (!::strcasecmp(command, "get")) return kCommandGET;

    if (!::strcasecmp(command, "put")) return kCommandPUT;

    if (!::strcasecmp(command, "cp")) return kCommandCP;
    if (!::strcasecmp(command, "copy")) return kCommandCP;

    if (!::strcasecmp(command, "rm")) return kCommandRM;
    if (!::strcasecmp(command, "remove")) return kCommandRM;
    if (!::strcasecmp(command, "del")) return kCommandRM;
    if (!::strcasecmp(command, "delete")) return kCommandRM;

    if (!::strcasecmp(command, "mv")) return kCommandMV;
    if (!::strcasecmp(command, "move")) return kCommandMV;
    if (!::strcasecmp(command, "rename")) return kCommandMV;

    if (!::strcasecmp(command, "krunch")) return kCommandKRUNCH;    

    
    return -1;
}

File::FileFlags commandFlags(unsigned command)
{
    switch (command)
    {
        case kCommandPUT:
        case kCommandKRUNCH:
        case kCommandRM:
        case kCommandMV:
        case kCommandCP:
            return File::ReadWrite;
        default:
            return File::ReadOnly;
    }
}

// from BSD rm, prompt question on stderr.

bool yes_or_no(const char *format, ...)
{
	int ch, first;
    va_list va;
    
    va_start(va, format);
    std::vfprintf(stderr, format, va);
    va_end(va);
    
    std::fputc(' ', stderr);
    std::fflush(stderr);
    
	first = ch = getchar();
	while (ch != '\n' && ch != EOF)
		ch = getchar();
	return (first == 'y' || first == 'Y');
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

void printFileEntry(Pascal::FileEntryPointer e, bool extended)
{
    Pascal::Date dt = e->modification();

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

int action_ls(int argc, char **argv, Pascal::VolumeEntry *volume)
{    
    bool extended = false;
    unsigned fileCount = volume->fileCount();
    unsigned used = volume->blocks();
    unsigned max = 0;
    unsigned volumeSize = volume->volumeBlocks();
    unsigned lastBlock = volume->lastBlock();
    int c;
    
    std::fprintf(stdout, "%s:\n", volume->name()); 
    
    //argv[0] = "afpm ls";
    
    while ((c = ::getopt(argc, argv, "lh")) != -1)
    {
        switch(c)
        {
            case 'l':
                extended = true;
                break;
            case 'h':
            default:
                commandUsage(kCommandLS);
                return c == 'h' ? 0 : 1;
                break;
        }
    }
    
    argc -= optind;
    argv += optind;
    // only print listed files?
    
    for (unsigned i = 0; i < fileCount; ++i)
    {
        Pascal::FileEntryPointer e = volume->fileAtIndex(i);
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

    if (lastBlock != volumeSize)
    {
        unsigned size = volumeSize - lastBlock;
        max = std::max(max, size);    
        if (extended)
            printUnusedEntry(lastBlock, size);
    }    
    
    
    std::fprintf(stdout, 
        "%u/%u files <listed/in-dir>, "
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


int action_cat(unsigned argc, char **argv, Pascal::VolumeEntry *volume)
{
    // cat file1, file2...
    //argv[0] = "afpm cat";
    
    int c;
    
    while ((c = ::getopt(argc, argv, "h")) != -1)
    {
        switch(c)
        {
            case 'h':
            default:
                commandUsage(kCommandCAT);
                return c == 'h' ? 0 : 1;
                break;
        }
    }
    
    
    argc -= optind;
    argv += optind;    
    
    if (argc < 1)
    {
        commandUsage(kCommandCAT);
        return -1;
    }

    for (unsigned i = 0; i < argc; ++i)
    {
        const char *fname = argv[i];
        unsigned fileSize;
        unsigned offset;
        uint8_t buffer[512];
        Pascal::FileEntryPointer e;
        // find it...

        e = volume->fileByName(argv[i]);
        
        if (!e)
        {
            std::fprintf(stderr, "apfm cat: %s: no such file.\n", fname);
            continue;
        }
    
        fileSize = e->fileSize();
        offset = 0;
        while (offset < fileSize)
        {
            unsigned count = std::min(512u, fileSize - offset);
            e->read(buffer, count, offset);
            
            std::fwrite(buffer, count, 1, stdout);
            offset += count; 
        }
    }
    
    return 0;
}



int action_mv(int argc, char **argv, Pascal::VolumeEntry *volume)
{
    // mv src dest
    bool iFlag = ::isatty(STDIN_FILENO);
    int c;
    
    while ((c = ::getopt(argc, argv, "fih")) != -1)
    {
        switch(c)
        {
            case 'f':
                iFlag = false;
                break;
            case 'i':
                iFlag = true;
                break;
                
            case 'h':
            default:
                commandUsage(kCommandMV);
                return c == 'h' ? 0 : 1;
                break;
        }
    }
    
    argc -= optind;
    argv += optind;
    
    if (argc != 2)
    {
        commandUsage(kCommandMV);
        return -1;
    }
    
    const char *source = argv[0];
    const char *dest = argv[1];
    
    if (!volume->fileByName(source))
    {
        std::fprintf(stderr, "apfm mv: %s: no such file.\n", source);
        return -1;
    }
    
    // if -i and destination file exists, confirm overwritting it.
    if (iFlag && volume->fileByName(dest))
    {
        bool ok = yes_or_no("Overwrite %s?", dest);
        if (!ok)
        {
            std::fprintf(stderr, "Not overwritten.\n");
            return -1;
        }        
    }
    
    volume->rename(source, dest);
    
    return 0;
}

int action_cp(int argc, char **argv, Pascal::VolumeEntry *volume)
{
    // cp src dest
    
    bool iFlag = ::isatty(STDIN_FILENO);
    int c;
    
    while ((c = ::getopt(argc, argv, "fih")) != -1)
    {
        switch(c)
        {
            case 'f':
                iFlag = false;
                break;
            case 'i':
                iFlag = true;
                break;
                
            case 'h':
            default:
                commandUsage(kCommandCP);
                return c == 'h' ? 0 : 1;
                break;
        }
    }
    
    argc -= optind;
    argv += optind;
    
    if (argc != 2)
    {
        commandUsage(kCommandCP);
        return -1;
    }
    
    const char *source = argv[0];
    const char *dest = argv[1];
    
    if (!volume->fileByName(source))
    {
        std::fprintf(stderr, "apfm cp: %s: no such file.\n", source);
        return -1;
    }

    // if -i and destination file exists, confirm overwritting it.
    if (iFlag && volume->fileByName(dest))
    {
        bool ok = yes_or_no("Overwrite %s?", dest);
        if (!ok)
        {
            std::fprintf(stderr, "Not overwritten.\n");
            return -1;
        }        
    }
    
    volume->copy(source, dest);
    
    return 0;
}

int action_rm(int argc, char **argv, Pascal::VolumeEntry *volume)
{
    // rm file [file ....]
    
    bool iFlag = ::isatty(STDIN_FILENO);
    int c;
    
    while ((c = ::getopt(argc, argv, "fih")) != -1)
    {
        switch(c)
        {
            case 'f':
                iFlag = false;
                break;
            case 'i':
                iFlag = true;
                break;
                
            case 'h':
            default:
                commandUsage(kCommandRM);
                return c == 'h' ? 0 : 1;
                break;
        }
    }
    
    argc -= optind;
    argv += optind;
    
    // TODO -- catch errors ?
    for (int i = 0; i < argc; ++i)
    {
        Pascal::FileEntryPointer e = volume->fileByName(argv[i]);
        
        if (!e)
        {
            if (iFlag) std::fprintf(stderr, "apfm rm: %s: No such file.\n", argv[i]);
            continue;
        }
        
        if (iFlag)
        {
            bool ok = yes_or_no("Remove %s?", argv[i]);
            if (!ok) continue;
        }
              
        volume->unlink(argv[i]);
    }
    return 0;
    
}

int action_krunch(int argc, char **argv, Pascal::VolumeEntry *volume)
{
    // compress file to remove gaps.
    
    bool iFlag = ::isatty(STDIN_FILENO);
    int c;
    
    while ((c = ::getopt(argc, argv, "fih")) != -1)
    {
        switch(c)
        {
            case 'f':
                iFlag = false;
                break;
            case 'i':
                iFlag = true;
                break;
                
            case 'h':
            default:
                commandUsage(kCommandKRUNCH);
                return c == 'h' ? 0 : 1;
                break;
        }
    }    
    
    argc -= optind;
    argv += optind;
    

    
    if (!volume->canKrunch())
    {
        if (iFlag)
            std::fprintf(stderr, "apfm krunch: Volume already optimized.\n");
            
        return 0;
    }

    if (iFlag)
    {
        bool ok = yes_or_no("Are you sure you want to krunch this volume?");
        if (!ok) return -1;
    }
    
    volume->krunch();
    return 0;
}



int action_get(int argc, char **argv, Pascal::VolumeEntry *volume)
{
    // get [-f] pascal_file [native file];
    
    char *infile;
    char *outfile;
    
    bool iFlag = ::isatty(STDIN_FILENO);
    int c;
    
    while ((c = ::getopt(argc, argv, "fih")) != -1)
    {
        switch(c)
        {
            case 'f':
                iFlag = false;
                break;
            case 'i':
                iFlag = true;
                break;
                
            case 'h':
            default:
                commandUsage(kCommandGET);
                return c == 'h' ? 0 : -1;
                break;
        }
    }      
    
    argc -= optind;
    argv += optind;
    
    Pascal::FileEntryPointer entry;
    
    switch(argc)
    {
        case 1:
            infile = outfile = argv[0];
            break;
        case 2:
            infile = argv[0];
            outfile = argv[1];
            break;
        default:
            commandUsage(kCommandGET);
            return -1;
            break;
    }
    
    entry = volume->fileByName(infile);
    
    
    if (!entry)
    {
        std::fprintf(stderr, "apfm get: %s: no such file.\n", infile);
        return -1;
    }
    
    
    // if not -f, check before overwriting file.
    if (iFlag)
    {
        struct stat st;
        if (::stat(outfile, &st) == 0)
        {
            bool ok = yes_or_no("Overwrite %s?", outfile);
            if (!ok)
            {
                std::fprintf(stderr, "Not overwritten.\n");
                return -1;
            }
            
        }
    }
    

    
    File file(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    
    unsigned fileSize = entry->fileSize();
    unsigned offset = 0;
    
    while (offset < fileSize)
    {
        uint8_t buffer[512];
        
        unsigned count = std::min(512u, fileSize - offset);
        entry->read(buffer, count, offset);
        
        ::write(file.fd(), buffer, count);
        offset += count; 
    }
    
    return 0;
}

int action_put(int argc, char **argv, Pascal::VolumeEntry *volume)
{
    // put [-t type] native_file [pascal_file]
    
    unsigned type = Pascal::kUntypedFile;
    
    struct stat st;
    int c;
    
    char *infile;
    char *outfile;
    char *tmp;
    bool iFlag = ::isatty(STDIN_FILENO);
    bool tFlag = false;
    
    while ((c = getopt(argc, argv, "fhit:")) != -1)
    {
        switch (c)
        {
            case 't':
                tFlag = true;
                
                if (!::strcasecmp("text", optarg))
                    type = Pascal::kTextFile;
                else if (!::strcasecmp("txt", optarg))
                    type = Pascal::kTextFile;
                else if (!::strcasecmp("code", optarg))
                    type = Pascal::kCodeFile;
                else if (!::strcasecmp("info", optarg))
                    type = Pascal::kInfoFile;
                else if (!::strcasecmp("data", optarg))
                    type = Pascal::kDataFile;
                else if (!::strcasecmp("graf", optarg))
                    type = Pascal::kGrafFile;
                else if (!::strcasecmp("foto", optarg))
                    type = Pascal::kFotoFile;
                else type = Pascal::kUntypedFile;
                    
                break;
                
            case 'i':
                iFlag = true;
                break;
            case 'f':
                iFlag = false;
                break;
            case 'h':
            default:
                commandUsage(kCommandPUT);
                return c == 'h' ? 0 : -1;
                break;
        }
        
    }
    
    
    argc -= optind;
    argv += optind;
    
    switch (argc)
    {
        case 1:
            infile = outfile = argv[0];
            
            // need to run basename on outfile.
            tmp = strrchr(outfile, '/');
            if (tmp) outfile = tmp + 1;
            break;
        case 2:
            infile = argv[0];
            outfile = argv[1];
            break;
        default:
            commandUsage(kCommandPUT);
            return -1;
            break;
    }
    
    
    // if no tFlag specified, check .text/.txt
    if (!tFlag)
    {
        tmp = strrchr(outfile, '.');
        if (tmp)
        {
            if (::strcasecmp(tmp, ".text") == 0 || strcasecmp(tmp, ".txt") == 0)
                type = Pascal::kTextFile;
        }
    }
    
    if (!Pascal::FileEntry::ValidName(outfile))
    {
        std::fprintf(stderr, "apfm put: `%s' is not a valid pascal name.\n", outfile);
    }

    if (::stat(infile, &st) != 0)
    {
        std::fprintf(stderr, "apfm put: %s: no such file.\n", infile);
        return -1;
    }

    if (!S_ISREG(st.st_mode))
    {
        std::fprintf(stderr, "apfm put: %s: not a regular file.\n", infile);
        return -1;
    }
    
    unsigned blocks = (st.st_size + 511) / 512;
    if (blocks > (0xffff - 6))
    {
        std::fprintf(stderr, "apfm put: %s: file is too large.\n", infile);
        return -1;
    }
    
    
    
    File file(infile, File::ReadOnly);
    MappedFile mf(file, File::ReadOnly, st.st_size);
    
    
    
    

    if (type == Pascal::kTextFile)
    {
        Pascal::TextWriter text;
        
        
        const char *address = (const char *)mf.address();

        unsigned start = 0;
        unsigned length = st.st_size;
        
        for (unsigned i = 0; i < length; ++i)
        {
            char c = address[i];
            if (c == 0x0d || c == 0x0a)
            {
                //std::fprintf(stdout, "%.*s\n", i - start, address + start);
                text.writeLine(address + start, i - start);
                start = i + 1;
                
                if (c == 0x0a && i < length && address[i +1 ] == 0x0d)
                {
                    ++start;
                    ++i;
                }
            }
        }
        
        //any remainder.
        if (start != length) text.writeLine(address + start, length - start);
        
        blocks = text.blocks();
        
        Pascal::FileEntryPointer entry = volume->create(outfile, blocks);
        if (!entry)
        {
            perror(NULL);
            return -1;
        }
        
        entry->setFileKind(type);
        entry->write(text);
        
    }
    else
    {
        Pascal::FileEntryPointer entry = volume->create(outfile, blocks);
        if (!entry)
        {
            perror(NULL);
            return -1;
        }
        
        entry->setFileKind(type);
                
        unsigned remaining = st.st_size;
        unsigned offset = 0;
        const uint8_t *address = (const uint8_t *)mf.address();
        while (remaining)
        {
            int rv;
            unsigned count = std::min(512u, remaining);
    
            rv = entry->write(address + offset, count, offset);
            if (rv == -1)
            {
                perror(NULL);
                return -1;
            }
            offset += count;
            remaining -= count;
        }
        
    }

    volume->sync();
    return 0;
    
}




int main(int argc, char **argv)
{
    Pascal::VolumeEntryPointer volume;
    Device::BlockDevicePointer device;
  
    unsigned fmt = 0;
    
    int c;
    

    #ifdef __linux__
    putenv((char *)"POSIXLY_CORRECT=1"); // fix getopt to not mutate
    #endif



    // getop stops at first non '-' arg so it will not affect action flags.    
    while ((c = ::getopt(argc, argv, "f:h")) != -1)
    {
        std::printf("%c\n", c);
        switch(c)
        {
            case 'f':
                fmt = Device::BlockDevice::ImageType(optarg);
                if (!fmt)
                {
                    std::fprintf(stderr, "Error: Invalid file format: ``%s''.\n",
                        optarg);
                }
                break;
                
            case 'h':
            default:
                usage();
                return c == 'h' ? 0 : 1;
        }
    }
    

    argc -= optind;
    argv += optind;

    #ifdef __linux__
    optind = 0;
    #else
    optreset = 1;
    optind = 1;
    #endif
    
    if (argc < 2)
    {
        usage();
        return 0;
    }
    
    
    const char *file = argv[0];
    const char *action = argv[1];
    
        
    
    try {
        
        unsigned actionCode = command(action);
        
        device = Device::BlockDevice::Open(file, commandFlags(actionCode), fmt);
    
        volume = Pascal::VolumeEntry::Open(device);
        device.reset();

        switch (actionCode)
        {
            case kCommandCAT:
                return action_cat(argc - 1, argv + 1, volume.get());
                break;
            case kCommandCP:
                return action_cp(argc - 1, argv + 1, volume.get());
                break;
            case kCommandKRUNCH:
                return action_krunch(argc - 1, argv + 1, volume.get());
                break;
            case kCommandLS:
                return action_ls(argc - 1, argv + 1, volume.get());
                break;
            case kCommandMV: 
                return action_mv(argc - 1, argv + 1, volume.get());
                break;
            case kCommandRM:
                return action_rm(argc - 1, argv + 1, volume.get());
                break;
            case kCommandGET:
                return action_get(argc -1, argv + 1, volume.get());
                break;
            case kCommandPUT:
                return action_put(argc -1, argv + 1, volume.get());
                break;
                 
        }
        usage();
        return 3;
    }
    catch (Exception& e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        std::fprintf(stderr, "%s\n", e.errorString());
    }
    
    return 0;
}
