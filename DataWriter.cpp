#include "DataWriter.h"
#include "Endian.h"

#include <cstring>

using namespace ProFUSE;

DataWriter::DataWriter(unsigned size)
{
    _size = size;
    _release = true;
    _offset = 0;
    _buffer = new uint8_t[size];
}

DataWriter::DataWriter(unsigned size, void *buffer)
{
    _size = size;
    _buffer = (uint8_t *)buffer;
    _release = false;
    _offset = 0;
}

DataWriter::~DataWriter()
{
    if (_release && _buffer) delete[] _buffer;
}

void DataWriter::write8(uint8_t data)
{
    _buffer[_offset] = data;
    _offset += 1;
}

void DataWriter::write(const void *data, unsigned size)
{
    std::memcpy(pointer(), data, size);
    _offset += size;
}



DataWriterLE::DataWriterLE(unsigned size) : 
    DataWriter(size)
{}

DataWriterLE::DataWriterLE(unsigned size, void *buffer) : 
    DataWriter(size, buffer)
{}


void DataWriterLE::write16(uint16_t data)
{
    LittleEndian::Write16(pointer(), data);
    _offset += 2;
}

void DataWriterLE::write24(uint32_t data)
{
    LittleEndian::Write24(pointer(), data);
    _offset += 3;
}

void DataWriterLE::write32(uint32_t data)
{
    LittleEndian::Write32(pointer(), data);
    _offset += 4;
}



DataWriterBE::DataWriterBE(unsigned size) : 
    DataWriter(size)
{}

DataWriterBE::DataWriterBE(unsigned size, void *buffer) : 
    DataWriter(size, buffer)
{}


void DataWriterBE::write16(uint16_t data)
{
    BigEndian::Write16(pointer(), data);
    _offset += 2;
}

void DataWriterBE::write24(uint32_t data)
{
    BigEndian::Write24(pointer(), data);
    _offset += 3;
}

void DataWriterBE::write32(uint32_t data)
{
    BigEndian::Write32(pointer(), data);
    _offset += 4;
}

