#ifndef __FILE_H__
#define __FILE_H__

#include <new>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

 
class File {

    public:
    File();
    File(File &);
    File(int fd);

    File(const char *name, int flags);
    File(const char *name, int flags, mode_t mode);
    File(const char *name, bool readOnly);

    File(const char *name, int flags, const std::nothrow_t &);
    File(const char *name, int flags, mode_t mode, const std::nothrow_t &);
    File(const char *name, bool readOnly, const std::nothrow_t &);

    ~File();

    bool isValid() const
    {
        return _fd >= 0;
    }
    
    int fd() const { return _fd; }
    
    void close();
    
    void adopt(File &f);
    void adopt(int fd);
    
    void swap(File &f);
        
    private:
    
    // could call dup() or something.
    File& operator=(const File &f);
    int _fd;    
};

#endif
