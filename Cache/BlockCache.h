#ifndef __BLOCKCACHE_H__
#define __BLOCKCACHE_H__

#include <stdint.h>
#include <vector>
#include <Device/Device.h>


class MappedFile;

namespace Device {


enum BlockReleaseFlags {
    kBlockDirty = 1,
    kBlockCommitNow = 2,
    kBlockReuse = 3
};

class BlockCache {
public:

    static BlockCachePointer Create(BlockDevicePointer device);

    virtual ~BlockCache();

    bool readOnly() { return _readOnly; }
    unsigned blocks() { return _blocks; }
    BlockDevicePointer device() { return _device; }
    
        
    virtual void sync() = 0;
    virtual void write(unsigned block, const void *bp);
    virtual void read(unsigned block, void *bp);

    virtual void *acquire(unsigned block) = 0;
    virtual void release(unsigned block, int flags) = 0 ;
    virtual void markDirty(unsigned block) = 0;
    
    
    virtual void zeroBlock(unsigned block);
    
    void release(unsigned block) { release(block, 0); }
    void release(unsigned block, bool dirty) 
    {
        release(block, dirty ? kBlockDirty : 0);
    }

protected:
    BlockCache(BlockDevicePointer device);

    BlockDevicePointer _device;
    
private:
    unsigned _blocks;
    bool _readOnly;
};

    
} // namespace

#endif
