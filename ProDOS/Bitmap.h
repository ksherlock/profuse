#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdint.h>
#include <vector>


namespace Device
{
    class BlockDevice; 
    class BlockCache;
}


namespace ProDOS {


class Bitmap {
public:

    Bitmap(unsigned blocks);
    Bitmap(Device::BlockCache *cache, unsigned keyPointer, unsigned blocks);

    ~Bitmap();
    
    int allocBlock();
    int allocBlock(unsigned block);
    
    void freeBlock(unsigned block);
    
    
    unsigned freeBlocks() const { return _freeBlocks; }
    unsigned blocks() const { return _blocks; }
    unsigned bitmapBlocks() const { return _bitmapBlocks; }
    unsigned bitmapSize() const { return _bitmapBlocks * 512; }
    const void *bitmap() const { return &_bitmap[0]; }

private:

    unsigned _freeIndex;
    unsigned _freeBlocks;
    
    unsigned _blocks;
    unsigned _bitmapBlocks;
    
    std::vector<uint8_t> _bitmap;
};



}

#endif
