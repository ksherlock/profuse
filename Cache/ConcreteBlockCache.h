#ifndef __CONCRETE_BLOCK_CACHE_H__
#define __CONCRETE_BLOCK_CACHE_H__

#include <vector>

#include <Cache/BlockCache.h>

namespace Device {

class ConcreteBlockCache : public BlockCache {
public:
    ConcreteBlockCache(BlockDevicePointer device, unsigned size = 16);
    virtual ~ConcreteBlockCache();

    virtual void sync();
    virtual void write(unsigned block, const void *vp);


    virtual void *acquire(unsigned block);
    virtual void release(unsigned block, int flags);
    virtual void markDirty(unsigned block);

    

private:
    struct Entry {
        unsigned block;
        unsigned count;
        bool dirty;

        struct Entry *next;
        struct Entry *prev;
        struct Entry *nextHash;

        uint8_t buffer[512];

    };

    typedef std::vector<Entry *>::iterator EntryIter;
    
    enum { HashTableSize = 23 };

    std::vector<Entry *>_buffers;

    Entry *_hashTable[HashTableSize];

    Entry *_first;
    Entry *_last;


    unsigned hashFunction(unsigned block);

    Entry *findEntry(unsigned block);
    void removeEntry(unsigned block);
    void addEntry(Entry *);

    Entry *newEntry(unsigned block);

    void pushEntry(Entry *);

    void setLast(Entry *);
    void setFirst(Entry *);
    
    void incrementCount(Entry *);
    void decrementCount(Entry *);
};

}

#endif
