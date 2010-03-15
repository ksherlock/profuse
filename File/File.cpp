#include "File.h"

#include <cerrno>

using namespace File;

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

File::File(const char *name, int flags)
{
    _fd = ::open(name, flags);
    if (_fd < 0)
        throw ProFUSE::PosixException(errno);
}

File::~File()
{
    close();
}

File::close()
{
    int fd = _fd;
    _fd = -1;
    
    if (fd >= 0 && ::close(fd) != 0)
        throw ProFUSE::PosixException(errno);
}

