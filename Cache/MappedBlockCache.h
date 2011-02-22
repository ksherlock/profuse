#ifndef __MAPPED_BLOCK_CACHE_H__
#define __MAPPED_BLOCK_CACHE_H__

#include <Cache/BlockCache.h>

namespace Device {

class MappedBlockCache : public BlockCache {
    public:

    static BlockCachePointer Create(BlockDevicePointer device, void *data);

    virtual ~MappedBlockCache();

    virtual void sync();
    virtual void write(unsigned block, const void *vp);

    virtual void zeroBlock(unsigned block);    
    

    virtual void *acquire(unsigned block);
    virtual void release(unsigned block, int flags);
    virtual void markDirty(unsigned block);

    private:

    MappedBlockCache(BlockDevicePointer device, void *data);

    void sync(unsigned block);
        
    uint8_t *_data;
    bool _dirty;
};

} // namespace

#endif

