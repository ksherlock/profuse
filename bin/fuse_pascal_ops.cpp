
#ifdef __APPLE__
#define __DARWIN_64_BIT_INO_T 1 
#endif

#define _FILE_OFFSET_BITS 64 
#define FUSE_USE_VERSION 27

#include <string>
#include <vector>
#include <cerrno>
#include <cstdio>
#include <cstring>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>




#include <fuse/fuse_opt.h>
#include <fuse/fuse_lowlevel.h>



#include <Pascal/Pascal.h>
#include <Common/auto.h>
#include <Common/Exception.h>
#include <POSIX/Exception.h>

#define NO_ATTR() \
{ \
    if (size) fuse_reply_buf(req, NULL, 0); \
    else fuse_reply_xattr(req, 0); \
    return; \
}

#undef ERROR
#define ERROR(cond,errno) if ( (cond) ){ fuse_reply_err(req, errno); return; }


#define RETURN_XATTR(DATA, SIZE) \
{ \
    if (size == 0) { fuse_reply_xattr(req, SIZE); return; } \
    if (size < SIZE) { fuse_reply_err(req, ERANGE); return; } \
    fuse_reply_buf(req, DATA, SIZE); return; \
}


#define DEBUGNAME() \
    if (0) { std::fprintf(stderr, "%s\n", __func__); }

// linux doesn't have ENOATTR.
#ifndef ENOATTR 
#define ENOATTR ENOENT
#endif

using namespace Pascal;


// fd_table is files which have been open.
// fd_table_available is a list of indexes in fd_table which are not currently used.
static std::vector<FileEntryPointer> fd_table;
static std::vector<unsigned> fd_table_available;

static FileEntryPointer findChild(VolumeEntry *volume, unsigned inode)
{

    for (unsigned i = 0, l = volume->fileCount(); i < l; ++i)
    {
        FileEntryPointer child = volume->fileAtIndex(i);
        if (!child) continue;
        
        if (inode == child->inode()) return child;
    }

    return FileEntryPointer();
}


#pragma mark -
#pragma mark fs


static void pascal_init(void *userdata, struct fuse_conn_info *conn)
{
    DEBUGNAME()

    // nop
    
    // text files have a non-thread safe index.
    // which is initialized via read() or fileSize()
    VolumeEntry *volume = (VolumeEntry *)userdata;
    
    for (unsigned i = 0, l = volume->fileCount(); i < l; ++i)
    {
        FileEntryPointer child = volume->fileAtIndex(i);
        child->fileSize();
    }

}

static void pascal_destroy(void *userdata)
{
    DEBUGNAME()

    // nop
}


static void pascal_statfs(fuse_req_t req, fuse_ino_t ino)
{
    DEBUGNAME()

    struct statvfs vst;
    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);

    ERROR(!volume, EIO)
    
    // returns statvfs for the mount path or any file in the fs
    // therefore, ignore ino.
    std::memset(&vst, 0, sizeof(vst));
    
    vst.f_bsize = 512;  // fs block size
    vst.f_frsize = 512; // fundamental fs block size
    vst.f_blocks = volume->volumeBlocks();
    vst.f_bfree = volume->freeBlocks(true); // free blocks
    vst.f_bavail = volume->freeBlocks(false); // free blocks (non-root)
    vst.f_files = volume->fileCount();
    vst.f_ffree = -1;  // free inodes.
    vst.f_favail = -1; // free inodes (non-root)
    vst.f_fsid = 0; // file system id?
    vst.f_flag = volume->readOnly() ?  ST_RDONLY | ST_NOSUID : ST_NOSUID;
    vst.f_namemax = 15;
    
    fuse_reply_statfs(req, &vst);    
}


#pragma mark -
#pragma mark xattr

static void pascal_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size)
{
    DEBUGNAME()

    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    FileEntryPointer file;
    std::string attr;
    unsigned attrSize;
    
    if (ino == 1) NO_ATTR()
    
    file = findChild(volume, ino);
    
    ERROR(file == NULL, ENOENT)
    
    attr += "pascal.fileKind";
    attr.append(1, 0);
    
    if (file->fileKind() == kTextFile)
    {
        attr += "com.apple.TextEncoding";
        attr.append(1, 0);

        attr += "user.charset";
        attr.append(1, 0);
    }
    // user.mime_type ? text/plain, ...
    
    attrSize = attr.size();
    if (size == 0)
    {
        fuse_reply_xattr(req, attrSize);
        return;        
    }
    
    ERROR(size < attrSize, ERANGE)

    fuse_reply_buf(req, attr.data(), attrSize);
}




