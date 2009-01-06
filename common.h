/*
 *  common.h
 *  ProFUSE
 *
 *  Created by Kelvin Sherlock on 12/20/08.
 *
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>

#define BLOCK_SIZE 512

inline unsigned load16(const uint8_t *cp)
{
    return (cp[1] << 8 ) | cp[0];
}

inline unsigned load24(const uint8_t *cp)
{
    return (cp[2] << 16 ) | (cp[1] << 8) | (cp[0]);
}

inline unsigned load32(const uint8_t *cp)
{
    return (cp[3] << 24) | (cp[2] << 16 ) | (cp[1] << 8) | (cp[0]);
}



#endif

