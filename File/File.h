#ifndef __FILE_H__
#define __FILE_H__

#include <unistd.h>
#include <fcntl.h>

namespace File {

enum Flags {
    ReadOnly = O_RDONLY,
    WriteOnly = O_WRONLY,
    ReadWrite = O_RDWR
};

class File {

    public:
    File();
    File(File &);
    File(int fd);
    File(const char *name, int flags);
    ~File();

    int fd() const { return _fd; }
        
    void close();
        
    private:
    int _fd;    
};

}
#endif
