
#include <Pascal/TextWriter.h>
#include <Pascal/FileEntry.h>

#include <ProFUSE/Exception.h>

#include <string>
#include <cstring>

using namespace Pascal;


TextWriter::TextWriter()
{   
    _offset = 0;
    _current = new uint8_t[1024];
    _blocks.push_back(new uint8_t[1024]); // 1024k for editor scratch data.
    
    std::memset(_blocks.back(), 0, 1024);
    std::memset(_current, 0, 1024);
}

TextWriter::~TextWriter()
{
    std::vector<uint8_t *>::iterator iter;
    
    if (_current) delete[] _current;
    
    for (iter = _blocks.begin(); iter != _blocks.end(); ++iter)
    {
        delete[] *iter;
    }
}

unsigned TextWriter::blocks() const
{
    if (_offset == 0) return _blocks.size() * 2;
    if (_offset <= 512) return _blocks.size() * 2 + 1;
    return _blocks.size() * 2 + 2;
}

void *TextWriter::data(unsigned block) const
{  
    unsigned offset = (block & 0x01) * 512;
    unsigned halfBlock = block >> 1;
    
    if (halfBlock < _blocks.size())
    {
        return _blocks[halfBlock] + offset;
    }
    if (halfBlock == _blocks.size())
    {
        if (offset > _offset) return NULL;
        return _current + offset;
    }
    return NULL;
}

void TextWriter::writeLine(const char *line)
{
    writeLine(line, std::strlen(line));
}

void TextWriter::writeLine(const char *line, unsigned length)
{
#undef __METHOD__
#define __METHOD__ "TextWriter::writeLine"
    
    
    if (line == NULL) line = "";
    
    std::string text(line, length);
    
    
    if (length)
    {
        char c = text[length - 1];
        if (c == 0x0a) text[length - 1] = 0x0d;
        else if (c != 0x0d) text.push_back(0x0d);
        
        FileEntry::Compress(text);
    }
    else
    {
        text.push_back(0x0d);
    }
    
    length = text.length();
    
    if (length > 1024)
    {
        throw ProFUSE::Exception(__METHOD__ ": String is too long.");
    }
    if (_offset + length > 1024)
    {
        _blocks.push_back(_current);
        _offset = 0;
        _current = new uint8_t[1024];
        std::memset(_current, 0, 1024);
    }
    
    std::memcpy(_current + _offset, text.data(), length);
    _offset += length;
}