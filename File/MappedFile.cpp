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
    _readOnly = true;
}

MappedFile::MappedFile(MappedFile &mf)
{
    _address = mf._address;
    _length = mf._length;
    _readOnly = mf._readOnly;
    
    mf._address = MAP_FAILED;
    mf._length = -1;
    mf._readOnly = true;
}

MappedFile::MappedFile(const File &f, File::FileFlags flags, size_t size)
{
    _length = -1;
    _address = MAP_FAILED;
    _readOnly = true;
    
    init(f, flags == File::ReadOnly, size);
}


MappedFile::MappedFile(const char *name, File::FileFlags flags)
{
    File f(name, flags);
    
    _length = -1;
    _address = MAP_FAILED;
    _readOnly = true;
    
    init(f, flags == File::ReadOnly, 0);
    
}


MappedFile::MappedFile(const char *name, File::FileFlags flags, const std::nothrow_t &nothrow)
{
    File f(name, flags, nothrow);
    
    _length = -1;
    _address = MAP_FAILED;
    _readOnly = true;
    
    if (f.isValid()) 
        init(f, flags == File::ReadOnly, 0);
}


MappedFile::~MappedFile()
{
    close();
}



void MappedFile::init(const File &f, bool readOnly, size_t size)
{
#undef __METHOD__
#define __METHOD__ "MappedFile::init"
    
    struct stat st;
    int prot = readOnly ? PROT_READ : PROT_READ | PROT_WRITE;
    int flags =  MAP_FILE | MAP_SHARED;
    
    // close enough
    if (f.fd() < 0)
        throw POSIXException( __METHOD__, EBADF);
    
    
    if (!size)
    {
        if (::fstat(f.fd(), &st) != 0)
            throw POSIXException(__METHOD__ ": fstat", errno);
        
        if (!S_ISREG(st.st_mode))
            throw POSIXException(__METHOD__, ENODEV);
        
        size = st.st_size;
    }
    
    _length = size;
    _address = ::mmap(0, _length, prot, flags, f.fd(), 0); 
    
    if (_address == MAP_FAILED)
        throw POSIXException(__METHOD__ ": mmap", errno);
    
    _readOnly = readOnly;
}



void MappedFile::close()
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::close"
    
    if (_address != MAP_FAILED)
    {
        /*
        void *address = _address;
        size_t length = _length;
        */
        
        ::munmap(_address, _length);
        
        _address = MAP_FAILED;
        _length = -1;
        _readOnly = true;
        
        // destructor shouldn't throw.
        /*
        if (::munmap(address, length) != 0)
            throw POSIXException(__METHOD__ ": munmap", errno);
         */
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
    _readOnly = mf._readOnly;
    
    mf._address = MAP_FAILED;
    mf._length = -1;
    mf._readOnly = true;
}

void MappedFile::swap(MappedFile &mf)
{
    std::swap(_address, mf._address);
    std::swap(_length, mf._length);
    std::swap(_readOnly, mf._readOnly);
}


MappedFile *MappedFile::Create(const char *name, size_t size)
{
#undef __METHOD__
#define __METHOD__ "MappedFile::Create"
    
    File fd(::open(name, O_CREAT | O_TRUNC | O_RDWR, 0644));

    if (!fd.isValid())
    {
        throw POSIXException(__METHOD__ ": Unable to create file.", errno);        
    }

    // TODO -- is ftruncate portable?
    if (::ftruncate(fd.fd(), size) < 0)
    {
        // TODO -- unlink?
        throw POSIXException(__METHOD__ ": Unable to truncate file.", errno);    
    }
    
    return new MappedFile(fd, File::ReadWrite, size);
}