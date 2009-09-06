#include "Bitmap.h"

#include <cstring>

namespace ProDOS {



/*
 * 
 *
 *
 */

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

inline static unsigned countLeadingZeros(uint8_t x)
{
    if (x == 0) return sizeof(uint8_t);
    #ifdef __GNUC__
    return __builtin_clz(x) + sizeof(uint8_t) - sizeof(unsigned);
    #endif    
    
    unsigned rv = 0;
    if ((x & 0xf0) == 0) { x <<= 4; rv = 4; }
    if (x & 0x80) return rv;
    if (x & 0x40) return rv + 1;
    if (x & 0x20) return rv + 2;
    if (x & 0x10) return rv + 3;
    // unreachable
    return 0;
}

inline static unsigned countLeadingOnes(uint8_t x)
{
    return countLeadingZeros(~x);
}

Bitmap::Bitmap(unsigned blocks) :
    _blocks(blocks), 
    _freeBlocks(blocks),
    _bitmapSize((blocks + 4096 - 1) >> 12),
    _bitmap(NULL)
{
    // 1 block = 512 bytes = 4096 bits 
    _bitmap = new uint8_t[_bitmapSize];

    // mark every requested block as free.
    unsigned bytes = blocks >> 3;
    std::memset(_bitmap, 0xff, bytes);

    // all trailing blocks are marked as used.
    std::memset(_bitmap + bytes, 0x00, _bitmapSize - bytes);
    
    // and handle the edge case...
    /*
     * 0 -> 0 0 0 0  0 0 0 0
     * 1 -> 1 0 0 0  0 0 0 0
     * 2 -> 1 1 0 0  0 0 0 0
     * 3 -> 1 1 1 0  0 0 0 0
     * ...
     * 7 -> 1 1 1 1  1 1 1 0
     */
     //unsigned tmp = (1 << (8 - (blocks & 0x07))) - 1;
     //_bitmap[bytes] = ~tmp & 0xff;

     _bitmap[bytes] = 0 - (1 << (8 - (blocks & 0x07)));

    //_bitmap[bytes] = (0xff00 >> (blocks & 0x07)) & 0xff;
}

Bitmap::~Bitmap()
{
    delete[] _bitmap;
}


bool Bitmap::markBlock(unsigned block, bool inUse)
{
    if (block >= _blocks) return false;

    unsigned index = BlockIndex(block);
    unsigned mask = BlockMask(block);
    uint8_t data = _bitmap[index];

    _freeBlocks -= popCount(data);
    
    if (inUse) data &= ~mask;
    else data |= mask;

    _bitmap[index] = data;
    _freeBlocks += popCount(data);
    
    return true;
}

// find the first block starting from (and including) the
// startingBlock

int Bitmap::firstFreeBlock(unsigned startingBlock) const
{
    if (startingBlock >= _blocks) return -1;
    if (!_freeBlocks) return -1;
    
    unsigned index = BlockIndex(startingBlock);
    unsigned bit = startingBlock & 0x0f;    
    unsigned bytes = (_blocks + 7) >> 3;
    
    // special case for first (partial) bitmap
    if (bit != 0)
    {
        uint8_t data = _bitmap[index];
        unsigned mask = BlockMask(startingBlock);
        // 0 0 1 0 0 0 0 0 -> 0 0 1 1 1 1 1 1
        mask = (mask - 1) | mask;
        data &= mask; 
        if (data) return (index << 3) + countLeadingZeros(data);
        ++index;
    }
    
    for ( ; index < bytes; ++index)
    {
        uint8_t data = _bitmap[index];
        if (!data) continue;
        
        return (index << 3) + countLeadingZeros(data);
    }

    return -1;
}


// count the number of unused blocks.... (including startingBlock)
int Bitmap::countUnusedBlocks(unsigned startingBlock, unsigned maxSearch) const
{
    if (startingBlock >= _blocks) return -1;
    if (!_freeBlocks) return -1;

    unsigned count = 0;

    unsigned index = BlockIndex(startingBlock);
    unsigned bit = startingBlock & 0x0f;
    
    unsigned bytes = (_blocks + 7) >> 3;
    
    
    // special case for the first (partial) byte.    
    if (bit)
    {
        uint8_t data = _bitmap[index];
        if (data == 0) return 0;
        unsigned mask = BlockMask(startingBlock);
        // 0 0 1 0 0 0 0 0 -> 1 1 0 0 0 0 0 0
        mask = ~ ((mask - 1) | mask);
        data = data | mask;
        
        if (data == 0xff) 
        {
            count = 8 - bit;
            if (count >= maxSearch) return count;
            ++index;
        }
        
        else
        {
            return countLeadingOnes(data) - bit;
        }   
    }
    
    
    
    for ( ; index < bytes; ++index)
    {
        uint8_t data = _bitmap[index];
        
        // no free blocks = end search
        if (data == 0) break;
        
        // all free = continue (if necessary)
        if (data == 0xff)
        {
            count += 8;
            if (count >= maxSearch) break;
            continue;
        }
        
        // otherwise, add on any leading free and terminate.
        count += countLeadingOnes(data);
        break;  
    }

    return count;


}

// finds the first free block (with a possible range).
int Bitmap::freeBlock(unsigned count) const
{
    if (count == 0 || count > freeBlocks()) return -1;
    
    // we could keep a linked list/etc of
    // free ranges
    // for now, scan the entire bitmap.


    int startBlock = 0;
    --count;
    
    for(;;)
    {
        startBlock = firstFreeBlock(startBlock);
        if (startBlock < 0) return -1;
    
        if (count == 0) return startBlock;
        
        int tmp = countUnusedBlocks(startBlock + 1, count);
        if (tmp <= 0) break;
        if (tmp >= count) return startBlock;

        // miss ... jump ahead.
        startBlock += tmp + 1;
    }
    return -1;
}

} // namespace

