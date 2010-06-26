#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdint.h>



namespace ProDOS {

class BlockDevice;


class Bitmap {
public:

    Bitmap(unsigned blocks);
    Bitmap(BlockDevice *device, unsigned keyPointer, unsigned blocks);
    //todo -- constructor by loading from, block device...
    ~Bitmap();
    
    int allocBlock();
    int allocBlock(unsigned block);
    
    void freeBlock(unsigned block);
    
    
    unsigned freeBlocks() const { return _freeBlocks; }
    unsigned blocks() const { return _blocks; }
    unsigned bitmapBlocks() const { return _bitmapBlocks; }
    unsigned bitmapSize() const { return _bitmapBlocks * 512; }
    const void *bitmap() const { return _bitmap; }

private:

    unsigned _freeIndex;
    unsigned _freeBlocks;
    
    unsigned _blocks;
    unsigned _bitmapBlocks;
    
    uint8_t *_bitmap;
};



}

#endif
