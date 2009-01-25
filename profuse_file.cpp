/*
 *  profuse_file.cpp
 *  profuse
 *
 *  Created by Kelvin Sherlock on 1/23/2009.
 *
 */

#include "profuse.h"
#include <errno.h>


#pragma mark Read Functions

void prodos_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    fprintf(stderr, "open: %u\n", ino);
    
    
    uint8_t buffer[BLOCK_SIZE];
    int ok;
    
    FileEntry *e = NULL;
    
    ERROR(ino == 1, EISDIR)
    
    ok = disk->Read(ino >> 9, buffer);
    ERROR(ok < 0, EIO)
    
    e = new FileEntry();
    e->Load(buffer + (ino & 0x1ff));
    
    if (e->storage_type == EXTENDED_FILE)
    {
        ok = disk->Normalize(*e, 0);
        
        if (ok < 0)
        {
            delete e;
            ERROR(true, EIO)
        }
    }
    
    // EXTENDED_FILE already handled (it would be an error here.)
    switch(e->storage_type)
    {
        case SEEDLING_FILE:
        case SAPLING_FILE:
        case TREE_FILE:
            break;
            //case PASCAL_FILE: //?
        case DIRECTORY_FILE:
            delete e;
            ERROR(true, EISDIR)
            break;
        default:
            ERROR(true, EIO)
    }
    
    if ( (fi->flags & O_ACCMODE) != O_RDONLY)
    {
        delete e;
        ERROR(true, EACCES);
    }
    fi->fh = (uint64_t)e;
    
    fuse_reply_open(req, fi);
}

void prodos_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    fprintf(stderr, "release: %d\n", ino);
    
    FileEntry *e = (FileEntry *)fi->fh;
    
    if (e) delete e;
    
    fuse_reply_err(req, 0);
    
}

void prodos_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    fprintf(stderr, "read: %u %u %u\n", ino, size, off);
    
    FileEntry *e = (FileEntry *)fi->fh;
    
    ERROR(e == NULL, EIO)
    
    if (off >= e->eof)
    {
        fuse_reply_buf(req, NULL, 0);
        return;
    }
    
    
    unsigned level = 0;
    switch(e->storage_type)
    {
        case TREE_FILE:
            level = 2;
            break;
        case SAPLING_FILE:
            level = 1;
            break;
        case SEEDLING_FILE:
            level = 0;
            break;
    }
    
    // currently, reading is done on a block basis.
    // experimentally, fuse reads the entire file
    // this may not hold for larger files.
    
    
    // TODO -- error if size + off > eof.
    
    unsigned blocks = (size + (off & 0x1ff) + BLOCK_SIZE - 1) >> 9;
    int ok;
    uint8_t *buffer = new uint8_t[blocks << 9];
    
    fprintf(stderr, "ReadIndex(%x, buffer, %x, %x, %x)\n", e->key_pointer, level, (int)off, (int)blocks);
    
    ok = disk->ReadIndex(e->key_pointer, buffer, level, off, blocks);
    if (ok < 0)
    {
        fuse_reply_err(req, EIO);
    }
    else
    {
        fuse_reply_buf(req, (const char *)buffer + (off & 0x1ff), size);
    }
    
    delete []buffer;
}
