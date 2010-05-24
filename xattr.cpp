#include <string>
#include <vector>
#include <algorithm>

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cerrno>

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/xattr.h>


typedef std::vector<std::string>::iterator vsiter;

#ifdef __APPLE__
// apple has additional parameter for position and options.

inline ssize_t getxattr(const char *path, const char *name, void *value, size_t size)
{
    return getxattr(path, name, value, size, 0, XATTR_NOFOLLOW);
}

// apple has additional parameter for options.
inline ssize_t listxattr(const char *path, char *namebuf, size_t size)
{
    return listxattr(path, namebuf, size, XATTR_NOFOLLOW);
}

#endif

void hexdump(const uint8_t *data, ssize_t size)
{
const char *HexMap = "0123456789abcdef";

    char buffer1[16 * 3 + 1 + 1];
    char buffer2[16 + 1];
    ssize_t offset = 0;
    unsigned i, j;
    
    
    while(size > 0)
    {        
        std::memset(buffer1, ' ', sizeof(buffer1));
        std::memset(buffer2, ' ', sizeof(buffer2));
        
        unsigned linelen = (unsigned)std::min(size, (ssize_t)16);
        
        
        for (i = 0, j = 0; i < linelen; i++)
        {
            unsigned x = data[i];
            buffer1[j++] = HexMap[x >> 4];
            buffer1[j++] = HexMap[x & 0x0f];
            j++;
            if (i == 7) j++;
            
            // isascii not part of std:: and may be a macro.
            buffer2[i] = isascii(x) && std::isprint(x) ? x : '.';
            
        }
        
        buffer1[sizeof(buffer1)-1] = 0;
        buffer2[sizeof(buffer2)-1] = 0;
        
    
        std::printf("%06x:\t%s\t%s\n", (unsigned)offset, buffer1, buffer2);
        offset += 16;
        data += 16;
        size -= 16;
    }
    std::printf("\n");
}

/*
 * build a list of attribute names for a file. 
 * returns -1 on failure
 * otherwise, returns # of attributes.
 *
 */
ssize_t get_attr_list(const char * fname, std::vector<std::string> &out)
{
    ssize_t asize = listxattr(fname, NULL, 0);
    if (asize < 0) return -1;
    if (asize == 0) return 0;
    
    char *buffer = new char[asize];
    
    if ((asize = listxattr(fname, buffer, asize)) < 0)
    {
        delete []buffer;
        return -1;
    }
    
    //hexdump((uint8_t *)buffer, asize);
    
    char *cp = buffer;
    // buffer is null-terminated utf-8 strings
    // eg:
    // c a t 0 d o g 0
    // 0 1 2 3 4 5 6 7 
    while (asize > 0)
    {
        ssize_t len = std::strlen(cp);
                
        out.push_back(std::string(cp, len));
        ++len;
        asize -= len;
        cp += len;
    }
    
    delete []buffer;
    
    return out.size();
}





/*
 * hexdump an individual attribute.
 */
void dumpxattr(const char *file, const char *attr)
{
ssize_t asize;
char *buffer;

    asize = getxattr(file, attr, NULL, 0);
    if (asize < 0)
    {
        std::perror(attr);
        return;
    }
    
    std::printf("%s: %u\n", attr, (unsigned)asize);

    buffer = new char[asize];
    //if (!buffer) return;
    
    asize = getxattr(file, attr, buffer, asize);
    if (asize >= 0)
    {
        hexdump((uint8_t *)buffer, asize);
    }
    else
    {
        std::perror(attr);
    }
    
    delete []buffer;
}


/*
 * list a file's attributes (and size)
 *
 */
int list(int argc, char **argv)
{

    const char *fname = *argv;
    
    std::vector<std::string>attr;
    
    if (argc == 1)
    {
        get_attr_list(fname, attr);
        std::sort(attr.begin(), attr.end());
    }
    else 
    {
        for (int i = 1; i < argc; ++i)
        {
            attr.push_back(argv[i]);
        }
    }
 

    // find the max name length (not utf-8 happy)
    unsigned maxname = 0;
    for (vsiter i = attr.begin(); i != attr.end() ; ++i)
    {
        maxname = std::max(maxname, (unsigned)i->length());
    }
    maxname += 2;
    
    for (vsiter i = attr.begin(); i != attr.end() ; ++i)
    {
        const char *attr_name = i->c_str();
        
        ssize_t asize = getxattr(fname, attr_name, NULL, 0);
        if (asize >= 0)
        {
            std::printf("%-*s %8u\n", maxname, attr_name, (unsigned)asize); 
        }
        else
        {
            std::perror(attr_name);
        }
    }
    
    return 0;
}


