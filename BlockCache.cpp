#include "BlockCache.h"

struct FileBlockCache::Entry {
    FileBlockCache::Entry *next;
    unsigned block;
    unsigned count;
    bool dirty;
    uint8_t buffer[512];
};

FileBlockCache::FileBlockCache(Device *device)
{
    _device = device;
    
    // allocate initial buffer.
    for (unsigned i = 0; i < 10; ++i)
    {
        Entry *e = new Entry;
        std::memset(e, 0, sizeof(Entry));
    
        _pages.push_back(e);
        _unused.push_back(e);
    }
}

FileBlockCache::~FileBlockCache()
{
    std::vector<Entry *>::iterator iter;
    
    // todo -- check if dirty.
    
    // deallocate everything that was allocated. 
    for (iter = _pages.begin(); iter != _pages.end(); ++iter)
    {
        Entrty *e = *iter;
        if (e->dirty) sync(e);
        delete e;
    }
}



uint8_t *FileBlockCache::acquire(unsigned block)
{
    Entry *e = findEntry(block);
    
    if (e)
    {
        if (++e->count == 1)
            _unused.remove(e);
        return e->buffer;
    }
    
    if (_unused.empty())
    {
        e = new Entry;
        _pages.push_back(e);
    }
    else
    {
        e = _unused.pop_front();
        
        removeEntry(e);    
    }
    std::memset(e, 0, sizeof(Entry));
    unsigned hash = hashFunction(block);

    e->block = block;
    e->count = 1;
    e->dirty = 0;
    
    _device->read(block, e->buffer);
    
    _e->next = _hashMap[hash];
    _hashMap[hash] = e;

    return e->buffer;
}


void FileBlockCache::release(unsigned block, bool dirty)
{
    Entry *e = findEntry(block);
    
    // throw error?
    if (!e) return;
    
    if (dirty) e->dirty = true;
    
    if (e->count == 0) return;
    if (--e->count == 0)
    {
        _unused.push_back(e);
    }
    // sync if dirty??
}

void FileBlockCache::markDirty(unsigned block)
{
    Entry *e = findEntry(e);
    
    if (e && e->count) e->dirty = true;
}

Entry *FileBlockCache::findEntry(unsigned block)
{
    unsigned hash = hashFunction(block);
    Entry *e;
    
    e = _hashMap[hash];
    
    while (e && e->block != block)
    {
        e = e->next;
    }
    
    return e;
}

void FileBlockCache::removeEntry(Entry *e)
{
    unsigned hash;
    Entry *curr;
    Entry *prev;
    
    if (!e) return;
    
    hash = hashFunction(e->block);
    
    curr = _hashMap[hash];
    
    if (curr == e)
    {
        _hashMap[hash] = e->next;
        return;
    } 
    
    for (;;)
    {
        prev = curr;
        curr = curr->next;
    
        if (!curr) break;
        
        if (e == curr)
        {
            prev->next = e->next;
            return;
        }
    }
}

unsigned FileBlockCache::hashFunction(unsigned block)
{
    return block % HashEntries;
}

void FileBlockCache::sync(Entry *e)
{
    if (!e) return;
    if (!e->dirty) return;
    
    _device->write(e->block, e->buffer);
    e->dirty = false;
}

void FileBlockCache::sync()
{
    std::vector<Entry *>::iterator iter;
    
    for (iter = _pages.begin(); iter != _pages.end(); ++iter)
    {
        Entry *e = *iter;
        
        if (e && e->dirty) sync(e);
    }
}
