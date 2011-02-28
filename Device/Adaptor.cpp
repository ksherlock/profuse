


#include <cstring>
#include <cstdio>
#include <Device/Adaptor.h>

#include <ProFUSE/Exception.h>

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



#pragma mark -
#pragma mark NibbleAdaptor

class CircleBuffer {
    
public:
    
    CircleBuffer(void *address, unsigned length)
    {
        _address = (uint8_t *)address;
        _length = length;
    }
    
    uint8_t operator[](unsigned i) const
    {
        if (i >= _length) i %= _length;
        return _address[i];
    }
    
    uint8_t& operator[](unsigned i)
    {
        if (i >= _length) i %= _length;
        return _address[i];
    }
    
private:
    uint8_t *_address;
    unsigned _length;
    
};



uint8_t NibbleAdaptor::decode44(uint8_t x, uint8_t y)
{
    return ((x << 1) | 0x01) & y;
}

std::pair<uint8_t, uint8_t> NibbleAdaptor::encode44(uint8_t val)
{
    uint8_t x = (val >> 1) | 0xaa;
    uint8_t y = val | 0xaa;
    
    return std::make_pair(x,y);
}

uint8_t NibbleAdaptor::encode62(uint8_t val)
{
#undef __METHOD__
#define __METHOD__ "NibbleAdaptor::encode62"
    
    static uint8_t table[64] = {
        0x96, 0x97, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa6, 
        0xa7, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb2, 0xb3,
        
        0xb4, 0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbc,
        0xbd, 0xbe, 0xbf, 0xcb, 0xcd, 0xce, 0xcf, 0xd3,
        
        0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 
        0xdf, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec,
        
        0xed, 0xee, 0xef, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
        0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };
    
    if (val > 0x3f)
        throw ProFUSE::Exception(__METHOD__ ": Invalid 6-2 value.");
        
    return table[val];
}


uint8_t NibbleAdaptor::decode62(uint8_t val)
{
#undef __METHOD__
#define __METHOD__ "decode62"
    
    // auto-generated via perl.
    static uint8_t table[] = {
        -1, -1, -1, -1, -1, -1, 0, 1, -1, -1, 2, 3, -1, 4, 5, 6, 
        -1, -1, -1, -1, -1, -1, 7, 8, -1, -1, -1, 9, 10, 11, 12, 13, 
        -1, -1, 14, 15, 16, 17, 18, 19, -1, 20, 21, 22, 23, 24, 25, 26, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 27, -1, 28, 29, 30, 
        -1, -1, -1, 31, -1, -1, 32, 33, -1, 34, 35, 36, 37, 38, 39, 40, 
        -1, -1, -1, -1, -1, 41, 42, 43, -1, 44, 45, 46, 47, 48, 49, 50, 
        -1, -1, 51, 52, 53, 54, 55, 56, -1, 57, 58, 59, 60, 61, 62, 63    
    };
    
    if ((val < 0x90) || (table[val - 0x90] == 0xff))
        throw ProFUSE::Exception(__METHOD__ ": Invalid 6-2 encoding.");
        
    return table[val - 0x90];
}  



static int FindByte(void *address, uint8_t c, unsigned length, unsigned offset = 0)
{

    for (unsigned i = offset; i < length; ++i)
    {
        if ( ((uint8_t *)address)[i] == c) return i;
    }
    
    return -1;
}



/*
 *  Address Field:
 *  prologue   volume  track  sector  checksum  epilogue
 *  D5 AA 96   XX YY   XX YY  XX YY   XX YY     DE AA EB
 */

/*
 * Data Field:
 * prologue    user data      checksum  epilogue
 * D5 AA AD    [6+2 encoded]  XX        DE AA EB
 */



