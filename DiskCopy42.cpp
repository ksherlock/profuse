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


uint32_t DiskCopy42::CheckSum(uint8_t *buffer, unsigned length)
{ 
    uint32_t checksum = 0;
    if (length & 0x01) return -1;

    /* 
     * checksum starts at 0
     * foreach big-endian 16-bit word w:
     *   checksum += w
     *   checksum = checksum rotate right 1 (bit 0 --> bit 31)
     */
    
    for(unsigned i = 0; i < length; i += 2)
    {
      checksum += (buffer[i] << 8);
      checksum += buffer[i + 1];
      checksum = (checksum >> 1) | (checksum  << 31);
      //if (checksum & 0x01) checksum = (checksum >> 1) | 0x80000000;
      //else checksum >>= 1;
    }
    return checksum;

}

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

