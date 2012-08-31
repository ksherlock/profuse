


class TextFile {

public:


    unsigned size() const;
    
    unsigned read(void *buffer, unsigned size, unsigned offset);


private:

    static unsigned decodeBlock(uint8_t *in, unsigned inSize, uint8_t *out);

    unsigned _size;
    std::vector<unsigned> _pageSize
};


unsigned decodeBlock(uint8_t *in, unsigned inSize, uint8_t *out)
{
    const unsigned DLE = 16;
    
    unsigned size = 0;
    
    for (unsigned i = 0; i < inSize; ++i)
    {
        uint8_t c = in[i];
        if (!c) break;

        if ((c == DLE) && (i + 1 < inSize))
        {
            unsigned x = in[++i] - 32;
            
            if (out)
            {
                for (unsigned i = 0; i < x; ++i)
                    *out++ = ' ';
            }
            size += x;    
        }
        else
        {
            if (out) *out++ = c;
            ++size;
        }


    }
    
    return size;
}


{

    // first 2 blocks are header information
    
    _fileSize = 0;
    
    unsigned pages = (_endBlock - _startBlock - 2) >> 1
    uint8_t buffer[1024];
    unsigned offset = 0;
    
    for (unsigned i _startBlock + 2; i <= _endBlock; i += 2)
    {
        uint8_t buffer[1024];
        unsigned dataSize = 0;
        unsigned offset = 0;
        unsigned size;
        
        // load a 2-block page.
        for (j = i; j <= _endBlock; ++j)
        {
            _device->readBlock(j, buffer + offset);
            dataSize += j == _endBlock ? _lastByte : 512;
        
        }

        size = decodeBlock(buffer, dataSize, NULL);
        
        _pageSize.push_back(size);
        _fileSize += size;
    }


}