NibbleAdaptor::NibbleAdaptor(void *address, unsigned length)
{
#undef __METHOD__
#define __METHOD__ "NibbleAdaptor::NibbleAdaptor"
    
    _address = (uint8_t *)address;
    _length = length;
    
    
    // build a map of track/sectors.
    
    unsigned state = 0;
    

    _index.resize(35 * 16, -1);
    
    int offset = 0;
    
    unsigned track = 0;
    unsigned sector = 0;
    unsigned volume = 0;
    unsigned checksum = 0;
    
    
    CircleBuffer buffer(_address, _length);
    for (;;)
    {
        
        offset = FindByte(address, 0xd5, length, offset);
        if (offset < 0) break;
                
        if (buffer[offset + 1] == 0xaa && buffer[offset + 2] == 0x96 && buffer[offset + 11] == 0xde && buffer[offset + 12] == 0xaa)
        {            
            volume = decode44(buffer[offset + 3], buffer[offset + 4]);
            track = decode44(buffer[offset + 5], buffer[offset + 6]);
            sector = decode44(buffer[offset + 7], buffer[offset + 8]);
            checksum = decode44(buffer[offset + 9], buffer[offset + 10]);
            
            if (volume ^ track ^ sector ^ checksum)
                throw ProFUSE::Exception(__METHOD__ ": Invalid address checksum.");
            
            if (track > 35 || sector > 16)
                throw ProFUSE::Exception(__METHOD__ ": Invalid track/sector.");
            
            offset += 3 + 8 + 3;
            
            state = 1;            
            continue;
            
        }
        
        if (buffer[offset + 1] == 0xaa && buffer[offset + 2] == 0xad && state == 1)
        {
            if (_index[track * 16 + sector] != -1)
            {
                std::fprintf(stderr, "track %u sector %u duplicated.\n", track, sector);
            }
            _index[track * 16 + sector] = (offset + 3) % _length;
            
            //offset += 3 + 342 + 1 + 3;
            offset++;
            
            state = 0;
            continue;
        }
        
        offset++; //???
        // ????
        
    }
    
    // possible wraparound.
    if (state == 1)
    {        
        offset = FindByte(address, 0xd5, length, 0);
        
        if (offset >= 0)
        {
            
            if (buffer[offset + 1] == 0xaa && buffer[offset + 2] == 0xad)
            {
                _index[track * 16 + sector] = (offset + 3) % _length;
            }            
        }
    }
    
    
    // now check _index for offset = -1, which means the sector/track wasn't found.
    
    for (std::vector<unsigned>::iterator iter = _index.begin(); iter != _index.end(); ++iter)
    {
        if (*iter == -1)
        {
            int offset = distance(_index.begin(), iter);
            std::fprintf(stderr, "Error: track %u sector %u missing.\n", offset / 16, offset % 16);
            //throw ProFUSE::Exception(__METHOD__ ": Sector missing.");
        }
    }
    
}

NibbleAdaptor::~NibbleAdaptor()
{
}

void NibbleAdaptor::readBlock(unsigned block, void *bp)
{
    
    unsigned track = (block & ~0x07) << 9;
    unsigned b = (block & 0x07) << 1;
    
    /*
     * block   sectors
     * 0       0, 2
     * 1       4, 6
     * 2       8, 10
     * 3       12,14
     * 4       1, 3
     * 5       5, 7
     * 6       9, 11
     * 7       13, 15
     */
    
    unsigned sector = b >> 2;
    if (sector >= 16) sector -= 15;
    

    readTrackSector(TrackSector(track, sector), bp);
    readTrackSector(TrackSector(track, sector + 1), (uint8_t *)bp + 256);
}

void NibbleAdaptor::writeBlock(unsigned block, const void *bp)
{
    unsigned track = (block & ~0x07) << 9;
    unsigned b = (block & 0x07) << 1;
    
    /*
     * block   sectors
     * 0       0, 2
     * 1       4, 6
     * 2       8, 10
     * 3       12,14
     * 4       1, 3
     * 5       5, 7
     * 6       9, 11
     * 7       13, 15
     */
    
    unsigned sector = b >> 2;
    if (sector >= 16) sector -= 15;
    
    
    writeTrackSector(TrackSector(track, sector), bp);
    writeTrackSector(TrackSector(track, sector + 1), (const uint8_t *)bp + 256);
}

