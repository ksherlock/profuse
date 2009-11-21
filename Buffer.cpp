
#include "Buffer.h"

using namespace ProFUSE;

void Buffer::push16be(uint16_t x)
{
    _buffer.push_back((x >> 8) & 0xff);
    _buffer.push_back(x & 0xff);
}

void Buffer::push16le(uint16_t x)
{
    _buffer.push_back(x & 0xff);
    _buffer.push_back((x >> 8) & 0xff);
}

void Buffer::push32be(uint32_t x)
{
    _buffer.push_back((x >> 24) & 0xff);
    _buffer.push_back((x >> 16) & 0xff);
    _buffer.push_back((x >> 8) & 0xff);
    _buffer.push_back(x & 0xff);
}

void Buffer::push32le(uint32_t x)
{
    _buffer.push_back(x & 0xff);
    _buffer.push_back((x >> 8) & 0xff);
    _buffer.push_back((x >> 16) & 0xff);
    _buffer.push_back((x >> 24) & 0xff);
}

void Buffer::pushBytes(const void *data, unsigned size)
{
    for (unsigned i = 0; i < size; ++i)
    {
        _buffer.push_back( ((uint8_t *)data)[i] );
    }
}