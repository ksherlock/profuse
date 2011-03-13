#include <cstring>

#include <ProDOS/Bitmap.h>
#include <Device/BlockDevice.h>
#include <Cache/BlockCache.h>

using namespace ProDOS;

// returns # of 1-bits set (0-8)
inline static unsigned popCount(uint8_t x)
{
    #ifdef __GNUC__
    return __builtin_popcount(x);
    #endif
    
    // Brian Kernighan / Peter Wegner in CACM 3 (1960), 322.
    
    unsigned count;
    for (count = 0; x; ++count)
    {
        x &= x - 1;
    }
    return count;
}



Bitmap::Bitmap(unsigned blocks)
{
    _blocks = _freeBlocks = blocks;
    
    _bitmapBlocks = (blocks + 4095) / 4096;
    _freeIndex = 0;
    
    unsigned bitmapSize = _bitmapBlocks * 512;
    
    _bitmap.reserve(bitmapSize);
    
    // mark blocks as free.. 
    _bitmap.resize(blocks / 8, 0xff);
    
    // edge case...
    
    if (blocks & 0x0f)
    {
        _bitmap.push_back( ~(0xff >> (blocks & 0x0f)) );
    }
    
    // mark any trailing blocks as in use.
    
    _bitmap.resize(bitmapSize, 0x00);  
}

Bitmap::Bitmap(Device::BlockCache *cache, unsigned keyPointer, unsigned blocks)
{
    _blocks = blocks;
    _freeBlocks = 0;
    _freeIndex = 0;
    
    _bitmapBlocks = (blocks + 4095) / 4096;

    unsigned bitmapSize = _bitmapBlocks * 512;
    unsigned blockSize = blocks / 8;
    
    _bitmap.reserve(bitmapSize);
    

    // load the full block(s).
    for (unsigned i = 0; i < blockSize; ++i)
    {
        uint8_t *buffer = (uint8_t *)cache->acquire(keyPointer);
        
        _bitmap.insert(_bitmap.end(), buffer, buffer + 512);
        
        cache->release(keyPointer);
        
        keyPointer++;
    }
    
    // and any remaining partial block.
    
    if (blocks & 4095)
    {

        uint8_t *buffer = (uint8_t *)cache->acquire(keyPointer);

        unsigned bits = blocks & 4095;
        unsigned bytes = bits / 8;
                
        //for (unsigned i = 0; i < bits / 8; ++i) _bitmap.push_back(buffer[i]);
        
        _bitmap.insert(_bitmap.end(), buffer, buffer + bytes);
        // partial...
        
        if (blocks & 0x0f)
        {
            uint8_t tmp = buffer[bytes];
            tmp &= ~(0xff >> (blocks & 0x0f));
            
            _bitmap.push_back(tmp);
        }
        
        // remainder set to in use.
        _bitmap.resize(bitmapSize, 0x00);

        cache->release(keyPointer);
        
        keyPointer++;        
        
    }
    
    
    // now set _freeBlocks and _freeIndex;
    std::vector<uint8_t>::iterator iter;
    
    _freeIndex = -1;
    for (iter = _bitmap.begin(); iter != _bitmap.end(); ++iter)
    {
        _freeBlocks += popCount(*iter);
        if (_freeIndex == -1 && *iter) 
            _freeIndex = std::distance(_bitmap.begin(), iter); 
    }
}

Bitmap::~Bitmap()
{
}


void Bitmap::freeBlock(unsigned block)
{
    if (block >= _blocks) return;
    
    unsigned index = block / 8;
    unsigned offset = block & 0x07;
    unsigned mask = 0x80 >> offset;
    
    uint8_t tmp = _bitmap[index];
    
    if ((tmp & mask) == 0)
    {
        ++_freeBlocks;
        _bitmap[index] = tmp | mask;
    }
}


int Bitmap::allocBlock(unsigned block)
{
    if (block >= _blocks) return -1;
    
    
    unsigned index = block / 8;
    unsigned offset = block & 0x07;
    unsigned mask = 0x80 >> offset;
    
    uint8_t tmp = _bitmap[index];
    
    if ((tmp & mask))
    {
        --_freeBlocks;
        _bitmap[index] = tmp & ~mask;
        return block;
    }
    
    return -1;
}


int Bitmap::allocBlock()
{
    if (!_freeBlocks) return -1;
    
    unsigned freeIndex = _freeIndex;
    unsigned maxIndex = (_blocks + 7) / 8;


    for (unsigned index = _freeIndex; index < maxIndex; ++index)
    {
        uint8_t tmp = _bitmap[index];
        if (!tmp) continue;
        
        unsigned mask = 0x80;
        for (unsigned offset = 0; offset < 8; ++offset)
        {
            if (tmp & mask)
            {
                _freeIndex = index;
                _bitmap[index] = tmp & ~mask;
                --_freeBlocks;
                return index * 8 + offset;
            }
            mask = mask >> 1;
        }
    }

    for (unsigned index = 0; index < freeIndex; ++index)
    {
        uint8_t tmp = _bitmap[index];
        if (!tmp) continue;
        
        unsigned mask = 0x80;
        for (unsigned offset = 0; offset < 8; ++offset)
        {
            if (tmp & mask)
            {
                _freeIndex = index;
                _bitmap[index] = tmp & ~mask;
                --_freeBlocks;
                return index * 8 + offset;
            }
            mask = mask >> 1;
        }
    }

    
    // should never happen...
    return -1;
}