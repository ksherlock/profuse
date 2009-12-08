#include "BlockCache.h"
#include "BlockDevice.h"
#include "auto.h"

#include <algorithm>



/*
 * Note -- everything is assumed to be single-threaded.
 *
 */



using namespace ProFUSE;

typedef std::vector<BlockDescriptor>::iterator BDIter;

const unsigned CacheSize = 16; 

BlockCache::BlockCache(BlockDevice *device)
{
    _device = device;
    _ts = 0;
    _blocks.reserve(CacheSize);
}

BlockCache::~BlockCache()
{
    for (BDIter iter = _blocks.begin(); iter != _blocks.end(); ++iter)
    {
        if (iter->data) delete[] iter->data;
    }
}


void *BlockCache::acquire(unsigned block)
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
    
    
    if (freeCount && (_blocks.size() >= CacheSize))
    {
        oldest->block = block;
        oldest->count = 1;
        oldest->ts = _ts;
        
        _device->read(block, oldest->data);
        return oldest->data;
    }
    

    auto_array<uint8_t> buffer(new uint8_t[512]);
    BlockDescriptor bd = { block, 1, _ts, buffer.get() };
    
    _device->read(block, buffer.get());    

    _blocks.push_back(bd);

    
    return buffer.release();
}


void BlockCache::release(unsigned block)
{

    for (BDIter iter = _blocks.begin(); iter != _blocks.end(); ++iter)
    {
        if (iter->block == block)
        {
            if (!--iter->count)
            {
                _device->write(block, iter->data);
                
                // trim back if too many entries.
                
                if (_blocks.size() > CacheSize)
                {
                    delete[] iter->data;
                    _blocks.erase(iter);
                }                
            }            
            return;
        }
    }
}