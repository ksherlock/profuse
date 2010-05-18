#ifndef __DEVICE_ADAPTOR_H__
#define __DEVICE_ADAPTOR_H__

#include <stdint.h>

namespace Device {

    class Adaptor
    {
    public:
        virtual ~Adaptor();
        virtual void readBlock(unsigned block, void *bp) = 0;
        virtual void writeBlock(unsigned block, const void *bp) = 0;        
    };
    

    class POAdaptor : public Adaptor
    {
    public:
        POAdaptor(void *address);
        virtual void readBlock(unsigned block, void *bp);
        virtual void writeBlock(unsigned block, const void *bp);
    private:
        uint8_t *_address;
    };
    
    class DOAdaptor : public Adaptor
    {
    public:
        DOAdaptor(void *address);
        virtual void readBlock(unsigned block, void *bp);
        virtual void writeBlock(unsigned block, const void *bp);
    private:
        uint8_t *_address;
        
        static unsigned Map[];
    };
    
    // TODO -- nibble adaptor.
    
    
}


#endif
