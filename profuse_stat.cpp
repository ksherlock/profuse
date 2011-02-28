/*
 *  prodos_stat.cpp
 *  profuse
 *
 *  Created by Kelvin Sherlock on 1/23/2009.
 *
 */


#pragma mark Stat Functions

#include "profuse.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <vector>

#include <sys/stat.h>
#include <sys/statvfs.h>

using std::vector;

int prodos_stat(FileEntry& e, struct stat *st)
{
    uint8_t buffer[BLOCK_SIZE];
    int ok;
    
    
    if (e.storage_type == EXTENDED_FILE)
    {
        ok = disk->Normalize(e, 0);
        if (ok < 0) return ok;
    }
    
    st->st_blksize = BLOCK_SIZE;
    
    st->st_ctime = e.creation;
#ifdef HAVE_STAT_BIRTHTIME
    st->st_birthtime = e.creation;
#endif
    
    st->st_mtime = e.last_mod;
    st->st_atime = e.last_mod;
    
    
    st->st_nlink = 1;
    st->st_mode = 0444 | S_IFREG;
    st->st_size = e.eof;    
    
    
    if (e.storage_type == DIRECTORY_FILE)
    {
        ok = disk->Read(e.key_pointer, buffer);
        if (ok < 0) return -1;
        
        SubdirEntry se;
        se.Load(buffer + 0x04);            
        
        if (se.storage_type != SUBDIR_HEADER) return -1;
        
        st->st_mode = S_IFDIR | 0555;
        st->st_size = BLOCK_SIZE;
        st->st_nlink = se.file_count + 1;
        
        return 0;
    }
    
    
    switch(e.storage_type)
    {
        case SEEDLING_FILE:
        case SAPLING_FILE:
        case TREE_FILE:
            //case PASCAL_FILE:
            break;
            
        default:
            return -1;
    }
    
    return 0;
}

int prodos_stat(const VolumeEntry &v, struct stat *st)
{
    
    if (v.storage_type != VOLUME_HEADER) return -1;
    
    st->st_mode = S_IFDIR | 0555;
    st->st_ctime = v.creation;
    
#ifdef HAVE_STAT_BIRTHTIME
    st->st_birthtime = v.creation;
#endif
    st->st_mtime = v.last_mod;
    st->st_atime = v.last_mod;
    
    st->st_nlink = v.file_count + 1;
    st->st_size = BLOCK_SIZE;
    st->st_blksize = BLOCK_SIZE;
    
    
    return 1;
}


void prodos_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    uint8_t buffer[BLOCK_SIZE];
    struct stat st;
    int ok;
    
    fprintf(stderr, "get_attr %u\n", (unsigned)ino);
    
    bzero(&st, sizeof(st));
    
    /*
     * ino 1 is the volume header.  Others are pointers.
     *
     */
    
    
    ok = disk->Read(ino == 1 ? 2 : ino >> 9, buffer);
    ERROR(ok < 0, EIO)
    
    // ino 1 is the volume header.
    if (ino == 1)
    {
        VolumeEntry v;
        v.Load(buffer + 0x04);
        ok = prodos_stat(v, &st);
        ERROR(ok < 0, EIO);
        
        st.st_ino = ino;
        
        fuse_reply_attr(req, &st, 0.0);
        return;
    }
    else 
    {
        
        
        FileEntry e;
        e.Load(buffer + (ino & 0x1ff));
        ok = prodos_stat(e, &st);
        
        ERROR(ok < 0, EIO);
        
        st.st_ino = ino;
        
        fuse_reply_attr(req, &st, 0.0); //
    }
}


// TODO -- add Disk::Lookup support so we don't have to parse the entire dir header.
// TODO -- add caching.
void prodos_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    uint8_t buffer[BLOCK_SIZE];
    struct fuse_entry_param entry;
    int ok;
    vector<FileEntry> files;
    
    
    fprintf(stderr, "lookup: %u %s\n", (unsigned)parent, name);
    
    ERROR(!validProdosName(name), ENOENT)
    
    ok = disk->Read(parent == 1 ? 2 : parent >> 9, buffer);
    ERROR(ok < 0, EIO)
    
    bzero(&entry, sizeof(entry));
    
    entry.attr_timeout = 0.0;
    entry.entry_timeout = 0.0;
    
    // get the file list
    // TODO -- Disk::look-up-one-file
    if (parent == 1)
    {
        VolumeEntry v;
        ok = disk->ReadVolume(&v, &files);
        ERROR(ok < 0, EIO)
    }
    else
    {
        FileEntry e;
        e.Load(buffer + (parent & 0x1ff));
        ERROR(e.storage_type != DIRECTORY_FILE, ENOENT);
        
        ok = disk->ReadDirectory(e.key_pointer, NULL, &files);
        ERROR(ok < 0, EIO)
    }
    // ok, now go through the file list and look for a (case insensitive) match.
    
    
    ok = -1;
    unsigned name_length = strlen(name);
    
    for(vector<FileEntry>::iterator iter = files.begin(); iter != files.end(); ++iter)
    {
        FileEntry& f = *iter;
        if ( (f.name_length == name_length) && (strcasecmp(name, f.file_name) == 0))
        {
            ok = prodos_stat(f, &entry.attr);
            fprintf(stderr, "stat %s %x (%x %x) %d\n", f.file_name, f.address, f.address >> 9, f.address & 0x1ff, ok);
            entry.ino = f.address; 
            entry.attr.st_ino = f.address;
            break;
        }
    }
    
    ERROR(ok < 0, ENOENT);
    
    fprintf(stderr, "file found!\n");
    
    fuse_reply_entry(req, &entry);
}




void prodos_statfs(fuse_req_t req, fuse_ino_t ino)
{    
    struct statvfs vst;
    
    VolumeEntry volume;
    
    
    disk->ReadVolume(&volume, NULL);

    // returns statvfs for the mount path or any file in the fs
    // therefore, ignore ino.
    std::memset(&vst, 0, sizeof(vst));
    
    vst.f_bsize = 512;  // fs block size
    vst.f_frsize = 512; // fundamental fs block size
    vst.f_blocks = volume.total_blocks;
    vst.f_bfree = 0; // free blocks
    vst.f_bavail = 0; // free blocks (non-root)
    vst.f_files = 0; // ?
    vst.f_ffree = -1;  // free inodes.
    vst.f_favail = -1; // free inodes (non-root)
    vst.f_fsid = 0; // file system id?
    vst.f_flag = ST_RDONLY | ST_NOSUID;
    vst.f_namemax = 15;
    
    fuse_reply_statfs(req, &vst);    
}

