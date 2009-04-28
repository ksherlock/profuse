/*
 *  profuse_xattr.cpp
 *  profuse
 *
 *  Created by Kelvin Sherlock on 1/23/2009.
 *
 */


#include "profuse.h"

#include <algorithm>
#include <string>
#include <errno.h>

using std::string;

#pragma mark XAttribute Functions


static void xattr_filetype(FileEntry& e, fuse_req_t req, size_t size, off_t off)
{
    uint8_t attr = e.file_type;
    unsigned attr_size = 1;
    
    if (size == 0)
    {
        fuse_reply_xattr(req, attr_size);
        return;
    }
    
    ERROR (size < attr_size, ERANGE)
    
    // consider position here?
    fuse_reply_buf(req, (char *)&attr, attr_size);    
}


static void xattr_auxtype(FileEntry& e, fuse_req_t req, size_t size, off_t off)
{
    uint8_t attr[2];
    unsigned attr_size = 2;
    
    attr[0] = e.aux_type & 0xff;
    attr[1] = (e.aux_type >> 8) & 0xff;
    
    if (size == 0)
    {
        fuse_reply_xattr(req, attr_size);
        return;
    }
    
    ERROR (size < attr_size, ERANGE)
    
    // consider position here?
    fuse_reply_buf(req, (char *)&attr, attr_size);    
}

static void xattr_textencoding(FileEntry& e, fuse_req_t req, size_t size, off_t off)
{
    // TODO -- ascii text encoding for ascii files?
    const char attr[] = "MACINTOSH;0";
    unsigned attr_size = sizeof(attr) - 1;
    
    // currently only valid for Teach Files, "ASCII" Text
    
    if (e.file_type == 0x04 || (e.file_type == 0x50 && e.aux_type == 0x5445)) 
    /* do nothing */ ;
    else 
    {
        ERROR(true, ENOENT)
    }
    
    if (size == 0)
    {
        fuse_reply_xattr(req, attr_size);
        return;
    }
    
    ERROR (size < attr_size, ERANGE)
    
    // consider position here?
    fuse_reply_buf(req, (char *)&attr, attr_size);    
}

static void xattr_rfork(FileEntry& e, fuse_req_t req, size_t size, off_t off)
{
    int ok;
    unsigned level;
    
    ERROR (e.storage_type != EXTENDED_FILE, ENOENT)
    
    ok = disk->Normalize(e, 1);
    ERROR(ok < 0, EIO)
    
    
    switch(e.storage_type)
    {
        case SEEDLING_FILE:
            level = 0;
            break;
        case SAPLING_FILE:
            level = 1;
            break;
        case TREE_FILE:
            level = 2;
            break;
        default:
            ERROR(true, EIO)
    }
    
    if (size == 0)
    {
        fuse_reply_xattr(req, e.eof);
        return;
    }
    
    size = std::min((uint32_t)(size + off), e.eof);
    
    unsigned blocks = (size + (off & 0x1ff) + BLOCK_SIZE - 1) >> 9;
    uint8_t *buffer = new uint8_t[blocks << 9];
    
    fprintf(stderr, "ReadIndex(%x, buffer, %x, %x, %x)\n", e.key_pointer, level, (int)off, (int)blocks);
    
    ok = disk->ReadIndex(e.key_pointer, buffer, level, off, blocks);
    
    if (ok < 0)
    {
        fuse_reply_err(req, EIO);
    }
    else
    {
        fuse_reply_buf(req, (char *)buffer + (off & 0x1ff), size);
    }
    delete []buffer;
    return;
}

static void set_creator(uint8_t *finfo, unsigned ftype, unsigned auxtype)
{
    finfo[0] = 'p';
    finfo[1] = ftype;
    finfo[2] = auxtype >> 8;
    finfo[3] = auxtype;
    finfo[4] = 'p';
    finfo[5] = 'd';
    finfo[6] = 'o';
    finfo[7] = 's';    
}

// Finder info.
static void xattr_finfo(FileEntry& e, fuse_req_t req, size_t size, off_t off)
{
    int ok;
    ExtendedEntry ee;
    
    uint8_t attr[32];
    unsigned attr_size = 32;
    
    //ERROR (e.storage_type != EXTENDED_FILE, ENOENT)
    
    switch (e.storage_type)
    {
        case SEEDLING_FILE:
        case SAPLING_FILE:
        case TREE_FILE:
            bzero(attr, attr_size);
            set_creator(attr, e.file_type, e.aux_type);
            fuse_reply_buf(req, (char *)attr, attr_size);
            return;
        
        case EXTENDED_FILE:
            // handled below.
            break;

        default:
            ERROR(true, ENOENT);
    }
    
    
    ok = disk->Normalize(e, 1, &ee);
    ERROR(ok < 0, EIO)
    
    // sanity check
    switch(e.storage_type)
    {
        case SEEDLING_FILE:
        case SAPLING_FILE:
        case TREE_FILE:
            break;
        default:
            ERROR(true, EIO)
    }
    
    if (size == 0)
    {
        fuse_reply_xattr(req, attr_size);
        return;
    }
    
    ERROR (size < attr_size, ERANGE)
    
    memcpy(attr, ee.FInfo, 16);
    memcpy(attr + 16, ee.xFInfo, 16);

    // if no creator, create one.
    if (memcmp(attr, "\0\0\0\0\0\0\0\0", 8) == 0)
        set_creator(attr, e.file_type, e.aux_type);
    
    fuse_reply_buf(req, (char *)attr, attr_size);
}


