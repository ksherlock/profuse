/*
 *  Disk.cpp
 *  ProFUSE
 *
 *  Created by Kelvin Sherlock on 12/18/08.
 *
 */

#include "Disk.h"


#include "common.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <set>
#include <vector>

#include <Endian/Endian.h>

struct ucmp
{
    bool operator()(unsigned a, unsigned b) const
    {
        return a < b;
    }
};

using std::set;
using std::vector;

using namespace LittleEndian;

typedef set<unsigned, ucmp> uset;

Disk::Disk()
{
    _blocks = 0;

}

Disk::~Disk()
{
}

Disk::Disk(Device::BlockDevicePointer device) :
    _device(device)
{
    _blocks = _device->blocks();
}

DiskPointer Disk::OpenFile(Device::BlockDevicePointer device)
{
    DiskPointer disk(new Disk(device));
    
    return disk;
}

// load the mini entry into the regular entry.
int Disk::Normalize(FileEntry &f, unsigned fork, ExtendedEntry *ee)
{
    uint8_t buffer[BLOCK_SIZE];
    int ok;
    
    if (fork > 1) return -P8_INVALID_FORK;
    
    if (f.storage_type != EXTENDED_FILE)
    {
        return fork == 0 ? 0 : -P8_INVALID_FORK;
    }
    
    ok = Read(f.key_pointer, buffer);
    if (ok < 0) return ok;
    
    ExtendedEntry e;
    e.Load(buffer);
    
    if (fork == 0)
    {
        f.storage_type = e.dataFork.storage_type;
        f.key_pointer = e.dataFork.key_block;
        f.eof = e.dataFork.eof;
        f.blocks_used = e.dataFork.blocks_used;
    }
    else
    {
        f.storage_type = e.resourceFork.storage_type;
        f.key_pointer = e.resourceFork.key_block;
        f.eof = e.resourceFork.eof;
        f.blocks_used = e.resourceFork.blocks_used;
    }
    
    if (ee) *ee = e;
    
    return 0;
 
}


int Disk::Read(unsigned block, void *buffer)
{

    if (block > _blocks) return -P8_INVALID_BLOCK;

    _device->read(block, buffer);
    
    return 1;
}


void *Disk::ReadFile(const FileEntry &f, unsigned fork, uint32_t *size, int *error)
{

#define SET_ERROR(x) if (error) *error = (x)
#define SET_SIZE(x) if (size) *size = (x)
    
    
    SET_ERROR(0);
    SET_SIZE(0);
        
    if (fork != P8_DATA_FORK && fork != P8_RESOURCE_FORK)
    {
        SET_ERROR(-P8_INVALID_FORK);
        return NULL;
    }
    
    uint8_t buffer[BLOCK_SIZE];
    int ok;
    uint32_t eof;
    uint32_t alloc;
    unsigned blocks;
    unsigned storage_type;
    unsigned key_block;

    switch(f.storage_type)
    {
        case SEEDLING_FILE:
        case SAPLING_FILE:
        case TREE_FILE:
            if (fork != P8_DATA_FORK)
            {
                SET_ERROR(1);
                return NULL;
            }
            storage_type = f.storage_type;
            eof = f.eof;
            key_block = f.key_pointer;
            break;
            
        case EXTENDED_FILE:
            {
                ok = Read(f.key_pointer, buffer);
                if (ok < 0)
                {
                   SET_ERROR(ok);
                    return NULL;
                }
                
                ExtendedEntry entry;
                entry.Load(buffer);
                
                if (fork == P8_DATA_FORK)
                {
                    storage_type = entry.dataFork.storage_type;
                    eof = entry.dataFork.eof;
                    key_block = entry.dataFork.key_block;
                }
                else
                {
                    storage_type = entry.resourceFork.storage_type;
                    eof = entry.resourceFork.eof;
                    key_block = entry.resourceFork.key_block;            
                }
            }
            break;
        default:
            SET_ERROR(-P8_INVALID_STORAGE_TYPE);
            return NULL;
    }
    
    if (eof == 0)
    {
        SET_ERROR(1);
        return NULL;
    }
    
    blocks = (eof + BLOCK_SIZE - 1) >> 9;
    alloc = (eof + BLOCK_SIZE - 1) & (~BLOCK_SIZE);
    uint8_t* data = new uint8_t[alloc];
    
    
    switch (storage_type)
    {
        case SEEDLING_FILE:
            ok = Read(key_block, data);
            break;
        case SAPLING_FILE:
            ok = ReadIndex(f.key_pointer, buffer, 1, 0, blocks);
            break;
        case TREE_FILE:
            ok = ReadIndex(f.key_pointer, buffer, 2, 0, blocks);
            break;
            
        default:
        ok = false;
    }
    
    if (ok < 0)
    {
        SET_ERROR(ok);
        delete[] data;
        return NULL;
    }
    
    
    bzero(data + eof, alloc - eof);
    SET_SIZE(eof);
    
    return data;
}


int Disk::ReadFile(const FileEntry &f, void *buffer)
{
    int blocks = (f.eof + BLOCK_SIZE - 1) >> 9;
    int ok;
    
    switch(f.storage_type)
    {
        case TREE_FILE:
            ok = ReadIndex(f.key_pointer, buffer, 2, 0, blocks);
            break;
        case SAPLING_FILE:
            ok = ReadIndex(f.key_pointer, buffer, 1, 0, blocks);
            break;
        case SEEDLING_FILE:
            ok = Read(f.key_pointer, buffer);
            break;
                        
        default:
            return -P8_INVALID_STORAGE_TYPE;
    }
    
    if (ok >= 0)
    {
        bzero((uint8_t *)buffer + f.eof, (blocks << 9) - f.eof);
    }
    
    return ok;
}


