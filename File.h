/*
 *  File.h
 *  ProFUSE
 *
 *  Created by Kelvin Sherlock on 12/18/08.
 *
 */

#ifndef __FILE_H__
#define __FILE_H__

#include <time.h>
#include <stdint.h>

enum {
    DELETED_FILE = 0,
    SEEDLING_FILE = 1,
    SAPLING_FILE = 2,
    TREE_FILE = 3,
    PASCAL_FILE = 4,
    EXTENDED_FILE = 5,
    DIRECTORY_FILE = 0x0d,
    SUBDIR_HEADER = 0x0e,
    VOLUME_HEADER = 0x0f
};


enum {
    FILE_ENTRY_SIZE = 0x27,
};


enum {
    ACCESS_DESTROY = 0x80,
    ACCESS_RENAME = 0x40,
    ACCESS_MODIFIED = 0x20,
    ACCESS_WRITE = 0x02,
    ACCRESS_READ = 0x01
};


class FileEntry {
public:
    
    bool Load(const void *data);
    
    unsigned storage_type;
    unsigned name_length;
    char file_name[15 + 1];
    unsigned file_type;
    unsigned key_pointer;
    unsigned blocks_used;
    uint32_t eof;
    time_t creation;
    //unsigned version;
    //unsigned min_version;
    unsigned access;
    unsigned aux_type;
    time_t last_mod;
    unsigned header_pointer;
    
    uint32_t address;
};



struct MiniEntry {

    unsigned storage_type;
    unsigned key_block;
    unsigned blocks_used;
    uint32_t eof;
    
};

class ExtendedEntry {
public:
    
    bool Load(const void *data);
    
    MiniEntry dataFork;
    MiniEntry resourceFork;
    
    uint8_t FInfo[16];
    uint8_t xFInfo[16];
};


class VolumeEntry {
public:
    
    bool Load(const void *data);
    
    unsigned storage_type;
    unsigned name_length;
    char volume_name[15+1];
    time_t creation;
    time_t last_mod;
    //unsigned version;
    //unsigned min_version;
    unsigned access;
    unsigned entry_length;
    unsigned entries_per_block;
    unsigned file_count;
    unsigned bit_map_pointer;
    unsigned total_blocks;

    friend class DirIter;

};

class SubdirEntry {
public:
    
    bool Load(const void *data);
    
    unsigned storage_type;
    unsigned name_length;
    char subdir_name[15+1];
    time_t creation;
    //unsigned version;
    //unsigned min_version;
    unsigned access;
    unsigned entry_length;
    unsigned entries_per_block;
    unsigned file_count;
    unsigned parent_pointer;
    unsigned parent_entry;
    unsigned parent_entry_length;
};


#endif
