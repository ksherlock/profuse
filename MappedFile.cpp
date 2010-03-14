#include <unistd.h>
#include <sys/mman.h>

#include <cerrno>

class MappedFile
{
public:

    MappedFile(int fd, bool readOnly)
    {
        _fd = fd;
        _length = ::lseek(fd, 0, SEEK_END);
        _address = MAP_FAILED;
        if (_length > 0)
        {
            _address = ::mmap(0, _length, 
                readOnly ? PROT_READ : PROT_READ | PROT_WRITE, 
                MAP_FILE, fd, 0); 
        }
    }
    
    MappedFile(void *address, size_t size)
    {
        _fd = -1;
        _address = address;
        _size = size;
    }
    
    ~MappedFile()
    {
        if (_address != MAP_FAILED) ::munmap(_address, _length);
        if (_fd != -1) ::close(_fd);
    }

    void *address() const
    {
        return _address;
    }
    
    size_t length()
    {
        return _length;
    }

    int sync()
    {
    
        if (::msync(_address, _length, MS_SYNC) == 0)
            return 0;
        return errno;
    }



private:
    int _fd;
    void *_address;
    size_t _length;

};