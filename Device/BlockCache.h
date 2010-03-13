#ifndef __BLOCKCACHE_H__
#define __BLOCKCACHE_H__

#include <stdint.h>
#include <vector>

namespace Device {

class BlockDevice;
class MappedFile;


class BlockCache {
public:

    BlockCache *Create(BlockDevice *device, unsigned size = 16);

    virtual ~BlockCache();

    bool readOnly() { return _readOnly; }
    unsigned blocks() { return _blocks; }
    BlockDevice *device() { return _device; }
    
        
    virtual void sync() = 0;
    virtual void write(unsigned block, const void *bp);
    virtual void read(unsigned block, void *bp);

    virtual void *acquire(unsigned block) = 0;
    virtual void release(unsigned block, bool dirty) = 0;
    virtual void markDirty(unsigned block) = 0;
    
    void release(unsigned block) { release(block, false); }

protected:
    BlockCache(BlockDevice *device);

    BlockDevice *_device;
private
    unsigned _blocks;
    bool _readOnly;
};


class ConcreteBlockCache : public BlockCache {
public:
    ConcreteBlockCache(BlockDevice *device, unsigned size = 16);
    virtual ~ConcreteBlockCache();
    
    virtual void sync();
    virtual void write(unsigned block, const void *vp) = 0;


    virtual void *acquire(unsigned block);
    virtual void release(unsigned block, bool dirty);
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
    
    incrementCount(Entry *);
    decrementCount(Entry *);
};

class MappedBlockCache : public BlockCache {
    public:
    
    MappedBlockCache(BlockDevice *, void *data);
    virtual ~MappedBlockCache();
    
    virtual void sync() = 0;
    virtual void write(unsigned block, const void *vp);


    virtual void *acquire(unsigned block);
    virtual void release(unsigned block, bool dirty);
    virtual void markDirty(unsigned block);
    
    private:
        void *_data;
    
};

} // namespace

#endif