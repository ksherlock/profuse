#ifndef __IOBUFFER_H__
#define __IOBUFFER_H__

#include "../Endian.h"

#include <cstring>
namespace LittleEndian {
    
    class IOBuffer {
        public:
        
        IOBuffer(void *vp, unsigned size)
        {
            _buffer = vp;
            _size = size;
            _offset = 0;
        }
        
        void write8(uint8_t value)
        {
            Write8(_buffer, _offset, value);
            _offset += 1;
        }
        void write16(uint16_t value)
        {
            Write16(_buffer, _offset, value);
            _offset += 2;
        }
                
        void write24(uint32_t value)
        {
            Write24(_buffer, _offset, value);
            _offset += 3;
        }
        void write32(uint32_t value)
        {
            Write32(_buffer, _offset, value);
            _offset += 4;
        }
        
        void writeBytes(const void *value, unsigned count)
        {
            std::memcpy(_offset + (uint8_t *)_buffer, value, count);
            _offset += count; 
        }
        
        void writeZero(unsigned count)
        {
            uint8_t *cp = _offset + (uint8_t *)_buffer;
            for (unsigned i = 0; i < count; ++i)
            {
                cp[i] = 0;
            }
            _offset += count;
        }
        
        unsigned offset() const { return _offset; }
        void setOffset(unsigned offset) { _offset = offset; }
        
        unsigned size() const { return _size; }
        
        private:
        void *_buffer;
        unsigned _size;
        unsigned _offset;
    
    };

}

#endif