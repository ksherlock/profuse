#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <vector>

namespace ProFUSE {
class Buffer {

public:
    Buffer() {}
    Buffer(unsigned size)  { _buffer.reserve(size); }

    void *buffer() const { return (void *)&_buffer[0]; }
    unsigned size() const { return _buffer.size(); }
    
    void resize(unsigned size) { _buffer.resize(size); }
    void clear() { _buffer.clear(); }
    
    void push8(uint8_t x) { _buffer.push_back(x); }
    
    void push16be(uint16_t);
    void push16le(uint16_t);
    
    void push32be(uint32_t);
    void push32le(uint32_t);
    
    void pushBytes(const void *data, unsigned size);


private:
    std::vector<uint8_t> _buffer;
};

}
#endif
