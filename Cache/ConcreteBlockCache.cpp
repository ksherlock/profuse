
#include <algorithm>
#include <cerrno>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <Device/BlockDevice.h>
#include <Cache/ConcreteBlockCache.h>

#include <ProFUSE/Exception.h>
#include <ProFUSE/auto.h>


/*
 * Note -- everything is assumed to be single-threaded.
 *
 */

/*
 * The primary purpose of the block cache is not as a cache 
 * (the OS probably does a decent job of that) but rather to
 * simplify read/writes. Blocks will always be accessed by
 * pointer, so any updates will be shared.
 * For memory mapped prodos-order files, MappedBlockCache just
 * returns a pointer to the memory.
 * For dos-order, nibblized, or raw devices, ConcreteBlockCache
 * uses an approach similar to minix (although the buffer pool will 
 * expand if needed).
 *
 * _buffers is a vector of all buffers and only exists to make
 * freeing them easier.
 * _hashTable is a simple hashtable of loaded blocks.
 * _first and _last are a double-linked list of unused blocks, stored
 * in lru order.
 *
 * The Entry struct contains the buffer, the block, a dirty flag, and an in-use
 * count as well as pointer for the hashtable and lru list.
 * When a block is loaded, it is stored in the _hashTable.  It remains in the 
 * hash table when the in-use count goes to 0 (it will also be added to the
 * end of the lru list).
 * 
 * dirty buffers are only written to disk in 3 scenarios:
 * a) sync() is called
 * b) the cache is deleted
 * c) a buffer is re-used from the lru list.
 */


using namespace Device;

using ProFUSE::Exception;
using ProFUSE::POSIXException;


typedef std::vector<ConcreteBlockCache::Entry *>::iterator EntryIter;



ConcreteBlockCache::ConcreteBlockCache(BlockDevice *device, unsigned size) :
    BlockCache(device)
{
    if (size < 16) size = 16;
    
    std::memset(_hashTable, 0, sizeof(Entry *) * HashTableSize);
    
    for (unsigned i = 0; i < size; ++i)
    {
        Entry *e = new Entry;
        
        std::memset(e, 0, sizeof(Entry));
        _buffers.push_back(e);
    }
    _first = _last = NULL;
    
    _first = _buffers.front();
    _last = _buffers.back();
    // form a chain....
    for (unsigned i = 0; i < size; ++i)
    {
        Entry *e = _buffers[i];
        
        if (i > 0) e->prev = _buffers[i - 1];
        
        if (i < size - 1) e->next = _buffers[i + 1];
    }
    
}

ConcreteBlockCache::~ConcreteBlockCache()
{
    EntryIter iter;
    for (iter = _buffers.begin(); iter != _buffers.end(); ++iter)
    {
        Entry *e = *iter;
        
        if (e->dirty)
        {
            _device->write(e->block, e->buffer);
        }
        
        delete e;
    }
    _device->sync();
}


void ConcreteBlockCache::sync()
{
    EntryIter iter;
    for (iter = _buffers.begin(); iter != _buffers.end(); ++iter)
    {
        Entry *e = *iter;
        
        if (e->dirty)
        {
            _device->write(e->block, e->buffer);
            e->dirty = false;
        }
    }
    _device->sync();
}


void ConcreteBlockCache::write(unsigned block, const void *bp)
{
    Entry *e = findEntry(block);
    
    if (e)
    {
        e->dirty = true;
        std::memcpy(e->buffer, bp, 512);
        return;
    }
    
    // returns a new entry not in the hashtable or the linked list.
    // we add it to both.
    e = newEntry(block);
    
    e->count = 0;
    e->dirty = true;
    
    std::memcpy(e->buffer, bp, 512);
    
    addEntry(e);
    setLast(e);
}

void ConcreteBlockCache::markDirty(unsigned block)
{
    Entry *e = findEntry(block);
    
    if (e) e->dirty = true;
    // error otherwise?
}


void ConcreteBlockCache::release(unsigned block, int flags)
{
    Entry *e = findEntry(block);
    bool dirty = flags & (kBlockDirty | kBlockCommitNow);
    
    if (e)
    {
        if (dirty) e->dirty = true;    
        
        decrementCount(e);
        
        if (flags & kBlockCommitNow)
        {
            _device->write(block, e->buffer);
            e->dirty = false;
        }
        
    }
    // error otherwise?
}

void *ConcreteBlockCache::acquire(unsigned block)
{
    Entry *e = findEntry(block);
    
    if (e)
    {
        incrementCount(e);
        return e->buffer;
    }
    
    // returns a new entry, not in hash table, not in free list.
    e = newEntry(block);
    
    _device->read(block, e->buffer);
    
    addEntry(e);
    
    return e->buffer;
}

unsigned ConcreteBlockCache::hashFunction(unsigned block)
{
    return block % HashTableSize;
}

/*
 * remove a block from the hashtable
 * and write to dick if dirty.
 */
void ConcreteBlockCache::removeEntry(unsigned block)
{
    Entry *e;
    Entry *prev;
    unsigned hash = hashFunction(block);
    
    e = _hashTable[hash];
    if (!e) return;
    
    // head pointer, special case.
    if (e->block == block)
    {
        _hashTable[hash] = e->nextHash;
        e->nextHash = NULL;
        
        return;
    }
    
    for(;;)
    {
        prev = e;
        e = e->next;
        
        if (!e) break;
        if (e->block == block)
        {
            prev->nextHash = e->nextHash;
            e->nextHash = NULL;
            
            break;
        }    
    }
}


void ConcreteBlockCache::addEntry(Entry *e)
{
    unsigned hash = hashFunction(e->block);
    
    e->nextHash = _hashTable[hash];
    _hashTable[hash] = e;
}

// increment the count and remove from the free list
// if necessary.
void ConcreteBlockCache::incrementCount(Entry *e)
{

    if (e->count == 0)
    {
        Entry *prev = e->prev;
        Entry *next = e->next;
        
        e->prev = e->next = NULL;

        if (prev) prev->next = next;
        if (next) next->prev = prev;
        
        if (_first == e) _first = next;
        if (_last == e) _last = prev;
    }
    
    e->count = e->count + 1;
    
}

// decrement the count.  If it goes to 0,
// add it as the last block entry.
void ConcreteBlockCache::decrementCount(Entry *e)
{
    e->count = e->count - 1;
    if (e->count == 0)
    {
        setLast(e);
    }
}

ConcreteBlockCache::Entry *ConcreteBlockCache::newEntry(unsigned block)
{
    Entry *e;
    
    if (_first)
    {
        e = _first;
        if (_first == _last)
        {
            _first = _last = NULL;
        }
        else
        {
            _first = e->next;
            _first->prev = NULL; 
        }


        if (e->dirty)
        {
            _device->write(e->block, e->buffer);
            e->dirty = false;
        }
        removeEntry(e->block);
    }
    else
    {
        e = new Entry;
        _buffers.push_back(e);
    }
    
    e->next = NULL;
    e->prev= NULL;
    e->nextHash = NULL;
    e->count = 1;
    e->block = block;
    e->dirty = false;

    return e;
}


void ConcreteBlockCache::setLast(Entry *e)
{
    if (_last == NULL)
    {
        _first = _last = e;
        return;
    }
    
    e->prev = _last;
    _last->next = e;
    _last = e;
}


void ConcreteBlockCache::setFirst(Entry *e)
{
    if (_first == NULL)
    {
        _first = _last = e;
        return;
    }
    
    e->next = _first;
    _first->prev = e;
    _first = e;
}