int Disk::ReadIndex(unsigned block, void *buffer, unsigned level, off_t offset, unsigned blocks)
{    
    if (level == 0)
    {
        // data level
        if (block == 0) // sparse file
        {
            bzero(buffer, BLOCK_SIZE);
            return 1;
        }
        return Read(block, buffer);
    }
    
    
    unsigned blockCount;
    unsigned readSize;
    unsigned first;
    //unsigned last;
    
    
    switch(level)
    {
        case 1:
            first = (offset >> 9) & 0xff;
            blockCount = 1;
            readSize = BLOCK_SIZE;
            offset = 0;
            break;
        case 2:
            first = (offset >> 17) & 0xff;
            blockCount = 256;
            readSize = BLOCK_SIZE << 8;
            offset &= 0x1ffff;
            break;
        default:
            return -P8_INTERNAL_ERROR;
    }
    
    

    
    int ok;
    uint8_t key[BLOCK_SIZE];
    
    if (block) // not sparse.
    {
        ok = Read(block, key);
        if (ok < 0 ) return ok;
    }
    else
    {
        // sparse -- zero it out so code below works w/o special cases.
        bzero(key, BLOCK_SIZE);
    }
    
    for (unsigned i = first; blocks; i++)
    {
        // block pointers are split up since 8-bit indexing is limited to 256.
        unsigned newBlock = (key[i]) | (key[256 + i] << 8);
        
        unsigned b = std::min(blocks, blockCount);
        
        ok = ReadIndex(newBlock, buffer, level - 1, offset, b);
        if (ok < 0) return ok;
        offset = 0;
        buffer = ((char *)buffer) + readSize;
        blocks -= b;
    }
    return blocks;
}



int Disk::ReadVolume(VolumeEntry *volume, std::vector<FileEntry> *files)
{
    if (files) files->resize(0);
    
    uint8_t buffer[BLOCK_SIZE];
    int ok;
    unsigned prev;
    unsigned next;
    
    uset blocks; 

    unsigned block = 2;
    blocks.insert(block);
    ok = Read(block, buffer);
    
    if (ok < 0) return ok;
    
    prev = Read16(&buffer[0x00]);
    next = Read16(&buffer[0x02]);
    
    VolumeEntry v;
    v.Load(buffer + 0x04);
    
    if (v.storage_type != VOLUME_HEADER) return -P8_INVALID_STORAGE_TYPE;
    
    if (volume) *volume = v;
    
    if (!files) return 1;
    
    if (v.file_count)
    {
        files->reserve(v.file_count);
        //files->resize(v.file_count);
        
        //unsigned count = 0;
        unsigned index = 1; // skip the header.
        for(;;)
        {
            // 
            if ( (buffer[0x04 + v.entry_length * index] >> 4) != DELETED_FILE)
            {
                unsigned offset = v.entry_length * index + 0x4;
                FileEntry f;
                f.Load(buffer + offset);
                f.address = (block << 9) + offset;
                
                files->push_back(f);
                //if (++count == v.file_count) break;
            }
            index++;
            if (index >= v.entries_per_block)
            {                
                if (!next) break; // all done!
                

                if (blocks.insert(next).second == false)
                {
                    return -P8_CYCLICAL_BLOCK;
                }                
                
                ok = Read(next, buffer);
                if (ok < 0) return ok;
                block = next;
                
                prev = Read16(&buffer[0x00]);
                next = Read16(&buffer[0x02]);                

                index = 0;
            }
        }
    }
    
    return 1;
}


int Disk::ReadDirectory(unsigned block, SubdirEntry *dir, std::vector<FileEntry> *files)
{
    if (files) files->resize(0);
    
    uint8_t buffer[BLOCK_SIZE];
    int ok;
    unsigned prev;
    unsigned next;
    
    // keep a list of blocks to prevent cyclical problems.
    uset blocks; 
    
    blocks.insert(block);
    
    ok = Read(block, buffer);
    
    if (ok < 0) return ok;
    
    prev = Read16(&buffer[0x00]);
    next = Read16(&buffer[0x02]);
    
    SubdirEntry v;
    v.Load(buffer + 0x04);
    
    if (v.storage_type != SUBDIR_HEADER) return -P8_INVALID_STORAGE_TYPE;

    
    if (dir) *dir = v;
    
    if (!files) return 1;
    
    if (v.file_count)
    {
        files->reserve(v.file_count);
        //files->resize(v.file_count);
        
        //unsigned count = 0;
        unsigned index = 1; // skip the header.
        for(;;)
        {
            // 
            if ( (buffer[0x04 + v.entry_length * index] >> 4) != DELETED_FILE)
            {
                unsigned offset = v.entry_length * index + 0x4;
                FileEntry f;
                f.Load(buffer + offset);
                f.address = (block << 9) + offset;
                
                files->push_back(f);

                //if (++count == v.file_count) break;
            }
            index++;
            if (index >= v.entries_per_block)
            {                
                if (!next) break; // all done!
                
                
                if (blocks.insert(next).second == false)
                {
                    return -P8_CYCLICAL_BLOCK;
                }
                
                ok = Read(next, buffer);
                if (ok < 0) return ok;
                block = next;
                
                
                prev = Read16(&buffer[0x00]);
                next = Read16(&buffer[0x02]);                
                
                index = 0;
            }
        }  
    }
    
    return 1;
}
