/*
 *  main.cpp
 *  ProFUSE
 *
 *  Created by Kelvin Sherlock on 12/24/08.
 *
 */
/*

#define __FreeBSD__ 10
#define _FILE_OFFSET_BITS 64
#define __DARWIN_64_BIT_INO_T 1
#define _REENTRANT 
#define _POSIX_C_SOURCE 200112L
*/
#define FUSE_USE_VERSION 26



#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <ctype.h>


#include <vector>
#include <string>
#include <algorithm>

#include "common.h"
#include "Disk.h"
#include "File.h"


using std::vector;
using std::string;

Disk *disk = NULL;
char *dfile = NULL;
VolumeEntry volume;


#undef ERROR
#define ERROR(cond,errno) if ( (cond) ){ fuse_reply_err(req, errno); return; }



static bool validProdosName(const char *name)
{
    // OS X looks for hidden files that don't exist (and aren't legal prodos names)
    // most are not legal prodos names, so this filters them out easily.
    
    // [A-Za-z][0-9A-Za-z.]{0,14}
    
    if (!isalpha(*name)) return false;
    
    unsigned i;
    for(i = 1; name[i]; i++)
    {
        char c = name[i];
        if (c == '.' || isalnum(c)) continue;
        
        return false;
        
    }
    
    return i < 16;
}


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


// Finder info.
static void xattr_finfo(FileEntry& e, fuse_req_t req, size_t size, off_t off)
{
    int ok;
    ExtendedEntry ee;
    
    uint8_t attr[32];
    unsigned attr_size = 32;
    
    ERROR (e.storage_type != EXTENDED_FILE, ENOENT)
    
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
    
    fuse_reply_buf(req, (char *)attr, attr_size);
}



static void prodos_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size)
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

    
    FileEntry e(buffer + (ino & 0x1ff));
    

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
            break;
            
        case DIRECTORY_FILE:
            NO_ATTR()
            break;
            
        default:
            
            NO_ATTR()
            break;
    }
    
    if (
        e.file_type == 0x04 
        || (e.file_type == 0x50 && e.aux_type == 0x5445)) // teach text
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

// TODO -- more consistent position support.
static void prodos_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size, uint32_t off)
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

    
    FileEntry e(buffer + (ino & 0x1ff));
    
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
    
    if ( (e.storage_type == EXTENDED_FILE) && (strcmp("com.apple.FinderInfo", name) == 0))   
    {
        xattr_finfo(e, req, size, off);
        return;          
    }
    
    
    fuse_reply_err(req, NO_ATTRIBUTE);

}
/*
 * Linux, et alia do not have an offset parameter.
 */
static void prodos_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size)
{
    prodos_getxattr(req, ino, name, size, 0);
}


#pragma mark Directory Functions

/*
 * when the directory is opened, we load the volume/directory and store the FileEntry vector into
 * fi->fh.
 *
 */
static void prodos_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    fprintf(stderr, "opendir: %u\n", ino);
    // verify it's a directory/volume here?

    
    uint8_t buffer[BLOCK_SIZE];
    vector<FileEntry> files;
    bool ok;
    

    ok = disk->Read(ino == 1 ? 2 : ino >> 9, buffer);
    ERROR(ok < 0, EIO)
    
    
    if (ino == 1)
    {
        VolumeEntry v(buffer + 0x04);
        
        ok = disk->ReadVolume(&v, &files);
        
        ERROR(ok < 0, EIO)        
    }
    else
    {
        
        FileEntry e(buffer + (ino & 0x1ff));
        
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

static void prodos_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    fprintf(stderr,"releasedir: %d\n", ino);
    vector<FileEntry> *files = (vector<FileEntry> *)fi->fh;
    
    if (files) delete files;
    
    fuse_reply_err(req, 0);
}
static void prodos_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    vector<FileEntry> *files = (vector<FileEntry> *)fi->fh;
    struct stat st;
        
    fprintf(stderr, "readdir %u %u %u\n", ino, size, off);

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

#pragma mark Stat Functions

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
        
        SubdirEntry se(buffer + 0x04);            
        
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
    st->st_mtime = v.creation;
    st->st_atime = v.creation;
    
    st->st_nlink = v.file_count + 1;
    st->st_size = BLOCK_SIZE;
    st->st_blksize = BLOCK_SIZE;

    
    return 1;
}


static void prodos_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    uint8_t buffer[BLOCK_SIZE];
    struct stat st;
    int ok;
    
    fprintf(stderr, "get_attr %u\n", ino);
    
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
        VolumeEntry v(buffer + 0x04);
        ok = prodos_stat(v, &st);
        ERROR(ok < 0, EIO);
        
        st.st_ino = ino;
        
        fuse_reply_attr(req, &st, 0.0);
        return;
    }
    else 
    {

        
        FileEntry e(buffer + (ino & 0x1ff));
        ok = prodos_stat(e, &st);
        
        ERROR(ok < 0, EIO);
        
        st.st_ino = ino;

        fuse_reply_attr(req, &st, 0.0); //
    }
}


// TODO -- add Disk::Lookup support so we don't have to parse the entire dir header.
static void prodos_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    uint8_t buffer[BLOCK_SIZE];
    struct fuse_entry_param entry;
    int ok;
    vector<FileEntry> files;
    
    
    fprintf(stderr, "lookup: %d %s\n", parent, name);

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
        FileEntry e(buffer + (parent & 0x1ff));
        ERROR(e.storage_type != DIRECTORY_FILE, ENOENT);
        
        ok = disk->ReadDirectory(e.key_pointer, NULL, &files);
        ERROR(ok < 0, EIO)
    }
    // ok, now go through the file list and look for a (case insensitive) match.
    
    
    ok = -1;
    unsigned name_length = strlen(name);
    
    for(vector<FileEntry>::iterator iter = files.begin(); iter != files.end(); iter++)
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


