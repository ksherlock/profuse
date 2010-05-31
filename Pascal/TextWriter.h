
#ifndef __PASCAL_TEXTWRITER_H__
#define __PASCAL_TEXTWRITER_H__

#include <vector>
#include <stdint.h>

namespace Pascal {

    
    class TextWriter {
        
    public:
        
        TextWriter();
        ~TextWriter();
        
        unsigned blocks() const;
        
        void *data(unsigned block) const;
        
        void writeLine(const char *);
        void writeLine(const char *, unsigned length);
        
    private:
        
        std::vector<uint8_t *> _blocks;
        unsigned _offset;
        
        uint8_t *_current;
        
    };
    
}


#endif