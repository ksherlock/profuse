
#include <algorithm>
#include <cerrno>

#include <sys/types.h>
#include <sys/mman.h>


#include "BlockDevice.h"
#include "BlockCache.h"
#include "Exception.h"
#include "auto.h"


/*
 * Note -- everything is assumed to be single-threaded.
 *
 */



using namespace ProFUSE;

#pragma mark -
#pragma mark AbstractBlockCache

AbstractBlockCache::~AbstractBlockCache()
{
}

#pragma mark -
#pragma mark MappedBlockCache

MappedBlockCache::MappedBlockCache(void *data, unsigned blocks)
{
    _blocks = blocks;
    _data = (uint8_t *)_data;
}

void MappedBlockCache::write()
{
    // TODO...
}

void *MappedBlockCache::load(unsigned block)
{
#undef __METHOD__
#define __METHOD__ "MappedBlockCache::load"
    
    if (block >= _blocks)
        throw Exception(__METHOD__ ": Invalid block.");
        
    
    return _data + block * 512;
}
void MappedBlockCache::unload(unsigned block, bool dirty)
{
#undef __METHOD__
#define __METHOD__ "MappedBlockCache::unload"

    if (!dirty) return;
    if (::msync(_data + block * 512, 512, MS_ASYNC) < 0)
    {
        throw POSIXException(__METHOD__ ": msync failed.", errno);
    }
}


#pragma mark -
#pragma mark BlockCache

typedef std::vector<BlockCache::BlockDescriptor>::iterator BDIter;



BlockCache::BlockCache(BlockDevice *device, unsigned size)
{
    _device = device;
    _ts = 0;
    _cacheSize = std::max(16u, size);
    _blocks.reserve(_cacheSize);
}

BlockCache::~BlockCache()
{
    for (BDIter iter = _blocks.begin(); iter != _blocks.end(); ++iter)
    {
        if (iter->data) delete[] iter->data;
    }
}

void BlockCache::write()
{
    for (BDIter iter = _blocks.begin(); iter != _blocks.end(); ++iter)
    {
        if (iter->dirty)
        {
            _device->write(iter->block, iter->data);
            iter->dirty = false;
        }
    }
}



void *BlockCache::load(unsigned block)
{
    unsigned mints = -1;
    unsigned freeCount = 0;
    
    BDIter oldest;


    ++_ts;

    /*
     * keep a pointer to the oldest free entry (according to ts).
     * and re-use if >= 16 entries. 
     */
 
    
    for (BDIter iter = _blocks.begin(); iter != _blocks.end(); ++iter)
    {
        if (iter->block == block)
        {
            ++iter->count;
            iter->ts = _ts;
                
            return iter->data;
        }
        
        if (iter->count == 0)
        {
            ++freeCount;
            if (iter->ts < mints)
            {
                mints = iter->ts;
                oldest = iter;
            }
        }
    }
    
    
    if (freeCount && (_blocks.size() >= _cacheSize))
    {
        // re-use old buffer.
        
        oldest->block = block;
        oldest->count = 1;
        oldest->ts = _ts;
        oldest->dirty = false;
        
        _device->read(block, oldest->data);
        return oldest->data;
    }
    

    auto_array<uint8_t> buffer(new uint8_t[512]);
    BlockDescriptor bd = { block, 1, _ts, false, buffer.get() };
    
    _device->read(block, buffer.get());    

    _blocks.push_back(bd);

    
    return buffer.release();
}


void BlockCache::unload(unsigned block, bool dirty)
{

    for (BDIter iter = _blocks.begin(); iter != _blocks.end(); ++iter)
    {
        if (iter->block == block)
        {
            iter->dirty = dirty || iter->dirty;
            if (!--iter->count)
            {
                if (iter->dirty)
                {
                    _device->write(block, iter->data);
                    iter->dirty = false;
                }
                // trim back if too many entries.
                if (_blocks.size() > _cacheSize)
                {
                    delete[] iter->data;
                    _blocks.erase(iter);
                }                
            }            
            return;
        }
    }
}