#pragma mark Read Functions
static void prodos_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    fprintf(stderr, "open: %u\n", ino);

    
    uint8_t buffer[BLOCK_SIZE];
    int ok;
    
    FileEntry *e = NULL;
    
    ERROR(ino == 1, EISDIR)

    ok = disk->Read(ino >> 9, buffer);
    ERROR(ok < 0, EIO)
    
    e = new FileEntry(buffer + (ino & 0x1ff));
    
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

static void prodos_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    fprintf(stderr, "release: %d\n", ino);
    
    FileEntry *e = (FileEntry *)fi->fh;
    
    if (e) delete e;
    
    fuse_reply_err(req, 0);
    
}

static void prodos_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
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


static struct fuse_lowlevel_ops prodos_oper;

enum {
    PRODOS_OPT_HELP,
    PRODOS_OPT_VERSION
};

static struct fuse_opt prodos_opts[] = {
    FUSE_OPT_KEY("-h",             PRODOS_OPT_HELP),
    FUSE_OPT_KEY("--help",         PRODOS_OPT_HELP),
    FUSE_OPT_KEY("-V",             PRODOS_OPT_VERSION),
    FUSE_OPT_KEY("--version",      PRODOS_OPT_VERSION),
    {0, 0, 0}
};

static void usage()
{
    fprintf(stderr, "profuse [options] disk_image mountpoint\n");
}

static int prodos_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
    switch(key)
    {
            /*
        case PRODOS_OPT_HELP:
            usage();
            exit(0);
            break;
             */
        case PRODOS_OPT_VERSION:
            // TODO
            exit(1);
            break;
            
        case FUSE_OPT_KEY_NONOPT:
            if (dfile == NULL)
            {
                dfile = strdup(arg);
                return 0;
            }
            return 1;
    }
    return 1;
}


#ifdef __APPLE__

// create a dir in /Volumes/diskname.
bool make_mount_dir(string name, string &path)
{
    path = "";
    
    if (name.find('/') != string::npos) return false;
    if (name.find('\\') != string::npos) return false;
    
    path = "";
    path = "/Volumes/" + name;
    rmdir(path.c_str());
    if (mkdir(path.c_str(), 0777) == 0) return true;
    
    for (unsigned i = 0; i < 26; i++)
    {
        path = "/Volumes/" + name + "_" + (char)('a' + i);
        
        rmdir(path.c_str());
        if (mkdir(path.c_str(), 0777) == 0) return true;
        
        
        
    }
    
    path = "";
    return false;
    
}

#endif

int main(int argc, char *argv[])
{
    
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_chan *ch;
	char *mountpoint = NULL;
	int err = -1;
    
#if __APPLE__
    string mountpath;
#endif
    
    
    disk = NULL;
    
    bzero(&prodos_oper, sizeof(prodos_oper));
    
    prodos_oper.listxattr = prodos_listxattr;
    prodos_oper.getxattr = prodos_getxattr;
    
    prodos_oper.opendir = prodos_opendir;
    prodos_oper.releasedir = prodos_releasedir;
    prodos_oper.readdir = prodos_readdir;

    prodos_oper.lookup = prodos_lookup;
    prodos_oper.getattr = prodos_getattr;
    
    prodos_oper.open = prodos_open;
    prodos_oper.release = prodos_release;
    prodos_oper.read = prodos_read;
    

    // scan the argument list, looking for the name of the disk image.
    if (fuse_opt_parse(&args, NULL , prodos_opts, prodos_opt_proc) == -1)
        exit(1);
    
    if (dfile == NULL || *dfile == 0)
    {
        usage();
        exit(0);
    }
    
    disk = Disk::OpenFile(dfile);
    
    if (!disk)
    {
        fprintf(stderr, "Unable to mount disk %s\n", dfile);
        exit(1);
    }    


    
    disk->ReadVolume(&volume, NULL);
    
#ifdef __APPLE__
    {
        // Macfuse supports custom volume names (displayed in Finder)
        string str="-ovolname=";
        str += volume.volume_name;
        fuse_opt_add_arg(&args, str.c_str());
    }
#endif
    

    // use 512byte blocks.
    #if __APPLE__
    fuse_opt_add_arg(&args, "-oiosize=512");
    #endif
    
    do {

        if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) == -1) break;
        
#ifdef __APPLE__
      
        if (mountpoint == NULL || *mountpoint == 0)
        {
            if (make_mount_dir(volume.volume_name, mountpath))
                mountpoint = (char *)mountpath.c_str();
        }
        
#endif
        
        
        
        if (mountpoint == NULL || *mountpoint == 0)
        {
            fprintf(stderr, "no mount point\n");
            break;
        }
        
        
        

        
	    if ( (ch = fuse_mount(mountpoint, &args)) != NULL)
        {
            struct fuse_session *se;
            
            se = fuse_lowlevel_new(&args, &prodos_oper, sizeof(prodos_oper), NULL);
            if (se != NULL) 
            {
                if (fuse_set_signal_handlers(se) != -1)
                {
                    fuse_session_add_chan(se, ch);
                    err = fuse_session_loop(se);
                    fuse_remove_signal_handlers(se);
                    fuse_session_remove_chan(ch);
                }
                
                fuse_session_destroy(se);
            }
            fuse_unmount(mountpoint, ch);
        }
        
    } while (false);
    
    fuse_opt_free_args(&args);
    
    if (disk) delete disk;
    
    if (dfile) free(dfile);
    
#ifdef __APPLE__
    if (mountpath.size()) rmdir(mountpath.c_str());
#endif
    
	return err ? 1 : 0;
}
