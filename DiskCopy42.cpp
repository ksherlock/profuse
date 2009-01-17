/*
 *  DiskCopy42.cpp
 *  profuse
 *
 *  Created by Kelvin Sherlock on 1/4/09.
 *
 */

#include "DiskCopy42.h"
#include "common.h"
#include <string.h>



bool DiskCopy42::Load(const uint8_t *buffer)
{
    unsigned i;
    
    i = buffer[0];
    if (i >= 64) return false;

    memcpy(disk_name, &buffer[1], i);
    disk_name[i] = 0;


    data_size = load32_be(&buffer[64]);
    tag_size = load32_be(&buffer[68]);

    data_checksum = load32_be(&buffer[72]);
    tag_checksum = load32_be(&buffer[76]);
    
    disk_format = buffer[80];
    format_byte = buffer[81];
    private_word = load16_be(&buffer[82]);
    
    if (private_word != 0x100) return false;
    
    return true;
}

