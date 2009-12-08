#ifndef __BLOCKCACHE_H__
#define __BLOCKCACHE_H__

#include <stdint.h>
#include <vector>

namespace ProFUSE {

class BlockDevice;

struct BlockDescriptor {
    unsigned block;
    unsigned count;
    unsigned ts;
    uint8_t *data;
};



class BlockCache {
    BlockCache(BlockDevice *device);
    ~BlockCache();
    
    void write();
    
    void *acquire(unsigned block);
    void release(unsigned block);
    
private:
    std::vector<BlockDescriptor> _blocks;
    BlockDevice *_device;
    unsigned _ts;
};

}

#endif