#include <algorithm>
#include <cerrno>

#include <File/File.h>
#include <ProFUSE/Exception.h>



using ProFUSE::Exception;
using ProFUSE::POSIXException;


File::File()
{
    _fd = -1;
}

File::File(int fd)
{
    _fd = fd;
}

File::File(File& f)
{
    _fd = f._fd;
    f._fd = -1;
}

File::File(const char *name, int flags, const std::nothrow_t&)
{
    _fd = ::open(name, flags);
}

File::File(const char *name, bool readOnly, const std::nothrow_t&)
{
    _fd = ::open(name, readOnly ? O_RDONLY : O_RDWR);
}

File::File(const char *name, int flags)
{
    #undef __METHOD__
    #define __METHOD__ "File::File"

    _fd = ::open(name, flags);
    if (_fd < 0)
        throw POSIXException( __METHOD__ ": open", errno);
}


File::File(const char *name, bool readOnly)
{
#undef __METHOD__
#define __METHOD__ "File::File"
    
    _fd = ::open(name, readOnly ? O_RDONLY : O_RDWR);
    if (_fd < 0)
        throw POSIXException( __METHOD__ ": open", errno);    
}


File::~File()
{
    close();
}

void File::close()
{
    #undef __METHOD__
    #define __METHOD__ "File::close"

    if (_fd >= 0)
    {
        int fd = _fd;
        _fd = -1;
    
        if (::close(fd) != 0)
            throw POSIXException(__METHOD__ ": close", errno);
    }
}


void File::adopt(File &f)
{
    if (&f == this) return;
    
    close();
    _fd = f._fd;
    f._fd = -1;
}

void File::adopt(int fd)
{
    if (fd == _fd) return;
    close();
    _fd = fd;
}


void File::swap(File &f)
{
    std::swap(_fd, f._fd);
}
