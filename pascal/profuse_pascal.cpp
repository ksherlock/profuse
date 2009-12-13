#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include <vector>
#include <string>
#include <memory>

#include <unistd.h>

#define __FreeBSD__ 10 
#define __DARWIN_64_BIT_INO_T 1 
#define _FILE_OFFSET_BITS 64 
#define FUSE_USE_VERSION 27

#include <fuse/fuse_opt.h>
#include <fuse/fuse_lowlevel.h>


#include "File.h"
#include "../BlockDevice.h"
#include "../Exception.h"
#include "../MappedFile.h"
#include "../DiskCopy42Image.h"

Pascal::VolumeEntry *fVolume = NULL;
std::string fDiskImage;

unsigned fFormat = 0;
bool fVerbose = false;
bool fReadOnly = true;

void usage()
{
    std::printf("profuse_pascal 0.1\n\n");
    std::printf(
        "usage:\n"
        "profuse_pascal [-w] [-f format] [-o options] diskimage [mountpoint]\n"
        "  -w               mount writable [not yet]\n"
        "  -f format        specify the disk image format. Valid values are:\n"
        "                   dc42  DiskCopy 4.2 Image\n"
        "                   do    DOS Order Disk Image\n"
        "                   po    ProDOS Order Disk Image (default)\n"
    );
}


static struct fuse_lowlevel_ops pascal_ops;

enum {
    PASCAL_OPT_HELP,
    PASCAL_OPT_VERSION,
    PASCAL_OPT_WRITE,
    PASCAL_OPT_FORMAT,
    PASCAL_OPT_VERBOSE
};

static struct fuse_opt pascal_options[] = {
    FUSE_OPT_KEY("-h",             PASCAL_OPT_HELP),
    FUSE_OPT_KEY("--help",         PASCAL_OPT_HELP),
    
    FUSE_OPT_KEY("-V",             PASCAL_OPT_VERSION),
    FUSE_OPT_KEY("--version",      PASCAL_OPT_VERSION),
    
    FUSE_OPT_KEY("-v",             PASCAL_OPT_VERBOSE),
    
    FUSE_OPT_KEY("-w",             PASCAL_OPT_WRITE),
    FUSE_OPT_KEY("rw",             PASCAL_OPT_WRITE),
    
    FUSE_OPT_KEY("-f=",            PASCAL_OPT_FORMAT),
    
    {0, 0, 0}
};


static int pascal_option_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
    switch(key)
    {
        case PASCAL_OPT_HELP:
            usage();
            exit(0);
            break;
            
        case PASCAL_OPT_VERSION:
            // TODO
            exit(0);
            break;
        case PASCAL_OPT_VERBOSE:
            fVerbose = true;
            return 0;
            break;
        
        case PASCAL_OPT_WRITE:
            fReadOnly = false;
            return 0;
        
        case PASCAL_OPT_FORMAT:
            fFormat = ProFUSE::DiskImage::ImageType(arg);
            if (fFormat == 0)
            {
                std::fprintf(stderr, "Invalid image format: ``%s''\n",
                    arg);
            }
            return 0;
        
        case FUSE_OPT_KEY_NONOPT:
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
bool make_mount_dir(std::string name, std::string &path)
{
    path = "";
    
    if (name.find('/') != std::string::npos) return false;
    if (name.find('\\') != std::string::npos) return false;
    if (name.find(':') != std::string::npos) return false;
    
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

int main(int argc, char **argv)
{
    extern void init_ops(fuse_lowlevel_ops *ops);


	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_chan *ch;
	char *mountpoint = NULL;
	int err = -1;
	std::string mountPath;

    int foreground = false;
    int multithread = false;
	
    init_ops(&pascal_ops);


    // scan the argument list, looking for the name of the disk image.
    if (fuse_opt_parse(&args, NULL ,pascal_options, pascal_option_proc) == -1)
        exit(1);
    
    if (fDiskImage.empty())
    {
        usage();
        exit(1);
    }

    // default prodos-order disk image.
    if (!fFormat)
        fFormat = ProFUSE::DiskImage::ImageType(fDiskImage.c_str(), 'PO__');
    
    try {
        std::auto_ptr<ProFUSE::BlockDevice> device;
       
        switch(fFormat)
        {
        case 'DC42':
            device.reset(new ProFUSE::DiskCopy42Image(fDiskImage.c_str(), fReadOnly));
            break;
            
        case 'PO__':
            device.reset(new ProFUSE::ProDOSOrderDiskImage(fDiskImage.c_str(), fReadOnly));
            break;
            
        case 'DO__':
            device.reset(new ProFUSE::DOSOrderDiskImage(fDiskImage.c_str(), fReadOnly));
            break;
        
        
        default:
            std::fprintf(stderr, "Error: Unknown or unsupported device type.\n");
            exit(1);
        }

        fVolume  = new Pascal::VolumeEntry(device.get());
        device.release();

    }
    catch (ProFUSE::POSIXException &e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        std::fprintf(stderr, "%s\n", std::strerror(e.error()));
    }    
    catch (ProFUSE::Exception &e)
    {
        std::fprintf(stderr, "%s\n", e.what());
    }
    
    
    
    #ifdef __APPLE__
    {
        // Macfuse supports custom volume names (displayed in Finder)
        std::string str("-ovolname=");
        str += fVolume->name();
        fuse_opt_add_arg(&args, str.c_str());
    
        // 512 byte blocksize.    
        fuse_opt_add_arg(&args, "-oiosize=512");
        
    }
    #endif  

    if (fReadOnly)
        fuse_opt_add_arg(&args, "-ordonly");

    if (fuse_parse_cmdline(&args, &mountpoint, &multithread, &foreground) == -1)
    {
        usage();
        return -1;
    }
    
#ifdef __APPLE__
      
        if (mountpoint == NULL || *mountpoint == 0)
        {
            if (make_mount_dir(fVolume->name(), mountPath))
                mountpoint = (char *)mountPath.c_str();
        }
        
#endif    
    
    
    if ((ch = fuse_mount(mountpoint, &args)) != NULL)
    {
        struct fuse_session* se;

        se = fuse_lowlevel_new(&args, &pascal_ops, sizeof(pascal_ops), fVolume);
        
        if (se) do {
            foreground = 1;
            multithread = 0;
            
            err = fuse_daemonize(foreground); // todo
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



    fuse_opt_free_args(&args);
    delete fVolume;


#ifdef __APPLE__
    if (!mountPath.empty()) rmdir(mountPath.c_str());
#endif


    return err ? 1 : 0;
}