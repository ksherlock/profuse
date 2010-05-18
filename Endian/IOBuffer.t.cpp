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
        
        void writeBytes(const void *src, unsigned count)
        {
            std::memcpy(_offset + (uint8_t *)_buffer, src, count);
            _offset += count; 
        }
        
        void writeZero(unsigned count)
        {
            std::memset(_offset + (uint8_t *)_buffer, 0, count);

            _offset += count;
        }
        
       
       uint8_t read8()
       {
           uint8_t x = Read8(_buffer, _offset);
           _offset += 1;
           return x;
       }

       uint16_t read16()
       {
           uint16_t x = Read16(_buffer, _offset);
           _offset += 2;
           return x;
       }
       
       uint32_t read24()
       {
           uint32_t x = Read24(_buffer, _offset);
           _offset += 3;
           return x;
       }
       
       uint32_t read32()
       {
           uint32_t x = Read32(_buffer, _offset);
           _offset += 4;
           return x;
       }
       
       void readBytes(void *dest, unsigned count)
       {
           std::memcpy(dest, _offset + (uint8_t *)_buffer, count);
           _offset += count;    
       }
       
       
        unsigned offset() const { return _offset; }
        void setOffset(unsigned offset) { _offset = offset; }
        
        void setOffset(unsigned offset, bool zero)
        {
            if (zero && offset > _offset)
            {
                writeZero(offset - _offset);
            }
            else setOffset(offset);
        }
        
        unsigned size() const { return _size; }
        
        void *buffer() const { return _buffer; }
        
        private:
        void *_buffer;
        unsigned _size;
        unsigned _offset;
    
    };