static bool isTextFile(unsigned ftype, unsigned auxtype)
{
    if (ftype == 0x04) return true; // ascii text
    if (ftype == 0x80) return true; // source code.
    if (ftype == 0x50 && auxtype == 0x5445) return true; // teach text
    
    return false;
}


void prodos_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size)
{
    // list of supported attributes.
    //
#define NO_ATTR() \
{ \
if (size) fuse_reply_buf(req, NULL, 0); \
else fuse_reply_xattr(req, 0); \
return; \
}
    
    fprintf(stderr, "listxattr %u\n", ino);
    
    uint8_t buffer[BLOCK_SIZE];
    int ok;
    unsigned attr_size;
    string attr;
    
    
    
    if(ino == 1)
        NO_ATTR()
        
        ok = disk->Read(ino >> 9, buffer);
    
    ERROR(ok < 0, EIO)
    
    
    FileEntry e;
    e.Load(buffer + (ino & 0x1ff));
    
    
    attr += "prodos.FileType";
    attr.append(1, 0);
    
    attr += "prodos.AuxType";
    attr.append(1, 0);
    
    switch(e.storage_type)
    {
        case EXTENDED_FILE:
        {
            // TODO -- pretend there's no resource fork if resource fork eof == 0 ?
            //
            //ok = disk->Normalize(e, 1);            
            //ERROR(ok < 0, EIO)
            
            attr += "prodos.ResourceFork"; 
            attr.append(1, 0);
            
            attr += "com.apple.FinderInfo";
            attr.append(1, 0);
            break;
        }
            
        case SEEDLING_FILE:
        case SAPLING_FILE:
        case TREE_FILE:
            // generate HFS creator codes.
            attr += "com.apple.FinderInfo";
            attr.append(1, 0);
            break;
            
        case DIRECTORY_FILE:
            NO_ATTR()
            break;
            
        default:
            
            NO_ATTR()
            break;
    }
    
    if (isTextFile(e.file_type, e.aux_type))
    {
        attr += "com.apple.TextEncoding";
        attr.append(1, 0);
    }
    
    attr_size = attr.length();
    
    fprintf(stderr, "%d %s\n", attr_size, attr.c_str());
    
    if (size == 0)
    {
        fuse_reply_xattr(req, attr_size);
        return;
    }
    
    if (size < attr_size)
    {
        fuse_reply_err(req, ERANGE);
        return;
    }
    
    fuse_reply_buf(req, attr.data(), attr_size);
    return;
}

/*
 * offset is only valid in OS X for the resource fork.
 *
 */
void prodos_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size, uint32_t off)
{
#ifdef __APPLE__ 
#define NO_ATTRIBUTE ENOENT
#else
#define NO_ATTRIBUTE EOPNOTSUPP 
#endif
    
    fprintf(stderr, "getxattr: %u %s %u %u \n", ino, name, (int)size, (int)off);
    
    uint8_t buffer[BLOCK_SIZE];
    
    
    ERROR(ino == 1, NO_ATTRIBUTE) // finder can't handle EISDIR.
    
    
    int ok = disk->Read(ino >> 9, buffer);
    
    ERROR(ok < 0, EIO)
    
    
    FileEntry e;
    e.Load(buffer + (ino & 0x1ff));
    
    switch(e.storage_type)
    {
        case SEEDLING_FILE:
        case SAPLING_FILE:
        case TREE_FILE:
        case EXTENDED_FILE:
            break;
        case DIRECTORY_FILE:
            ERROR(true, NO_ATTRIBUTE) // Finder can't handle EISDIR.
        default:
            ERROR(true, NO_ATTRIBUTE);
    }
    
    if (strcmp("prodos.FileType", name) == 0)
    {
        xattr_filetype(e, req, size, off);
        return;
    }
    
    if (strcmp("prodos.AuxType", name) == 0)
    {
        xattr_auxtype(e, req, size, off);
        return;
    }
    
    if (strcmp("com.apple.TextEncoding", name) == 0)
    {
        xattr_textencoding(e, req, size, off);
        return;        
    }
    
    if ( (e.storage_type == EXTENDED_FILE) && (strcmp("prodos.ResourceFork", name) == 0))   
    {
        xattr_rfork(e, req, size, off);
        return;          
    }
    
    if ( strcmp("com.apple.FinderInfo", name) == 0)   
    {
        xattr_finfo(e, req, size, off);
        return;          
    }
    
    
    fuse_reply_err(req, NO_ATTRIBUTE);
    
}
/*
 * Linux, et alia do not have an offset parameter.
 */
void prodos_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size)
{
    prodos_getxattr(req, ino, name, size, 0);
}

