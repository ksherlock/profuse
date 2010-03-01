#ifndef __BLOCK_CACHE_H__
#define __BLOCK_CACHE__

#include <stdint.h>

#include <vector>
#include <list>

//#include <ext/hash_map>
//typedef  std::__gnu_cxx::hash_map hash_map;

namespace ProFUSE {

class BlockCache {
    public:
    
    virtual ~BlockCache();
    
    virtual uint8_t *acquire(unsigned block) = 0;
    virtual void release(unsigned block, bool dirty = false) = 0;
    virtual void markDirty(unsigned block) = 0;
    
    virtual void sync() = 0;
    
};


class FileBlockCache : public BlockCache {


    FileBlockCache(Device *);
    virtual ~FileBlockCache();

    virtual uint8_t *acquire(unsigned block);
    virtual void release(unsigned block, bool dirty = false);
    virtual void markDirty(unsigned block);
    
    virtual void sync();

private:
    struct Entry;
    
    enum { HashEntries = 23 };
    
    
    unsigned hashFunction(unsigned);
    
    void removeEntry(Entry *);
    Entry *findEntry(unsigned block);
    void sync(Entry *);
    
    std::vector<Entry *> _pages;
    std::list<Entry *> _unused;
    
    Entry *_hashMap[HashEntries];

    Device *_device;

};

class MappedBlockCache : public BlockCache
{
public:

    MappedBlockCache(Device *);
    virtual ~MappedBlockCache();

    virtual uint8_t *acquire(unsigned block);
    virtual void release(unsigned block, bool dirty = false);
    virtual void markDirty(unsigned block);
    
    virtual void sync();

    
    private:
        Device *_device;
};

}

#endif
