#ifndef __BLOCKCACHE_H__
#define __BLOCKCACHE_H__

#include <stdint.h>
#include <vector>

class MappedFile;

namespace Device {

class BlockDevice;

enum BlockReleaseFlags {
    kBlockDirty = 1,
    kBlockCommitNow = 2,
    kBlockReuse = 3
};

class BlockCache {
public:

    static BlockCache *Create(BlockDevice *device);

    virtual ~BlockCache();

    bool readOnly() { return _readOnly; }
    unsigned blocks() { return _blocks; }
    BlockDevice *device() { return _device; }
    
        
    virtual void sync() = 0;
    virtual void write(unsigned block, const void *bp);
    virtual void read(unsigned block, void *bp);

    virtual void *acquire(unsigned block) = 0;
    virtual void release(unsigned block, int flags) = 0 ;
    virtual void markDirty(unsigned block) = 0;
    
    void release(unsigned block) { release(block, 0); }
    void release(unsigned block, bool dirty) 
    {
        release(block, dirty ? kBlockDirty : 0);
    }

protected:
    BlockCache(BlockDevice *device);

    BlockDevice *_device;
    
private:
    unsigned _blocks;
    bool _readOnly;
};


} // namespace

#endif