static void pascal_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size)
{
    DEBUGNAME()


    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    FileEntryPointer file;
    std::string attr(name);
    
    ERROR(ino == 1, ENOATTR)
    
    file = findChild(volume, ino);
    
    ERROR(file == NULL, ENOENT)
    
    if (attr == "pascal.fileKind")
    {
        uint8_t fk = file->fileKind();

        RETURN_XATTR((const char *)&fk, 1)
    }

    if (file->fileKind() == kTextFile)
    {

        if (attr == "user.charset")
        {
            static const char data[]="ascii";
            static unsigned dataSize = sizeof(data) - 1;

            RETURN_XATTR(data, dataSize)
        }
        
        if (attr == "com.apple.TextEncoding")
        {
            static const char data[] = "us-ascii;1536";
            static unsigned dataSize = sizeof(data) - 1;

            RETURN_XATTR(data, dataSize)
        }
    }
    
    ERROR(true, ENOATTR);
    return;
}

// OS X version.
static void pascal_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size, uint32_t off)
{
    DEBUGNAME()

    pascal_getxattr(req, ino, name, size);
}

#pragma mark -
#pragma mark dirent

static void pascal_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    DEBUGNAME()

    ERROR(ino != 1, ENOTDIR)
    
    fi->fh = 0;
    
    fuse_reply_open(req, fi);
}

static void pascal_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    DEBUGNAME()

    fuse_reply_err(req, 0);
}

static void pascal_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    DEBUGNAME()

    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    ::auto_array<uint8_t> buffer(new uint8_t[size]);
    unsigned count = volume->fileCount();


    struct stat st;
    unsigned currentSize = 0;

    std::memset(&st, 0, sizeof(struct stat));
    
        // . and .. need to be added in here but are handled by the fs elsewhere.
    
    do {
        if (off == 0)
        {
            unsigned tmp;
            
            st.st_mode = S_IFDIR | 0555;
            st.st_ino = 1;
            
            tmp = fuse_add_direntry(req, NULL, 0, ".", NULL, 0);
            
            if (tmp + currentSize > size) break;
    
            fuse_add_direntry(req, (char *)buffer.get() + currentSize, size, ".", &st, ++off);
            currentSize += tmp;
        }
        
        if (off == 1)
        {
            unsigned tmp;
            
            st.st_mode = S_IFDIR | 0555;
            st.st_ino = 1;
            
            tmp = fuse_add_direntry(req, NULL, 0, "..", NULL, 0);
            
            if (tmp + currentSize > size) break;
    
            fuse_add_direntry(req, (char *)buffer.get() + currentSize, size, "..", &st, ++off);
            currentSize += tmp;
        }
    
        for (unsigned i = off - 2; i < count; ++i)
        {
            unsigned tmp;
            
            FileEntryPointer file = volume->fileAtIndex(i);
            if (file == NULL) break; //?
        
            // only these fields are used.
            st.st_mode = S_IFREG | 0444;
            st.st_ino = file->inode();
            
        
            tmp = fuse_add_direntry(req, NULL, 0, file->name(), NULL, 0);
            
            if (tmp + currentSize > size) break;
        
            fuse_add_direntry(req, (char *)buffer.get() + currentSize, size, file->name(), &st, ++off);
            currentSize += tmp;
        }
    
    } while (false);
    
    
    fuse_reply_buf(req, (char *)buffer.get(), currentSize);
    
}


#pragma mark -
#pragma mark stat


static void stat(FileEntry *file, struct stat *st)
{
    DEBUGNAME()

    std::memset(st, 0, sizeof(struct stat));
    
    time_t t = file->modification();
    
    st->st_ino = file->inode();
    st->st_nlink = 1;
    st->st_mode = S_IFREG | 0444;
    st->st_size = file->fileSize();
    st->st_blocks = file->blocks();
    st->st_blksize = 512;
    
    st->st_atime = t;
    st->st_mtime = t;
    st->st_ctime = t;
    // st.st_birthtime not yet supported by MacFUSE.
}

