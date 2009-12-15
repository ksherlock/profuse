#ifndef __ENDIAN_H__
#define __ENDIAN_H__

// utlities to read/write bytes.

#include <stdint.h>

namespace LittleEndian {


    inline uint8_t Read8(const void *vp)
    {
        const uint8_t *p = (const uint8_t *)vp;
        return (p[0]);        
    }
    
    uint16_t Read16(const void *vp);

    uint32_t Read24(const void *vp);

    uint32_t Read32(const void *vp);
   
    
    inline uint8_t Read8(const void *vp, unsigned offset)
    {
        return Read8(offset + (const uint8_t *)vp);       
    }    
    
    inline uint16_t Read16(const void *vp, unsigned offset)
    {
        return Read16(offset + (const uint8_t *)vp);       
    }    
    
    inline uint32_t Read24(const void *vp, unsigned offset)
    {
        return Read24(offset + (const uint8_t *)vp);       
    }    
    
    inline uint32_t Read32(const void *vp, unsigned offset)
    {
        return Read32(offset + (const uint8_t *)vp);       
    }        


    // write
    inline void Write8(void *vp, uint8_t x)
    {
        uint8_t *p = (uint8_t *)vp;
        p[0] = x;        
    }

    void Write16(void *vp, uint16_t x);

    void Write24(void *vp, uint32_t x);

    void Write32(void *vp, uint32_t x);

    
    inline void Write8(void *vp, unsigned offset, uint8_t x)
    {
        Write8(offset + (uint8_t *)vp, x);
    }

    inline void Write16(void *vp, unsigned offset, uint16_t x)
    {
        Write16(offset + (uint8_t *)vp, x);
    }

    inline void Write24(void *vp, unsigned offset, uint32_t x)
    {
        Write24(offset + (uint8_t *)vp, x);
    }

    inline void Write32(void *vp, unsigned offset, uint32_t x)
    {
        Write32(offset + (uint8_t *)vp, x);
    }

}


namespace BigEndian {


    inline uint8_t Read8(const void *vp)
    {
        const uint8_t *p = (const uint8_t *)vp;
        return p[0];        
    }
    
    uint16_t Read16(const void *vp);

    uint32_t Read24(const void *vp);

    uint32_t Read32(const void *vp);
   
    
    inline uint8_t Read8(const void *vp, unsigned offset)
    {
        return Read8(offset + (const uint8_t *)vp);       
    }    
    
    inline uint16_t Read16(const void *vp, unsigned offset)
    {
        return Read16(offset + (const uint8_t *)vp);       
    }    
    
    inline uint32_t Read24(const void *vp, unsigned offset)
    {
        return Read24(offset + (const uint8_t *)vp);       
    }    
    
    inline uint32_t Read32(const void *vp, unsigned offset)
    {
        return Read32(offset + (const uint8_t *)vp);       
    }        



    // write
    inline void Write8(void *vp, uint8_t x)
    {
        uint8_t *p = (uint8_t *)vp;
        p[0] = x;        
    }

    void Write16(void *vp, uint16_t x);

    void Write24(void *vp, uint32_t x);

    void Write32(void *vp, uint32_t x);


    inline void Write8(void *vp, unsigned offset, uint8_t x)
    {
        Write8(offset + (uint8_t *)vp, x);
    }

    inline void Write16(void *vp, unsigned offset, uint16_t x)
    {
        Write16(offset + (uint8_t *)vp, x);
    }

    inline void Write24(void *vp, unsigned offset, uint32_t x)
    {
        Write24(offset + (uint8_t *)vp, x);
    }

    inline void Write32(void *vp, unsigned offset, uint32_t x)
    {
        Write32(offset + (uint8_t *)vp, x);
    }


}



#endif