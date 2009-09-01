/*
 *  newfs_prodos.cpp
 *  ProFUSE
 *
 *  Created by Kelvin Sherlock on 9/2/2009.
 *
 */
 
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <climits>
#include <cctype>

#include <unistd.h>
#include <libgen.h>

#define VERSION_STRING "0.1"

void help(void)
{
    printf("newfs_prodos v %s\n",  VERSION_STRING);
    
    printf("usage: newfs_prodos -N -s size -v volname file\n");
    printf("\t-N          test mode (do not create)\n");
    printf("\t-s size     file size in blocks (or use K/M)\n");
    printf("\t-v volname  volume name");
    
    exit(0);
}


    //returns number of blocks.
long parseBlocks(const char *cp)
{
    long size;
    char *end;
    unsigned base = 0;
    const char *ocp = cp;
    
    if (cp && *cp == '$')
    {
        base = 16;
        cp++;
    } 
    
    if (!cp || !*cp)
    {
        fprintf(stderr, "Invalid size.\n");
        exit(1);
    }
    
    size = strtol(cp, &end, base);
    
    if (end == cp)
    {
        fprintf(stderr, "Invalid size: `%s'\n", ocp);
        exit(1);
    }
    
    // number may be M, K, or null (blocks)    
    if (*end == 0) return size;
    if (*end == 'K') return size << 1;
    if (*end == 'M') return size << 10;
    
    fprintf(stderr, "Invalid size modifier: `%s'\n", end);
    exit(1);
     
    return -1;
}


// verify a size is reasonable for a ProDOS volume
bool validBlocks(long blocks)
{
    if (blocks < 280) return false;
    if (blocks > (32 << 10)) return false;
    
    return true;
}

bool validName(const char *name)
{
    unsigned i;
    
    if (name == 0 || *name == 0) return false;
        
    for(i = 0; name[i]; ++i)
    {
        char c = name[i];
        
        if (c == '.') continue;
        if (c & 0x80) return false;
        if (isdigit(c) && i) continue;
        if (isalpha(c)) continue;

        return false;
    }
    
    // i = strlen(name) + 1
    return i < 16;

}

const char *generateName(const char *src)
{
    // basename may alter it's argument.
    
    char * cp = strdup(src);
    
    cp = basename(cp);
    
    unsigned l = strlen(cp);
    // remove any extension...
    for (unsigned i = l; i; --i)
    {
        if (cp[i-1] == '.')
        {
            cp[i-1] = 0;
            l = i - 1;
            break;
        }
    } 
    if (validName(cp)) return cp;
    
    free(cp);
    return "Untitled";
}  

int main(int argc, char **argv)
{
    int ch;
    bool fTest = false;
    const char *fVolname = NULL;
    const char *filename;
    
    long fBlocks = 800 << 1;
    
    
    while ((ch = getopt(argc, argv, "hNs:v:")) != -1)
    {
        switch(ch)
        {
        case 'h':
        default:
            help();
            break;
            
        case 'N':
            fTest = true;
            break;
        
        case 'v':
            fVolname = optarg;
            break;
        
        
        case 's':
            fBlocks = parseBlocks(optarg);
            break;
        }   
    }

    // verify size is ok
    if (!validBlocks(fBlocks))
    {
        fprintf(stderr, "Invalid size.  Please use 140K -- 32M\n");
        exit(1);
    }
    
    if (fVolname && !validName(fVolname))
    {
        fprintf(stderr, "Invalid volume name: `%s'.\n", fVolname);
        exit(1);
    }

    argv += optind;
    argc -= optind;
    
    if (argc != 1)
    {
        help();
    }
    filename = *argv;
    
    if (!fVolname) fVolname = generateName(filename);

    if (fTest)
    {
        printf(" Image Name: %s\n", filename);
        printf("Volume Name: %s\n", fVolname);
        printf("Volume Size: %u blocks (%uK)\n", fBlocks, fBlocks >> 1);
    
        exit(0);
    }

    exit(0);
}