/*
 *  profuse_dirent.cpp
 *  profuse
 *
 *  Created by Kelvin Sherlock on 1/23/2009.
 *
 */

#include "profuse.h"

#include <errno.h>
#include <strings.h>

#include <algorithm>
#include <string>
#include <vector>

using std::string;
using std::vector;

#pragma mark Directory Functions

/*
 * when the directory is opened, we load the volume/directory and store the FileEntry vector into
 * fi->fh.
 *
 */
void prodos_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    fprintf(stderr, "opendir: %u\n", (unsigned)ino);
    // verify it's a directory/volume here?
    
    
    uint8_t buffer[BLOCK_SIZE];
    vector<FileEntry> files;
    bool ok;
    
    
    ok = disk->Read(ino == 1 ? 2 : ino >> 9, buffer);
    ERROR(ok < 0, EIO)
    
    
    if (ino == 1)
    {
        VolumeEntry v;
        v.Load(buffer + 0x04);
        
        ok = disk->ReadVolume(&v, &files);
        
        ERROR(ok < 0, EIO)        
    }
    else
    {
        
        FileEntry e;
        e.Load(buffer + (ino & 0x1ff));
        
        ERROR(e.storage_type != DIRECTORY_FILE, ENOTDIR)
        
        ok = disk->ReadDirectory(e.key_pointer, NULL, &files);
        
        ERROR(ok < 0, EIO);
    } 
    
    // copy the vector contents to a vector *.
    vector<FileEntry> *fp = new vector<FileEntry>();
    files.swap(*fp);
    
    fi->fh = (uint64_t)fp;
    fuse_reply_open(req, fi);
}

void prodos_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    fprintf(stderr,"releasedir: %u\n", (unsigned)ino);
    vector<FileEntry> *files = (vector<FileEntry> *)fi->fh;
    
    if (files) delete files;
    
    fuse_reply_err(req, 0);
}

void prodos_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    vector<FileEntry> *files = (vector<FileEntry> *)fi->fh;
    struct stat st;
    
    fprintf(stderr, "readdir %u %u %u\n", (unsigned)ino, (unsigned)size, (unsigned)off);
    
    // TODO -- add "." and ".." entries...
    
    
    // if the offset >= number of entries, get out.
    if (!files || files->size() <= off)
    {
        fprintf(stderr, "fuse_reply_buf(req, NULL, 0)\n");
        fuse_reply_buf(req, NULL, 0);
        return;
    }
    
    
    // now some dirent info...
    
    bzero(&st, sizeof(st));
    // only mode and ino are used.
    
    char *buffer = new char[size];
    
    unsigned count = files->size();
    unsigned current_size = 0;
    for (unsigned i = off; i < count; ++i)
    {
        FileEntry &f = (*files)[i];
        
        st.st_mode = f.storage_type == DIRECTORY_FILE ? S_IFDIR | 0555 : S_IFREG | 0444;
        st.st_ino = f.address;
        
        unsigned entry_size = fuse_add_direntry(req, NULL, 0, f.file_name, NULL, 0);
        if (entry_size + current_size >= size) break;
        
        
        fuse_add_direntry(req, (char *)buffer + current_size, size, f.file_name, &st, i + 1);
        current_size += entry_size;
        
    }
    
    fuse_reply_buf(req, buffer, current_size);    
    delete []buffer;
}

