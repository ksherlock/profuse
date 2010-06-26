#include <cstring>

#include <ProDOS/Bitmap.h>
#include <ProDOS/BlockDevice.h>
#include "auto.h"


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
    unsigned blockSize = blocks / 8;
    
    auto_array<uint8_t> bitmap(new uint8_t[bitmapSize]);

    // mark overflow in use, everything else free.

    std::memset(bitmap, 0xff, blocks / 8);
    std::memset(bitmap + blockSize, 0x00, bitmapSize - blockSize);
    
    // edge case
    unsigned tmp = blocks & 0x07;
    
    bitmap[blocks / 8] = ~(0xff >> tmp);
    
    _bitmap = bitmap.release();
    
    
}

Bitmap::Bitmap(BlockDevice *device, unsigned keyPointer, unsigned blocks)
{
    _blocks = blocks;
    _freeBlocks = 0;
    _freeIndex = 0;
    
    _bitmapBlocks = (blocks + 4095) / 4096;

    unsigned bitmapSize = _bitmapBlocks * 512;
    unsigned blockSize = blocks / 8;
    
    auto_array<uint8_t> bitmap(new uint8_t[bitmapSize]);

    for (unsigned i = 0; i < blockSize; ++i)
    {
        device->read(keyPointer + i, bitmap + 512 * i);
    }
    
    // make sure all trailing bits are marked in use.

    // edge case
    unsigned tmp = blocks & 0x07;
    
    bitmap[blocks / 8] &= ~(0xff >> tmp);

    std::memset(bitmap + blockSize, 0x00, bitmapSize - blockSize);

    // set _freeBlocks and _freeIndex;
    for (unsigned i = 0; i < (blocks + 7) / 8; ++i)
    {
        _freeBlocks += popCount(bitmap[i]);
    } 
    
    if (_freeBlocks)
    {
        for (unsigned i = 0; i < (blocks + 7) / 8; ++i)
        {
            if (bitmap[i])
            {
                _freeIndex = i;
                break;
            }
        }
    }

    
    _bitmap = bitmap.release();
}

Bitmap::~Bitmap()
{
    if (_bitmap) delete []_bitmap;
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