#ifndef __DEVICE_ADAPTOR_H__
#define __DEVICE_ADAPTOR_H__

#include <utility>
#include <vector>

#include <stdint.h>

#include <Device/TrackSector.h>

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
    
 
    class NibbleAdaptor : public Adaptor
    {
    public:
        
        NibbleAdaptor(void *address, unsigned length);
        virtual ~NibbleAdaptor();
        
        virtual void readBlock(unsigned block, void *bp);
        virtual void writeBlock(unsigned block, const void *bp);
    
        
        virtual void readTrackSector(TrackSector ts, void *bp);
        virtual void writeTrackSector(TrackSector ts, const void *bp);
        
        static std::pair<uint8_t, uint8_t>encode44(uint8_t);
        static uint8_t decode44(uint8_t, uint8_t);
        
        static uint8_t encode62(uint8_t);
        static uint8_t decode62(uint8_t);
        
        
    private:
        uint8_t *_address;
        unsigned _length;
        
        std::vector<unsigned> _index;
    };
    
}


#endif
