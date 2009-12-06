
#include <cstring>
#include <memory>

#include "Bitmap.h"
#include "BlockDevice.h"
#include "Buffer.h"
#include "Endian.h"
#include "Entry.h"
#include "Exception.h"

using namespace ProFUSE;
using namespace LittleEndian;


#pragma mark VolumeDirectory


VolumeDirectory *VolumeDirectory::Create(const char *name, BlockDevice *device)
{
    return new VolumeDirectory(name, device);
}

VolumeDirectory *VolumeDirectory::Create(BlockDevice *device)
{
    uint8_t block[512];
    // always block 2.
    
    device->read(2, block);
    
    return new VolumeDirectory(device, block);
}




VolumeDirectory::VolumeDirectory(const char *name, BlockDevice *device) :
    Directory(VolumeHeader, name)
{    
    _totalBlocks = device->blocks();
    _bitmapPointer = 6;
    
    _entryBlocks.push_back(2);
    _entryBlocks.push_back(3);
    _entryBlocks.push_back(4);
    _entryBlocks.push_back(5);
    
    std::auto_ptr<Bitmap> bitmap(new Bitmap(_totalBlocks));
    
    Buffer buffer(512);
    
    // 2 bootcode blocks
    for (unsigned i = 0; i < 2; ++i)
    {
        bitmap->allocBlock(i);
        device->zeroBlock(i);
    }
    
    //4 volume header blocks.
    for (unsigned i = 2; i < 6; ++i)
    {
        bitmap->allocBlock(i);
        
        buffer.clear();
        // prev block, next block
        buffer.push16le(i == 2 ? 0 : i - 1);
        buffer.push16le(i == 5 ? 0 : i + 1);
        
        if (i == 2)
        {
            // create the volume header.
            // all ivars must be set.
            write(&buffer);
        }
        
        buffer.resize(512);
        device->write(i, buffer.buffer());
    }

    
    // allocate blocks for the bitmap itself
    unsigned bb = bitmap->bitmapBlocks();    
    for (unsigned i = 0; i < bb; ++i)
        bitmap->allocBlock(_bitmapPointer + i);
    
    // now write the bitmap...
    const uint8_t *bm = (const uint8_t *)bitmap->bitmap();
    for (unsigned i = 0; i < bb; ++i)
    {
        device->write(_bitmapPointer + i, 512 * i + bm);
    }
    
    _device = device;
    _bitmap = bitmap.release();
}


VolumeDirectory::VolumeDirectory(BlockDevice *device, const void *bp) :
    Directory(bp),
    _modification(0,0)
{
#undef __METHOD__
#define __METHOD__ "VolumeDirectory::VolumeDirectory"

    // + 4 to skip over the block poitners.
    std::auto_ptr<Bitmap> bitmap;
    const void *vp = 4 + (const uint8_t *)bp;
 
 
    if (storageType() != VolumeHeader)
        throw ProDOSException(__METHOD__ ": Invalid storage type.", 0x4b);
        
        
    
    _modification = DateTime(Read16(vp, 0x12), Read16(vp, 0x14));

    _bitmapPointer = Read16(vp, 0x23);

    _totalBlocks = Read16(vp, 0x25);
    

    // verify totalBlocks <=  device->blocks() ?
    
    if (_bitmapPointer >= _totalBlocks)
        throw ProDOSException(__METHOD__ ": Invalid bitmap pointer.", 0x5a);


    // bitmap pointer...
    bitmap.reset(new Bitmap(device, _bitmapPointer, _totalBlocks));


    // parse the directory header....

    _bitmap = bitmap.release();
}

VolumeDirectory::~VolumeDirectory()
{
    if (_device)
    {
        _device->sync();
        delete _device;
    }
    delete _bitmap;
}

void VolumeDirectory::write(Buffer *out)
{
    out->push8((VolumeHeader << 4 ) | nameLength());
    out->pushBytes(namei(), 15);
    
    // reserved.  SOS uses 0x75 for the first byte [?] 
    out->push8(0);
    out->push8(0);
    
    // last mod
    out->push16le(_modification.date());
    out->push16le(_modification.time());
    
    // filename case bits
    out->push16le(caseFlag());
    
    // creation
    out->push16le(creation().date());
    out->push16le(creation().time());    

    out->push8(version());
    out->push8(minVersion());
    

    out->push8(access());
    out->push8(entryLength());
    out->push8(entriesPerBlock());
    out->push16le(fileCount());
    
    out->push16le(_bitmapPointer);
    out->push16le(_totalBlocks);
}
