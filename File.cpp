/*
 *  File.cpp
 *  ProFUSE
 *
 *  Created by Kelvin Sherlock on 12/18/08.
 *
 */

#include "File.h"

#include "common.h"
#include <string.h>
#include <stdint.h>
#include <ctype.h>


/*
 * ProDOS technote 28
 *
 * The following definition allows the same range of years that the Apple IIgs 
 * Control Panel CDA currently does:
 *
 * o  A seven-bit ProDOS year value is in the range 0 to 99
 * (100 through 127 are invalid)
 * o  Year values from 40 to 99 represent 1940 through 1999
 * o  Year values from 0 to 39 represent 2000 through 2039
 */
inline time_t timeToUnix(unsigned yymmdd, unsigned hhmm)
{
    if (yymmdd == 0) return 0;
    
    tm t;
    bzero(&t, sizeof(tm));
    
    t.tm_min = hhmm & 0x3f;
    t.tm_hour = (hhmm >> 8) & 0x1f;
    
    t.tm_mday = yymmdd & 0x1f;
    t.tm_mon = ((yymmdd >> 5) & 0x0f) - 1;
    t.tm_year = (yymmdd) >> 9;
    
    if (t.tm_year <= 39) t.tm_year += 100;
    
    return mktime(&t);
}



FileEntry::FileEntry()
{
    bzero(this, sizeof(FileEntry));
}
FileEntry::FileEntry(const FileEntry& f)
{
    *this = f;
}
FileEntry::FileEntry(const FileEntry *f)
{
    if (f) *this = *f;
    else bzero(this, sizeof(FileEntry));
}

FileEntry::FileEntry(const void *data)
{
    const uint8_t *cp = (const uint8_t *)data;
    
    address = 0;
    
    storage_type = cp[0x00] >> 4;
    name_length = cp[0x00] & 0x0f;
    
    memcpy(file_name, &cp[0x01], name_length);
    file_name[name_length] = 0;
    
    file_type = cp[0x10];
    
    key_pointer = load16(&cp[0x11]);
    
    blocks_used = load16(&cp[0x13]);
    
    eof = load24(&cp[0x15]);

    creation = timeToUnix(load16(&cp[0x18]), load16(&cp[0x1a]));
    
    //version = cp[0x1c];
    //min_version = cp[0x1d];
    
    unsigned xcase = load16(&cp[0x1c]);
    if (xcase & 0x8000)
    {
        // gsos technote #8
        unsigned mask = 0x4000;
        for (unsigned i = 0; i < name_length; i++)
        {
            if (xcase & mask) file_name[i] = tolower(file_name[i]);
            mask = mask >> 1;
        }
    }
    
    
    
    access = cp[0x1e];


    aux_type = load16(&cp[0x1f]);
    
    last_mod = timeToUnix(load16(&cp[0x21]), load16(&cp[0x23]));
    
    header_pointer = load16(&cp[0x25]);
}


ExtendedEntry::ExtendedEntry()
{
    bzero(this, sizeof(ExtendedEntry));
}

ExtendedEntry::ExtendedEntry(const void *data)
{
    const uint8_t *cp = (const uint8_t *)data;
    
    //prodos technote #25.
    // offset 0 - mini entry for data fork
    
    dataFork.storage_type = cp[0x00] & 0x0f;
    dataFork.key_block = load16(&cp[0x01]);
    dataFork.blocks_used = load16(&cp[0x03]);
    dataFork.eof = load24(&cp[0x05]);
    
    // offset 256 - mini entry for resource fork.

    resourceFork.storage_type = cp[256 + 0x00] & 0x0f;
    resourceFork.key_block = load16(&cp[256 + 0x01]);
    resourceFork.blocks_used = load16(&cp[256 + 0x03]);
    resourceFork.eof = load24(&cp[256 + 0x05]);
    
    // xFInfo may be missing.
    bzero(FInfo, sizeof(FInfo));
    bzero(xFInfo, sizeof(xFInfo));
    
    // size must be 18.
    unsigned size;
    unsigned entry;
    
    for (unsigned i = 0; i < 2; i++)
    {
        unsigned ptr = i == 0 ? 8 : 26;
        size = cp[ptr];
        if (size != 18) continue;
        entry = cp[ptr + 1];
        switch(entry)
        {
            case 1:
                memcpy(FInfo, &cp[ptr + 2], 16);
                break;
            case 2:
                memcpy(xFInfo, &cp[ptr + 2], 16);
                break;
        }
    }
    //
}



VolumeEntry::VolumeEntry()
{
    bzero(this, sizeof(VolumeEntry));
}


VolumeEntry::VolumeEntry(const void *data)
{
    const uint8_t *cp = (const uint8_t *)data;

    //prev_block = load16(&cp[0x00]);
    //next_block = load16(&cp[0x02]);
    
    storage_type = cp[0x00] >> 4;
    name_length = cp[0x00] & 0x0f;
    
    memcpy(volume_name, &cp[0x01], name_length);
    volume_name[name_length] = 0;

    // 0x14--0x1b reserved
    
    //creation = timeToUnix(load16(&cp[0x18]), load16(&cp[0x1a]));

    //version = cp[0x1c];
    //min_version = cp[0x1d];
    
    unsigned xcase = load16(&cp[0x16]);
    if (xcase & 0x8000)
    {
        // gsos technote #8
        unsigned mask = 0x4000;
        for (unsigned i = 0; i < name_length; i++)
        {
            if (xcase & mask) volume_name[i] = tolower(volume_name[i]);
            mask = mask >> 1;
        }
    }    
    
    
    access = cp[0x1e];

    entry_length = cp[0x1f];

    entries_per_block = cp[0x20];

    file_count = load16(&cp[0x21]);

    bit_map_pointer = load16(&cp[0x23]);
    
    total_blocks = load16(&cp[0x25]);
}



SubdirEntry::SubdirEntry()
{
    bzero(this, sizeof(SubdirEntry));
}


SubdirEntry::SubdirEntry(const void *data)
{
    const uint8_t *cp = (const uint8_t *)data;
    
    
    //prev_block = load16(&cp[0x00]);
    //next_block = load16(&cp[0x02]);
    
    storage_type = cp[0x00] >> 4;
    name_length = cp[0x00] & 0x0f;
    
    memcpy(subdir_name, &cp[0x01], name_length);
    subdir_name[name_length] = 0;
    
    // 0x14 should be $14.
    
    // 0x145-0x1b reserved
    
    creation = timeToUnix(load16(&cp[0x18]), load16(&cp[0x1a]));
    
    //version = cp[0x1c];
    //min_version = cp[0x1d];
    /*
    unsigned xcase = load16(&cp[0x1c]);
    if (xcase & 0x8000)
    {
        // gsos technote #8
        unsigned mask = 0x4000;
        for (unsigned i = 0; i < name_length; i++)
        {
            if (xcase & mask) subdir_name[i] = tolower(subdir_name[i]);
            mask = mask >> 1;
        }
    }    
    */
    
    access = cp[0x1e];
    
    entry_length = cp[0x1f];
    
    entries_per_block = cp[0x20];
    
    file_count = load16(&cp[0x21]);
    
    parent_pointer = load16(&cp[0x23]);
    
    parent_entry = cp[0x25];
    
    parent_entry_length = cp[0x26];
}