static void stat(VolumeEntry *volume, struct stat *st)
{
    DEBUGNAME()
    
    std::memset(st, 0, sizeof(struct stat));
    
    time_t t = volume->lastBoot();
    
    st->st_ino = volume->inode();
    st->st_nlink = 1 + volume->fileCount();
    st->st_mode = S_IFDIR | 0555;
    st->st_size = volume->blocks() * 512;
    st->st_blocks = volume->blocks();
    st->st_blksize = 512;
    
    st->st_atime = t;
    st->st_mtime = t;
    st->st_ctime = t;
    // st.st_birthtime not yet supported by MacFUSE.
}

static void pascal_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    DEBUGNAME()

    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    struct fuse_entry_param entry;

    ERROR(parent != 1, ENOTDIR)
    ERROR(!FileEntry::ValidName(name), ENOENT)

    
    for (unsigned i = 0, l = volume->fileCount(); i < l; ++i)
    {
        FileEntryPointer file = volume->fileAtIndex(i);
        if (file == NULL) break;
    
        if (::strcasecmp(file->name(), name)) continue;
        
        // found it!
        std::memset(&entry, 0, sizeof(entry));
        entry.attr_timeout = 0.0;
        entry.entry_timeout = 0.0;
        entry.ino = file->inode();
        
        stat(file.get(), &entry.attr);
        fuse_reply_entry(req, &entry); 
        return;
    }
    
    ERROR(true, ENOENT)
}


static void pascal_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    DEBUGNAME()

    struct stat st;
    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    FileEntryPointer file;
    
    if (ino == 1)
    {
        stat(volume, &st);
        fuse_reply_attr(req, &st, 0.0);
        return;
    }
    
    file = findChild(volume, ino);
    
    ERROR(file == NULL, ENOENT)
    //printf("\t%s\n", file->name());
    
    stat(file.get(), &st);
    fuse_reply_attr(req, &st, 0.0);    
}


#pragma mark -
#pragma mark file

static void pascal_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    unsigned index;
    DEBUGNAME()

    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    FileEntryPointer file;
    
    ERROR(ino == 1, EISDIR)
    
    file = findChild(volume, ino);
    
    ERROR(file == NULL, ENOENT)
    
    ERROR((fi->flags & O_ACCMODE) != O_RDONLY, EACCES)
    
    // insert the FileEntryPointer into fd_table.
    if (fd_table_available.size())
    {
        index = fd_table_available.back();
        fd_table_available.pop_back();
        fd_table[index] = file;
    }
    else
    {
        index = fd_table.size();
        fd_table.push_back(file);
    }
    
    fi->fh = index;
    
    fuse_reply_open(req, fi);
}


static void pascal_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    unsigned index = fi->fh;
    DEBUGNAME()

    fd_table[index].reset();
    fd_table_available.push_back(index);

    fuse_reply_err(req, 0);
}


static void pascal_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    unsigned index = fi->fh;
    
    DEBUGNAME()


    //VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    FileEntryPointer file = fd_table[index];
    

    
    try
    {
        ::auto_array<uint8_t> buffer(new uint8_t[size]);
        unsigned rsize = file->read(buffer.get(), size, off);
        
        fuse_reply_buf(req, (char *)(buffer.get()), rsize);
        return;
    }
    
    catch (POSIX::Exception &e)
    {
        printf("posix error...\n");
        ERROR(true, e.error());
    }
    catch( ... )
    {
        printf("error...\n");
        ERROR(true, EIO)
    }
    
}

void init_ops(fuse_lowlevel_ops *ops)
{
    DEBUGNAME()

    std::memset(ops, 0, sizeof(fuse_lowlevel_ops));

    ops->init = pascal_init;
    ops->destroy = pascal_destroy;
    ops->statfs = pascal_statfs;
    
    // returns pascal.filekind, text encoding.
    ops->listxattr = pascal_listxattr;
    ops->getxattr = pascal_getxattr;
    
    // volume is a dir.
    ops->opendir = pascal_opendir;
    ops->releasedir = pascal_releasedir;
    ops->readdir = pascal_readdir;
    
    
    ops->lookup = pascal_lookup;
    ops->getattr = pascal_getattr;
    
    ops->open = pascal_open;
    ops->release = pascal_release;
    ops->read = pascal_read;

}
