#ifndef __MAPPED_BLOCK_CACHE_H__
#define __MAPPED_BLOCK_CACHE_H__

#include <BlockCache.h>

namespace Device {

class MappedBlockCache : public BlockCache {
    public:

    MappedBlockCache(BlockDevice *, void *data);
    virtual ~MappedBlockCache();

    virtual void sync() = 0;
    virtual void write(unsigned block, const void *vp);


    virtual void *acquire(unsigned block);
    virtual void release(unsigned block, int flags);
    virtual void markDirty(unsigned block);

    private:
        void *_data;
        bool _dirty;

};

} // namespace


