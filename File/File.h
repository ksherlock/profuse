#ifndef __FILE_H__
#define __FILE_H__

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

 
class File {

    public:
    File();
    File(File &);
    File(int fd);
    File(const char *name, int flags);
    File(const char *name, bool readOnly);
    ~File();

    bool isValid() const
    {
        return _fd >= 0;
    }
    
    int fd() const { return _fd; }
    
    void close();
    
    void adopt(File &f);
    void swap(File &f);
        
    private:
    
    // could call dup() or something.
    File& operator=(const File &f);
    int _fd;    
};

#endif
