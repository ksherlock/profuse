#include "Bitmap.h"

#include <cstring>

namespace ProDOS {

/*
 * 
 *
 *
 */

Bitmap::Bitmap(unsigned blocks) :
    _bitmap(NULL), _blocks(blocks), 
    _bitmapSize((blocks + 4096 - 1) >> 12)
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

    if (inUse) data &= ~mask;
    else data |= mask;

    _bitmap[index] = data;

    return true;
}
} // namespace

