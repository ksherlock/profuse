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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include <vector>
#include <string>


#include "profuse.h"

using std::vector;
using std::string;

Disk *disk = NULL;
char *dfile = NULL;
VolumeEntry volume;

bool dos_order = false;



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
    PRODOS_OPT_DOS_ORDER
};

static struct fuse_opt prodos_opts[] = {
    FUSE_OPT_KEY("-h",             PRODOS_OPT_HELP),
    FUSE_OPT_KEY("--help",         PRODOS_OPT_HELP),
    FUSE_OPT_KEY("-V",             PRODOS_OPT_VERSION),
    FUSE_OPT_KEY("--version",      PRODOS_OPT_VERSION),
    FUSE_OPT_KEY("--dos-order",    PRODOS_OPT_DOS_ORDER),
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
        case PRODOS_OPT_DOS_ORDER:
            dos_order = true;
            return 0;
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
    
    disk = Disk::OpenFile(dfile, dos_order);
    
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
