


#include <cstring>
#include <Device/Adaptor.h>

using namespace Device;


Adaptor::~Adaptor()
{
}



POAdaptor::POAdaptor(void *address)
{
    _address = (uint8_t *)address;
}

void POAdaptor::readBlock(unsigned block, void *bp)
{
    std::memcpy(bp, _address + block * 512, 512);
}

void POAdaptor::writeBlock(unsigned block, const void *bp)
{
    std::memcpy(_address + block * 512, bp, 512);
}


unsigned DOAdaptor::Map[] = {
    0x00, 0x0e, 0x0d, 0x0c, 
    0x0b, 0x0a, 0x09, 0x08, 
    0x07, 0x06, 0x05, 0x04, 
    0x03, 0x02, 0x01, 0x0f
};

DOAdaptor::DOAdaptor(void *address)
{
    _address = (uint8_t *)address;
}

void DOAdaptor::readBlock(unsigned block, void *bp)
{
    
    unsigned track = (block & ~0x07) << 9;
    unsigned sector = (block & 0x07) << 1;
    
    for (unsigned i = 0; i < 2; ++i)
    {
        size_t offset =  track | (Map[sector+i] << 8); 
        
        std::memcpy(bp, _address + offset,  256);
        
        bp = (uint8_t *)bp + 256;
    }
}

void DOAdaptor::writeBlock(unsigned block, const void *bp)
{
    unsigned track = (block & ~0x07) << 9;
    unsigned sector = (block & 0x07) << 1;
    
    for (unsigned i = 0; i < 2; ++i)
    {
        size_t offset =  track | (Map[sector+i] << 8); 
        
        std::memcpy(_address + offset,  bp, 256);
        bp = (uint8_t *)bp + 256;
    }
}





