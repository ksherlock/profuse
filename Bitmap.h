#ifndef __PRODOS_BITMAP_H__
#define __PRODOS_BITMAP_H__

#include <stdint.h>

namespace ProDOS {

class Bitmap {
public:
    Bitmap(unsigned blocks);
    ~Bitmap();

    bool blockFree(unsigned block) const;
    bool markBlock(unsigned block, bool inUse);

    unsigned blocks() const;
    unsigned bitmapBlocks() const;

private:
    static unsigned BlockMask(unsigned block);
    static unsigned BlockIndex(unsigned block);

    uint8_t *_bitmap;
    unsigned _blocks;
    unsigned _bitmapSize;
};

inline unsigned Bitmap::blocks() const
{
    return _blocks;
}

inline unsigned Bitmap::bitmapBlocks() const
{
    return _bitmapSize >> 12;
}

inline unsigned Bitmap::BlockMask(unsigned block)
{
    return 0x80 >> (block & 0x07);
}

inline unsigned Bitmap::BlockIndex(unsigned block)
{
    return block >> 3;
}

inline bool Bitmap::blockFree(unsigned block) const
{
    if (block >= _blocks) return false;
    return (_bitmap[BlockIndex(block)] & BlockMask(block)) != 0;
}

} // namespace

#endif