/*
 * hexdump a file's attributes.
 *
 */
int dump(int argc, char **argv)
{

    const char *fname = *argv;
    
    std::vector<std::string>attr;
    
    if (argc == 1)
    {
        get_attr_list(fname, attr);
        std::sort(attr.begin(), attr.end());
    }
    else 
    {
        for (int i = 1; i < argc; ++i)
        {
            attr.push_back(argv[i]);
        }
    }
 
    
    for (vsiter i = attr.begin(); i != attr.end() ; ++i)
    {
        const char *attr_name = i->c_str();
        
        dumpxattr(fname, attr_name);
    }
    
    return 0;
}


// must specify the xattr name.
int read(int argc, char **argv)
{

    if (argc != 2)
    {
        std::fprintf(stderr, "Must specify attribute to be read.\n");
        return -1;
    }
    
    const char *fname = argv[0];
    const char *attr_name = argv[1];
    
    // get the attribute size and read it in.
    ssize_t asize = getxattr(fname, attr_name, NULL, 0);
    if (asize < 0)
    {
        std::perror(attr_name);
        return -1;
    }
    if (asize == 0) return 0;
    
    uint8_t *buffer = new uint8_t[asize];
    asize = getxattr(fname, attr_name, buffer, asize);

    if (asize < 0)
    {
        std::perror(attr_name);
        return -1;
    }
    
    if (::write(STDOUT_FILENO, buffer, asize) != asize)
    {
        perror(NULL);
        delete []buffer;
        return -1;
    }

    
    delete []buffer;
    return 0;
    

}


ssize_t read_all(int fd, std::vector<uint8_t> &out)
{
    struct stat st;
    ssize_t bsize;
    
    // stdin could be a file, a pipe, or a tty.
    if (fstat(fd, &st) < 0)
    {
        std::perror("stdin");
        return -1;
    }
    

    bsize = std::max(st.st_blksize, (blksize_t)512);
    out.reserve(std::max(st.st_size, (off_t)512));

    uint8_t *buffer = new uint8_t[bsize];
    
    for(;;)
    {
        ssize_t size = ::read(fd, buffer, bsize);
                
        if (size == 0) break;
        if (size == -1)
        {
            if (errno == EINTR) continue;
            delete[] buffer;
            std::perror(NULL);
            return -1;
        }
        
        // force reserve?
        out.insert(out.end(), buffer, buffer + size);
    }
    
    
    delete []buffer;

    return out.size();
}


// xattr write filename attrname
// stdin -> filename:attrname 
int write(int argc, char **argv)
{

    std::vector<uint8_t> buffer;
    int flags = 0;
        
    if (argc != 2)
    {
        std::fprintf(stderr, "Must specify attribute to be read.\n");
        return -1;
    }
    
    const char *fname = argv[0];
    const char *attr_name = argv[1];
    
    if (read_all(STDIN_FILENO, buffer) < 0)
    {
        return -1;
    }

    // Flag options:
    // XATTR_CREATE : fails if attr already exists.
    // XATTR_REPLACE : fails if attr does not exist.
    // if neither of the above is specified, will create and replace.
    // XATTR_NOFOLLOW : do not follow symbolic links (Apple only)

    #ifdef __APPLE__
    flags = XATTR_NOFOLLOW;
    #endif

    ssize_t asize = setxattr(fname, attr_name, &buffer[0], buffer.size(), flags);
    
    if (asize < 0)
    {
        std::perror(attr_name);
        return -1;
    }
    
    return 0;
}


void usage(const char *name)
{
    std::printf("usage:\n");
    std::printf("%s list file [attr ...]\n", name);
    std::printf("%s dump file [attr ...]\n", name);
    std::printf("%s read file attr\n", name);
    std::printf("%s write file attr\n", name);
    
    std::exit(0);
}

/*
 * xattr list file [xattr ...]
 * xattr dump file [xattr ...]
 *
 */
int main(int argc, char **argv)
{
    if (argc < 3) usage(*argv);
    
    if (std::strcmp(argv[1], "list") == 0) return list(argc - 2, argv + 2);
    if (std::strcmp(argv[1], "dump") == 0) return dump(argc - 2, argv + 2);
    
    if (std::strcmp(argv[1], "read") == 0) return read(argc - 2, argv + 2);
    if (std::strcmp(argv[1], "write") == 0) return write(argc - 2, argv + 2);
    
    
    usage(*argv);

    return 1;
}
