#ifndef __MAPPED_FILE__
#define __MAPPED_FILE__


#include <stdint.h>
#include <cstdlib>

namespace ProFUSE {



class MappedFile {
public:
    MappedFile(const char *name, bool ReadOnly);
    MappedFile(int fd, bool readOnly);
    MappedFile(const char *name, size_t size);
    
    ~MappedFile();
        
    void readBlock(unsigned block, void *bp);
    void writeBlock(unsigned block, const void *bp);
    
    void sync();
    
    void reset();

    bool dosOrder() const { return _dosOrder; }
    void setDosOrder(bool tf) { _dosOrder = tf; }
    
    unsigned offset() const { return _offset; }
    void setOffset(unsigned o) { _offset = o; }
    
    unsigned blocks() const { return _blocks; }
    void setBlocks(unsigned b) { _blocks = b; }
    
    bool readOnly() const { return _readOnly; }
    size_t fileSize() const { return _size; }  
    void *fileData() const { return _map; }
    


private:
    MappedFile& operator=(const MappedFile& other);
    
    void init(int fd, bool readOnly);
    
    static const unsigned DOSMap[];
    
    int _fd;
    void *_map;
    
    size_t _size;
    bool _readOnly;
    
    bool _dosOrder;
    unsigned _offset;
    unsigned _blocks;

};
}
#endif