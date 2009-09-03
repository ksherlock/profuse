/*
 *  DiskCopy42.h
 *  profuse
 *
 *  Created by Kelvin Sherlock on 1/4/09.
 *
 */

#ifndef __DISKCOPY42__
#define __DISKCOPY42__


#include <stdint.h>

struct DiskCopy42
{
    bool Load(const uint8_t *buffer);
    static uint32_t CheckSum(uint8_t *buffer, unsigned length);
    
    char disk_name[64];
    uint32_t data_size;
    uint32_t tag_size;
    uint32_t data_checksum;
    uint32_t tag_checksum;
    unsigned disk_format;
    unsigned format_byte;
    unsigned private_word;
};

#endif

