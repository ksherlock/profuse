
#ifndef __DATAWRITER_H__
#define __DATAWRITER_H__

#include <stdint.h>

namespace ProFUSE {

class DataWriter {

public:

    DataWriter(unsigned size);
    DataWriter(unsigned size, void *data);
    virtual ~DataWriter();
    
    void write8(uint8_t);
    virtual void write16(uint16_t) = 0;
    virtual void write24(uint32_t) = 0;
    virtual void write32(uint32_t) = 0;

    void write(const void *data, unsigned size);
    
    
    void setOffset(unsigned o) { _offset = o; }
    unsigned offset() const { return _offset; }

    void forward(unsigned count) { _offset += count; }
    void rewind(unsigned count) { _offset -= count; }
    
    void *data() const { return _buffer; }
    unsigned size() const { return _size; }

protected:

    uint8_t *pointer() const { return _offset + _buffer; }

    bool _release;
    unsigned _size;

    unsigned _offset;
    uint8_t *_buffer;
    

};

class DataWriterLE : public DataWriter {
public:
    DataWriterLE(unsigned);
    DataWriterLE(unsigned, void *);
    
    virtual void write8(uint8_t);
    virtual void write16(uint16_t);
    virtual void write24(uint32_t);
    virtual void write32(uint32_t);
};


class DataWriterBE : public DataWriter {
public:
    DataWriterBE(unsigned);
    DataWriterBE(unsigned, void *);
    
    virtual void write8(uint8_t);
    virtual void write16(uint16_t);
    virtual void write24(uint32_t);
    virtual void write32(uint32_t);
};

}

#endif