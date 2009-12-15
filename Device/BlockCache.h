#ifndef __BLOCKCACHE_H__
#define __BLOCKCACHE_H__

#include <stdint.h>
#include <vector>

namespace ProFUSE {

class BlockDevice;



class AbstractBlockCache {
public:
    virtual ~AbstractBlockCache();
    
    virtual void write() = 0;
    
    virtual void *load(unsigned block) = 0;
    virtual void unload(unsigned block, bool dirty) = 0;
    
    void unload(unsigned block) { unload(block, false); }
};


class BlockCache : public AbstractBlockCache {
public:
    BlockCache(BlockDevice *device, unsigned size = 16);
    ~BlockCache();
    
    virtual void write();
    
    virtual void *load(unsigned block);
    virtual void unload(unsigned block, bool dirty);


private:

    struct BlockDescriptor {
        unsigned block;
        unsigned count;
        unsigned ts;
        bool dirty;
        uint8_t *data;
    };
    


    std::vector<BlockDescriptor> _blocks;
    BlockDevice *_device;
    unsigned _ts;
    unsigned _cacheSize;
    
};

class MappedBlockCache : public AbstractBlockCache {
    public:
    
    MappedBlockCache(void *data, unsigned blocks);
    
    virtual void write();
    
    virtual void *load(unsigned block);
    virtual void unload(unsigned block, bool dirty);    
    
    private:
        unsigned _blocks;
        uint8_t * _data;
};

} // namespace

#endif