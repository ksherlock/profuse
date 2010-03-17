#include <algorithm>
#include <cerrno>

#include <sys/stat.h>

#include <File/MappedFile.h>
#include <ProFUSE/Exception.h>


using ProFUSE::POSIXException;

MappedFile::MappedFile()
{
    _length = -1;
    _address = MAP_FAILED;
}

MappedFile::MappedFile(MappedFile &mf)
{
    _address = mf._address;
    _length = mf._length;
    
    mf._address = MAP_FAILED;
    mf._length = -1;
}

MappedFile::MappedFile(File f, bool readOnly)
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::MappedFile"

    struct stat st;
    
    // close enough
    if (f.fd() < 0)
        throw POSIXException( __METHOD__, EBADF);


    if (::fstat(f.fd(), &st) != 0)
        throw POSIXException(__METHOD__ ": fstat", errno);
    
    if (!S_ISREG(st.st_mode))
        throw POSIXException(__METHOD__, ENODEV);
        
    _length = st.st_size;
    _address = ::mmap(0, _length, 
        readOnly ? PROT_READ : PROT_READ | PROT_WRITE, 
        MAP_FILE | MAP_SHARED, f.fd(), 0); 

    if (_address == MAP_FAILED)
        throw POSIXException(__METHOD__ ": mmap", errno);
}

MappedFile::~MappedFile()
{
    close();
}



void MappedFile::close()
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::close"
    
    if (_address != MAP_FAILED)
    {
        void *address = _address;
        size_t length = _length;
        
        _address = MAP_FAILED;
        _length = -1;
        
        if (::munmap(address, length) != 0)
            throw POSIXException(__METHOD__ ": munmap", errno);
    }
}

void MappedFile::sync()
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::sync"
    
    if (_address != MAP_FAILED)
    {
        if (::msync(_address, _length, MS_SYNC) != 0)
            throw POSIXException(__METHOD__ ": msync", errno);
    }
}

void MappedFile::adopt(MappedFile &mf)
{
    close();
    _address = mf._address;
    _length = mf._length;
    
    mf._address = MAP_FAILED;
    mf._length = -1;
}

void MappedFile::swap(MappedFile &mf)
{
    std::swap(_address, mf._address);
    std::swap(_length, mf._length);
}