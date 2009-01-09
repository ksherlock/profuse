/*
 *  UniversalDiskImage.h
 *  profuse
 *
 *  Created by Kelvin Sherlock on 1/6/09.
 *
 */

#ifndef __UNIVERSAL_DISK_IMAGE_H__
#define __UNIVERSAL_DISK_IMAGE_H__

#include <stdint.h>


#define UDI_FORMAT_DOS_ORDER 0
#define UDI_FORMAT_PRODOS_ORDER 1
#define UDI_FORMAT_NIBBLIZED 2

struct UniversalDiskImage
{
    bool Load(const uint8_t * buffer);
    
    uint32_t magic_word;
    uint32_t creator;
    unsigned header_size;
    unsigned version;
    unsigned image_format;
    uint32_t flags;
    unsigned data_blocks;
    unsigned data_offset;
    unsigned data_size;
    unsigned comment_offset;
    unsigned comment_size;
    unsigned creator_data_offset;
    unsigned creator_data_size;
};


#endif

