#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include <vector>
#include <string>
#include <memory>

#include <unistd.h>


#ifdef __APPLE__
#define __DARWIN_64_BIT_INO_T 1 
#endif

#define _FILE_OFFSET_BITS 64 
#define FUSE_USE_VERSION 27

#include <fuse/fuse_opt.h>
#include <fuse/fuse_lowlevel.h>

#include <Pascal/Pascal.h>
#include <Common/Exception.h>

#include <File/File.h>

#include <Device/Device.h>
#include <Device/BlockDevice.h>

std::string fDiskImage;



void usage()
{
    std::printf("profuse_pascal 0.1\n\n");
    std::printf(
        "usage:\n"
        "profuse_pascal [options] diskimage [mountpoint]\n"
        "Options:\n"
        "  -d                debug\n"
        "  -r                readonly\n"
        "  -w                mount writable [not yet]\n"
        "  -v                verbose\n"
        "  --format=format   specify the disk image format. Valid values are:\n"
        "                    dc42  DiskCopy 4.2 Image\n"
        "                    davex Davex Disk Image\n"
        "                    2img  Universal Disk Image\n"
#ifdef HAVE_NUFX
        "                    sdk   ShrinkIt Disk Image\n"
#endif
        "                    do    DOS Order Disk Image\n"
        "                    po    ProDOS Order Disk Image (default)\n"
        "  -o opt1,opt2...   other mount parameters.\n"
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

struct options {
    char *format;
    int readOnly;
    int readWrite;
    int verbose;
} options;

#define PASCAL_OPT_KEY(T, P, V) {T, offsetof(struct options, P), V}

static struct fuse_opt pascal_options[] = {
    FUSE_OPT_KEY("-h",             PASCAL_OPT_HELP),
    FUSE_OPT_KEY("--help",         PASCAL_OPT_HELP),
    
    FUSE_OPT_KEY("-V",             PASCAL_OPT_VERSION),
    FUSE_OPT_KEY("--version",      PASCAL_OPT_VERSION),
    
    PASCAL_OPT_KEY("-v", verbose, 1),
    
    PASCAL_OPT_KEY("-w", readWrite, 1),
    PASCAL_OPT_KEY("rw", readWrite, 1),
    
    PASCAL_OPT_KEY("--format=%s", format, 0),
    PASCAL_OPT_KEY("format=%s", format, 0),
    
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
bool make_mount_dir(std::string name, std::string &path)
{
    path = "";
    
    if (name.find('/') != std::string::npos
        || name.find('\\') != std::string::npos
        || name.find(':') != std::string::npos )
    {
        name = "Pascal Volume";
    }
    
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
    struct options options;

    std::memset(&options, 0, sizeof(options));

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_chan *ch;
    char *mountpoint = NULL;
    int err = -1;
    std::string mountPath;
    unsigned format = 0;

    int foreground = false;
    int multithread = false;
    
    
    Pascal::VolumeEntryPointer volume;
    
	
    init_ops(&pascal_ops);


    // scan the argument list, looking for the name of the disk image.
    if (fuse_opt_parse(&args, &options ,pascal_options, pascal_option_proc) == -1)
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

        
    try
    {        
        Device::BlockDevicePointer device;
        
        device = Device::BlockDevice::Open(fDiskImage.c_str(), File::ReadOnly, format);
        
       
        if (!device.get())
        {
            std::fprintf(stderr, "Error: Unknown or unsupported device type.\n");
            exit(1);
        }
        
        volume = Pascal::VolumeEntry::Open(device);
    }
    catch (::Exception &e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        std::fprintf(stderr, "%s\n", std::strerror(e.error()));
        return -1;
    }    

    
    
    
    #ifdef __APPLE__
    {
        // Macfuse supports custom volume names (displayed in Finder)
        std::string str("-ovolname=");
        str += volume->name();
        fuse_opt_add_arg(&args, str.c_str());
    
        // 512 byte blocksize.    
        fuse_opt_add_arg(&args, "-oiosize=512");
        
    }
    #endif  

    fuse_opt_add_arg(&args, "-ofsname=PascalFS");

    if (!options.readOnly)
        fuse_opt_add_arg(&args, "-ordonly");
        
    if (options.readWrite)
    {
        std::fprintf(stderr, "Warning:  write support is not yet enabled.\n");
    }

    if (fuse_parse_cmdline(&args, &mountpoint, &multithread, &foreground) == -1)
    {
        usage();
        return -1;
    }
    
#ifdef __APPLE__
      
        if (mountpoint == NULL || *mountpoint == 0)
        {
            if (make_mount_dir(volume->name(), mountPath))
                mountpoint = (char *)mountPath.c_str();
        }
        
#endif    
    
    
    if ((ch = fuse_mount(mountpoint, &args)) != NULL)
    {
        struct fuse_session* se;

        std::printf("Mounting ``%s'' on ``%s''\n", volume->name(), mountpoint);
        
        se = fuse_lowlevel_new(&args, &pascal_ops, sizeof(pascal_ops), volume.get());

        if (se) do {

            
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



    fuse_opt_free_args(&args);


#ifdef __APPLE__
    if (!mountPath.empty()) rmdir(mountPath.c_str());
#endif


    return err ? 1 : 0;
}