void NibbleAdaptor::readTrackSector(TrackSector ts, void *bp)
{
#undef __METHOD__
#define __METHOD__ "NibbleAdaptor::readTrackSector"
    
    if (ts.track > 35 || ts.sector > 16)
        throw ProFUSE::Exception(__METHOD__ ": Invalid track/sector.");

    
    CircleBuffer buffer(_address, _length);
    uint8_t bits[86 * 3];

    uint8_t checksum = 0;

    
    unsigned offset = _index[ts.track * 16 + ts.sector];
    
    if (offset == -1)
    {
        throw ProFUSE::Exception(__METHOD__ ": Missing track/sector.");
    }
    
    // first 86 bytes are in the auxbuffer, backwards.
    unsigned index = offset;
    for (unsigned i = 0; i < 86; ++i)
    {
        uint8_t x = buffer[index++];
        x = decode62(x);
        
        checksum ^= x;
        
        uint8_t y = checksum;
        
        /*
        for (unsigned j = 0; j < 3; ++j)
        {
            //bits[i + j * 86] = ((y & 0x01) << 1) | ((y & 0x02) >> 1);
            bits[i + j * 86] = "\x00\x01\x02\x03"[y & 0x03];
            y >>= 2;
        }
        */
        bits[i + 86 * 0] = "\x00\x02\x01\x03"[y & 0x03];
        bits[i + 86 * 1] = "\x00\x02\x01\x03"[(y >> 2) & 0x03];
        bits[i + 86 * 2] = "\x00\x02\x01\x03"[(y >> 4) & 0x03];
        
    }
    
    for (unsigned i = 0; i < 256; ++i)
    {
        uint8_t x = buffer[index++];
        x = decode62(x);
        
        checksum ^= x;
        
        uint8_t y = (checksum << 2) | bits[i];
   
        
        
        ((uint8_t *)bp)[i] = y;
    }
    
    if (checksum != decode62(buffer[index++]))
        std::fprintf(stderr, "Invalid checksum on track %u, sector %u\n", ts.track, ts.sector);
        //throw ProFUSE::Exception(__METHOD__ ": Invalid field checksum.");
   
}

void NibbleAdaptor::writeTrackSector(TrackSector ts, const void *bp)
{
#undef __METHOD__
#define __METHOD__ "NibbleAdaptor::writeTrackSector"
    
    if (ts.track > 35 || ts.sector > 16)
        throw ProFUSE::Exception(__METHOD__ ": Invalid track/sector.");
    
    uint8_t auxBuffer[86];
    uint8_t checksum = 0;
    
    // create the aux buffer. 
    
    std::memset(auxBuffer, 0, sizeof(auxBuffer));
                
    for (unsigned i = 0, j = 0, shift = 0; i < 256; ++i)
    {
        uint8_t x = ((const uint8_t *)bp)[i];
        
        // grab the bottom 2 bytes and reverse them.
        //uint8_t y = ((x & 0x01) << 1) | ((x & 0x02) >> 1);
        uint8_t y = "\x00\x02\x01\x03"[x & 0x03];
        
        auxBuffer[j++] |= (y << shift);
        
        if (j == 86)
        {
            j = 0;
            shift += 2;
        }
    }
    
    unsigned offset = _index[ts.track * 16 + ts.sector];
    
    CircleBuffer buffer(_address, _length);
    // create the checksum while writing to disk..

    // aux buffer
    for (unsigned i = 0; i < 86; ++i)
    {
        uint8_t x = auxBuffer[i];
        
        buffer[offset + i] = encode62(x ^ checksum);
        
        checksum = x;
    }    
    
    for (unsigned i = 0; i < 256; ++i)
    {
        uint8_t x = ((const uint8_t *)bp)[i];
        x >>= 2;
        
        buffer[offset + 86 + i] = encode62(x ^ checksum);
        
        checksum = x;
    }
    

    
    buffer[offset + 342] = encode62(checksum);
}

