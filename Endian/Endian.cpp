#include "Endian.h"

using namespace LittleEndian {


    uint16_t Read16(const void *vp)
    {
        const uint8_t *p = (const uint8_t *)vp;
        return p[0] | (p[1] << 8);        
    }

    uint32_t Read24(const void *vp)
    {
        const uint8_t *p = (const uint8_t *)vp;
        return (p[0]) | (p[1] << 8) | (p[2] << 16);        
    }


    uint32_t Read32(const void *vp)
    {
        const uint8_t *p = (const uint8_t *)vp;
        return (p[0]) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);  
    }
   


    void Write16(void *vp, uint16_t x)
    {
        uint8_t *p = (uint8_t *)vp;
        p[0] = (x) & 0xff;
        p[1] = (x >> 8) & 0xff;   
    }

    void Write24(void *vp, uint32_t x)
    {
        uint8_t *p = (uint8_t *)vp;
        p[0] = (x) & 0xff;
        p[1] = (x >> 8) & 0xff;     
        p[2] = (x >> 16) & 0xff;     
    }
    
    void Write32(void *vp, uint32_t x)
    {
        uint8_t *p = (uint8_t *)vp;
        p[0] = (x) & 0xff;
        p[1] = (x >> 8) & 0xff;     
        p[2] = (x >> 16) & 0xff;   
        p[3] = (x >> 24) & 0xff;        
    }





}

using namespace BigEndian {


    inline uint16_t Read16(const void *vp)
    {
        const uint8_t *p = (const uint8_t *)vp;
        return (p[0] << 8) | (p[1]);        
    }

    inline uint32_t Read24(const void *vp)
    {
        const uint8_t *p = (const uint8_t *)vp;
        return (p[0] << 16) | (p[1] << 8) | (p[2]);        
    }


    inline uint32_t Read32(const void *vp)
    {
        const uint8_t *p = (const uint8_t *)vp;
        return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);  
    }


    inline void Write16(void *vp, uint16_t x)
    {
        uint8_t *p = (uint8_t *)vp;
        p[0] = (x >> 8) & 0xff;
        p[1] = (x) & 0xff;   
    }

    inline void Write24(void *vp, uint32_t x)
    {
        uint8_t *p = (uint8_t *)vp;
        p[0] = (x >> 16) & 0xff;
        p[1] = (x >> 8) & 0xff;     
        p[2] = (x) & 0xff;     
    }
    
    inline void Write32(void *vp, uint32_t x)
    {
        uint8_t *p = (uint8_t *)vp;
        p[0] = (x >> 24) & 0xff;
        p[1] = (x >> 16) & 0xff;     
        p[2] = (x >> 8) & 0xff;   
        p[3] = (x) & 0xff;        
    }


}