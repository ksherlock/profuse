

#include <string>
#include <vector>
#include <cerrno>
#include <cstdio>

#include <sys/stat.h>
#include <unistd.h>


#define __FreeBSD__ 10 
#define __DARWIN_64_BIT_INO_T 1 
#define _FILE_OFFSET_BITS 64 

#define FUSE_USE_VERSION 27

#include <fuse/fuse_opt.h>
#include <fuse/fuse_lowlevel.h>



#include <Pascal/File.h>
#include <ProFUSE/auto.h>
#include <ProFUSE/Exception.h>

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



using namespace Pascal;


static FileEntry *findChild(VolumeEntry *volume, unsigned inode)
{

    for (unsigned i = 0, l = volume->fileCount(); i < l; ++i)
    {
        FileEntry *child = volume->fileAtIndex(i);
        if (inode == child->inode()) return child;
    }

    return NULL;
}

static void pascal_init(void *userdata, struct fuse_conn_info *conn)
{
    std::printf("pascal_init\n");
    // nop
    
    // text files have a non-thread safe index.
    // which is initialized via read() or fileSize()
    VolumeEntry *volume = (VolumeEntry *)userdata;
    for (unsigned i = 0, l = volume->fileCount(); i < l; ++i)
    {
        volume->fileAtIndex(i)->fileSize();
    }

}

static void pascal_destroy(void *userdata)
{
    std::printf("pascal_destroy\n");

    // nop
}



#pragma mark -
#pragma mark xattr

static void pascal_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size)
{
    std::printf("pascal_listxattr\n");

    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    FileEntry *file;
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
    std::printf("pascal_getxattr\n");


    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    FileEntry *file;
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
    pascal_getxattr(req, ino, name, size);
}

#pragma mark -
#pragma mark dirent

static void pascal_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    std::printf("pascal_opendir\n");


    ERROR(ino != 1, ENOTDIR)
    
    fi->fh = 0;
    
    fuse_reply_open(req, fi);
}

static void pascal_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    std::printf("pascal_releasedir\n");

    fuse_reply_err(req, 0);
}

static void pascal_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    std::printf("pascal_readdir %u, %u, %u\n", (unsigned)ino, (unsigned)size, (unsigned)off);

    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    ProFUSE::auto_array<uint8_t> buffer(new uint8_t[size]);
    unsigned count = volume->fileCount();


    struct stat st;
    unsigned currentSize = 0;

    std::memset(&st, 0, sizeof(st));
    
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
            
            FileEntry *file = volume->fileAtIndex(i);
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
    std::memset(st, 0, sizeof(st));
    
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
    std::memset(st, 0, sizeof(st));
    
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
    std::printf("pascal_lookup %u %s\n", (unsigned)parent, name);

    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    struct fuse_entry_param entry;

    ERROR(parent != 1, ENOTDIR)
    ERROR(!FileEntry::ValidName(name), ENOENT)

    
    for (unsigned i = 0, l = volume->fileCount(); i < l; ++i)
    {
        FileEntry *file = volume->fileAtIndex(i);
        if (file == NULL) break;
    
        if (::strcasecmp(file->name(), name)) continue;
        
        // found it!
        std::memset(&entry, 0, sizeof(entry));
        entry.attr_timeout = 0.0;
        entry.entry_timeout = 0.0;
        entry.ino = file->inode();
        
        stat(file, &entry.attr);
        fuse_reply_entry(req, &entry); 
        return;
    }
    
    ERROR(true, ENOENT)
}


static void pascal_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    std::printf("pascal_getattr %u\n", (unsigned)ino);

    struct stat st;
    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    FileEntry *file;
    
    if (ino == 1)
    {
        stat(volume, &st);
        fuse_reply_attr(req, &st, 0.0);
        return;
    }
    
    file = findChild(volume, ino);
    
    ERROR(file == NULL, ENOENT)
    printf("\t%s\n", file->name());
    
    stat(file, &st);
    fuse_reply_attr(req, &st, 0.0);    
}


#pragma mark -
#pragma mark file

static void pascal_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    std::printf("pascal_open\n");

    VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    FileEntry *file;
    
    ERROR(ino == 1, EISDIR)
    
    file = findChild(volume, ino);
    
    ERROR(file == NULL, ENOENT)
    
    ERROR((fi->flags & O_ACCMODE) != O_RDONLY, EACCES)
    
    fi->fh = (uint64_t)file;
    
    fuse_reply_open(req, fi);
}


static void pascal_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    std::printf("pascal_release\n");

    fuse_reply_err(req, 0);
}


static void pascal_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    std::printf("pascal_read %u %u %u\n", (unsigned)ino, (unsigned)size, (unsigned)off);

    //VolumeEntry *volume = (VolumeEntry *)fuse_req_userdata(req);
    FileEntry *file = (FileEntry *)fi->fh;
    

    
    try
    {
        ProFUSE::auto_array<uint8_t> buffer(new uint8_t[size]);
        unsigned rsize = file->read(buffer.get(), size, off);
        
        fuse_reply_buf(req, (char *)(buffer.get()), rsize);
        return;
    }
    catch (ProFUSE::POSIXException &e)
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

    std::memset(ops, 0, sizeof(fuse_lowlevel_ops));

    ops->init = pascal_init;
    ops->destroy = pascal_destroy;
    
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
