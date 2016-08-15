/*
 *  profuse.h
 *  profuse
 *
 *  Created by Kelvin Sherlock on 1/23/2009.
 *
 */

#ifndef __PROFUSE_H__
#define __PROFUSE_H__

#include <ProDOS/File.h>
#include <ProDOS/Disk.h>
#include <ProDOS/common.h>


#ifdef __APPLE__
#define __DARWIN_64_BIT_INO_T 1 
#endif

#define _FILE_OFFSET_BITS 64 
#define FUSE_USE_VERSION 27

#include <fuse/fuse_opt.h>
#include <fuse/fuse_lowlevel.h>


#undef ERROR
#define ERROR(cond,errno) if ( (cond) ){ fuse_reply_err(req, errno); return; }


extern DiskPointer disk;

bool validProdosName(const char *name);

// xattr
void prodos_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size, uint32_t off);
void prodos_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size);
void prodos_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size);

//dirent
void prodos_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
void prodos_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
void prodos_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);

// stat
void prodos_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
void prodos_lookup(fuse_req_t req, fuse_ino_t parent, const char *name);

void prodos_statfs(fuse_req_t req, fuse_ino_t ino);


// file io.
void prodos_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
void prodos_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
void prodos_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);



#endif

