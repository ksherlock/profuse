
#include <algorithm>
#include <cerrno>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <Device/BlockDevice.h>
#include <Device/BlockCache.h>

#include <ProFUSE/Exception.h>
#include <ProFUSE/auto.h>


/*
 * Note -- everything is assumed to be single-threaded.
 *
 */



using namespace Device;

using ProFUSE::Exception;
using ProFUSE::POSIXException;

#pragma mark -
#pragma mark BlockCache

BlockCache::~BlockCache(BlockDevice *device)
{
    _device = _device;
    _blocks = device->blocks();
    _readOnly = device->readOnly();
}

BlockCache::~BlockCache()
{
    delete _device;
}

#pragma mark -
#pragma mark MappedBlockCache

MappedBlockCache::MappedBlockCache(void *data, unsigned blocks)
{
    _blocks = blocks;
    _data = (uint8_t *)data;
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
    
    // msync must be page-size aligned.
    unsigned pagesize = ::getpagesize();
    unsigned offset = block * 512;
    void *address = _data + offset / pagesize * pagesize;
    unsigned length = offset % pagesize + 512;
    
    if (::msync(address, length, MS_ASYNC) < 0)
    {
        throw POSIXException(__METHOD__ ": msync failed.", errno);
    }
}


#pragma mark -
#pragma mark ConcreteBlockCache

typedef std::vector<BlockCache::Entry *>::iterator EntryIter;



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


ConcreteBlockCache::sync()
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


void ConcreteBlockCache::markDirty(unsigned block)
{
    Entry *e = findEntry(block);
    
    if (e) e->dirty = true;
    // error otherwise?
}


void ConcreteBlockCache::release(unsigned block, bool dirty)
{
    Entry *e = findEntry(block);
    
    if (e)
    {
        if (dirty) e->dirty = true;
        
        decrementCount(e);
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

    unsigned hash = hashFunction(block);
    
    // returns a new entry, not in hash table, not in free list.
    e = newEntry(block);
    
    _device->read(block, e->buffer);
    
    e->nextHash = _hashTable[hash];
    _hashTable[hash] = e;
    
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
void removeEntry(unsigned block)
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
            
            if (e->dirty)
            {
                _device->write(e->block, e->buffer);
                e->dirty = false;
            }
            
            break;
        }    
    }
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
        if (_last == NULL)
        {
            _first = _last = e;
            return;
        }
        
        e->prev = _last;
        _last = e;
    }
}

Entry *ConcreteBlockCache::newEntry(unsigned block)
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