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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include <vector>
#include <string>

#include <tr1/memory>

#include <Device/BlockDevice.h>


#include "profuse.h"



using std::vector;
using std::string;
using std::tr1::shared_ptr;


/*
 * globals variables.
 *
 */
 
std::string fDiskImage;


DiskPointer disk;
VolumeEntry volume;

bool validProdosName(const char *name)
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







static struct fuse_lowlevel_ops prodos_oper;

enum {
    PRODOS_OPT_HELP,
    PRODOS_OPT_VERSION,
    PRODOS_OPT_WRITE,
    PRODOS_OPT_FORMAT,
    PRODOS_OPT_VERBOSE
};

struct options {
    char *format;
    int readOnly;
    int readWrite;
    int verbose;
    int debug;
    
} options;

#define PRODOS_OPT_KEY(T, P, V) {T, offsetof(struct options, P), V}


static struct fuse_opt prodos_opts[] = {
    FUSE_OPT_KEY("-h",             PRODOS_OPT_HELP),
    FUSE_OPT_KEY("--help",         PRODOS_OPT_HELP),

    FUSE_OPT_KEY("-V",             PRODOS_OPT_VERSION),
    FUSE_OPT_KEY("--version",      PRODOS_OPT_VERSION),
    
    PRODOS_OPT_KEY("-v", verbose, 1),
    
    PRODOS_OPT_KEY("-w", readWrite, 1),
    PRODOS_OPT_KEY("rw", readWrite, 1),
    
    PRODOS_OPT_KEY("-d", debug, 1),

    PRODOS_OPT_KEY("--format=%s", format, 0),
    PRODOS_OPT_KEY("format=%s", format, 0),
    {0, 0, 0}
};

static void usage()
{
    fprintf(stderr, "profuse [options] disk_image [mountpoint]\n"
            
            "Options:\n"
            "  -d                debug\n"
            "  -r                readonly\n"
            "  -w                mount writable [not yet]\n"
            "  -v                verbose\n"
            "  --format=format   specify the disk image format. Valid values are:\n"
            "                    dc42  DiskCopy 4.2 Image\n"
            "                    davex Davex Disk Image\n"
            "                    2img  Universal Disk Image\n"
            "                    do    DOS Order Disk Image\n"
            "                    po    ProDOS Order Disk Image (default)\n"
            "  -o opt1,opt2...   other mount parameters.\n"            
            
            );
}

static int prodos_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
    switch(key)
    {
        case PRODOS_OPT_HELP:
            usage();
            exit(0);
            break;

        case PRODOS_OPT_VERSION:
            // TODO
            exit(1);
            break;

        case FUSE_OPT_KEY_NONOPT:
            // first arg is the disk image.
            if (fDiskImage.empty())
            {
                fDiskImage = arg;
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
    if (name.find(':') != string::npos) return false;
    
    path = "";
    path = "/Volumes/" + name;
    rmdir(path.c_str());
    if (mkdir(path.c_str(), 0777) == 0) return true;
    
    for (unsigned i = 0; i < 26; i++)
    {
        path = "/Volumes/" + name + " " + (char)('a' + i);
        
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
    struct options options;
    
    unsigned format = 0;
    
    int foreground = false; 
    int multithread = false;    
    
    
#if __APPLE__
    string mountpath;
#endif
    
    
    
        
    std::memset(&prodos_oper, 0, sizeof(prodos_oper));

    std::memset(&options, 0, sizeof(options));    

    
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
    if (fuse_opt_parse(&args, &options , prodos_opts, prodos_opt_proc) == -1)
        exit(1);
    
    if (fDiskImage.empty())
    {
        usage();
        exit(1);
    }
    
    // default prodos-order disk image.
    if (options.format)
    {
        format = Device::BlockDevice::ImageType(options.format);
        if (!format)
            std::fprintf(stderr, "Warning: Unknown image type ``%s''\n", options.format);
    }
    
    try {
        Device::BlockDevicePointer device;
        
        device.reset ( Device::BlockDevice::Open(fDiskImage.c_str(), File::ReadOnly, format) );
        
        if (!device.get())
        {
            std::fprintf(stderr, "Error: Unknown or unsupported device type.\n");
            exit(1);
        }
        
        disk = Disk::OpenFile(device);
        
        if (!disk.get())
        {
            fprintf(stderr, "Unable to mount disk %s\n", fDiskImage.c_str());
            exit(1);
        }    
    }
    catch (ProFUSE::POSIXException &e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        std::fprintf(stderr, "%s\n", e.errorString());
        return -1;
    }    
    catch (ProFUSE::Exception &e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        return -1;
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
        
        foreground = options.debug;
        
        
        if (mountpoint == NULL || *mountpoint == 0)
        {
            fprintf(stderr, "no mount point\n");
            break;
        }
        
        

	    if ( (ch = fuse_mount(mountpoint, &args)) != NULL)
        {
            struct fuse_session *se;
            
            se = fuse_lowlevel_new(&args, &prodos_oper, sizeof(prodos_oper), NULL);
            if (se != NULL) do {
            
                err = fuse_daemonize(foreground);
                if (err < 0 ) break;
                
                err = fuse_set_signal_handlers(se);
                if (err < 0) break;
                

                fuse_session_add_chan(se, ch);

                if (multithread) err = fuse_session_loop_mt(se);
                else err = fuse_session_loop(se);
                
                fuse_remove_signal_handlers(se);
                fuse_session_remove_chan(ch);
                
            } while (false);
            
            if (se) fuse_session_destroy(se);
            fuse_unmount(mountpoint, ch);
        }
        
    } while (false);
    
    fuse_opt_free_args(&args);
    
    disk.reset();
    
    
#ifdef __APPLE__
    if (mountpath.size()) rmdir(mountpath.c_str());
#endif
    
	return err ? 1 : 0;
}
