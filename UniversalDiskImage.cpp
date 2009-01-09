/*
 *  UniversalDiskImage.cpp
 *  profuse
 *
 *  Created by Kelvin Sherlock on 1/6/09.
 *
 */

#include "UniversalDiskImage.h"

#include "common.h"
#include <string.h>

bool UniversalDiskImage::Load(const uint8_t *buffer)
{
    if (strncmp((const char *)buffer, "2IMG", 4) != 0) return false;
    
    // all numbers little-endian.
    
    magic_word = load32(&buffer[0]);
    
    creator = load32(&buffer[0x04]);

    header_size = load16(&buffer[0x08]);
    
    version = load16(&buffer[0x0a]);
    
    image_format = load32(&buffer[0x0c]);
    
    flags = load32(&buffer[0x10]);
    
    data_blocks = load32(&buffer[0x14]);
    
    data_offset = load32(&buffer[0x18]);
    data_size = load32(&buffer[0x1c]);


    comment_offset = load32(&buffer[0x20]);
    comment_size = load32(&buffer[0x24]);
    
    
    
    creator_data_offset = load32(&buffer[0x28]);
    creator_data_size = load32(&buffer[0x2c]);

    // 16 bytes reserved.


    return true;
}

