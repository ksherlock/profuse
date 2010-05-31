
#include <algorithm>
#include <cstring>
#include <cctype>
#include <memory>
#include <cerrno>

#include <Pascal/Pascal.h>
#include <Pascal/TextWriter.h>

#include <ProFUSE/auto.h>
#include <ProFUSE/Exception.h>

#include <Endian/Endian.h>
#include <Endian/IOBuffer.h>

#include <Device/BlockDevice.h>



using namespace LittleEndian;
using namespace Pascal;


enum {
    kDLE = 16
};


/*
 * _lastByte is in the range 1..512 and indicates how many bytes in the last
 * block are in use.
 * _lastBlock is the block *after* the actual last block.
 * _maxFileSize is the maximum file size the file can grow to.
 *
 */



unsigned FileEntry::ValidName(const char *cp)
{
    return Entry::ValidName(cp, 15);
}

FileEntry::FileEntry(void *vp) :
    Entry(vp)
{
    _status = Read8(vp, 0x05) & 0x01;
    _fileNameLength = Read8(vp, 0x06);
    std::memset(_fileName, 0, 16);
    std::memcpy(_fileName, 0x07 + (uint8_t *)vp, _fileNameLength);
    _lastByte = Read16(vp, 0x16);
    _modification = Date(Read16(vp, 0x18));
    
    _fileSize = 0;
    _pageSize = NULL;
    _maxFileSize = 0;
}

FileEntry::FileEntry(const char *name, unsigned fileKind)
{
#undef __METHOD__
#define __METHOD__ "FileEntry::FileEntry"

    unsigned length = ValidName(name);
    
    if (!length)
        throw ProFUSE::Exception(__METHOD__ ": Invalid file name.");
        
    _fileKind = fileKind;
    _status = 0;
    
    _fileNameLength = length;
    std::memset(_fileName, 0, sizeof(_fileName));
    for (unsigned i = 0; i < length; ++i)
        _fileName[i] = std::toupper(name[i]);
    
    _modification = Date::Today();
    _lastByte = 0;
    
    _fileSize = 0;
    _pageSize = NULL;
    _maxFileSize = 0;
}

FileEntry::~FileEntry()
{
    delete _pageSize;
}


void FileEntry::setFileKind(unsigned kind)
{
    _fileKind = kind;
    if (_pageSize)
    {
        delete _pageSize;
        _fileSize = 0;
        _pageSize = NULL;
    }
    
    parent()->writeEntry(this);
}


void FileEntry::setName(const char *name)
{
#undef __METHOD__
#define __METHOD__ "FileEntry::setName"
    
    unsigned length = ValidName(name);
    
    if (!length)
        throw ProFUSE::ProDOSException(__METHOD__ ": Invalid file name.", ProFUSE::badPathSyntax);
    
    _fileNameLength = length;
    for (unsigned i = 0; i < length; ++i)
        _fileName[i] = std::toupper(name[i]);

    
    // parent's responsibility.
    //parent()->writeEntry(this);
}

unsigned FileEntry::fileSize()
{
    switch(fileKind())
    {
    case kTextFile:
        return textFileSize();
        break;
    default:
        return dataFileSize();
        break;
    }
}

void FileEntry::writeDirectoryEntry(IOBuffer *b)
{
    Entry::writeDirectoryEntry(b);
    
    b->write8(_status ? 0x01 : 0x00);
    b->write8(_fileNameLength);
    b->writeBytes(_fileName, 15);
    b->write16(_lastByte);
    b->write16(_modification);
}

int FileEntry::read(uint8_t *buffer, unsigned size, unsigned offset)
{
    unsigned fsize = fileSize();
    
    if (offset + size > fsize) size = fsize - offset;
    if (offset >= fsize) return 0;
    
    switch(fileKind())
    {
    case kTextFile:
        return textRead(buffer, size, offset);
        break;
    default:
        return dataRead(buffer, size, offset);
        break;
    }
}


int FileEntry::truncate(unsigned newSize)
{
#undef __METHOD__
#define __METHOD__  "FileEntry::truncate"
    
    unsigned currentSize = fileSize();
    
    if (currentSize == newSize) return 0;
    
    if (fileKind() == kTextFile)
    {
        if (newSize)
        {
            errno = EINVAL;
            return -1;
        }
        newSize = 2; // text files have a 2-page scratch buffer for the editor.
        
        if (_pageSize)
        {
            _pageSize->clear();
            _fileSize = 0;
        }
    }
    
    if (truncateCommon(newSize) != 0)
        return -1;
    
    _modification = Date::Today();
    parent()->writeEntry(this);
    
    return 0;
}

/*
 * truncateCommon -- common truncation code.
 * updates _lastByte and _lastBlock but does
 * not update _modification or commit to disk.
 */
int FileEntry::truncateCommon(unsigned newSize)
{
#undef __METHOD__
#define __METHOD__  "FileEntry::truncateCommon"
    
    unsigned currentSize = fileSize();
    
    if (newSize == currentSize) return 0;
    if (newSize > currentSize)
    {
        
        if (newSize > _maxFileSize)
        {
            errno = ENOSPC;
            return -1;
        }
        
        unsigned remainder = newSize - currentSize;
        unsigned block = _lastBlock - 1;
        
        if (_lastByte != 512)
        {
            // last page not full
            unsigned count = std::min(512 - _lastByte, remainder);
            uint8_t *address = (uint8_t *)parent()->loadBlock(block);
            
            std::memset(address + _lastByte, 0, count);

            parent()->unloadBlock(block, true);
            
            remainder -= count;
        }
        block++;
        
        while (remainder)
        {
            unsigned count = std::min(512u, remainder);
            uint8_t *address = (uint8_t *)parent()->loadBlock(block);
            
            std::memset(address, 0, count);
            
            parent()->unloadBlock(block, true);
            
            remainder -= count;
            block++;            
        }
        
        
    }
    
    setFileSize(newSize);

    return 0;
}


int FileEntry::write(TextWriter &text)
{
    unsigned blocks = text.blocks();
    
    if (parent()->readOnly())
    {
        errno = EROFS;
        return -1;
    }
    
    if (fileKind() != kTextFile)
    {
        errno = EINVAL;
        return -1;
    }    
    
    
    if (blocks * 512 > _maxFileSize)
    {
        errno = ENOSPC;
        return -1;
    }
    
    for (unsigned i = 0; i < blocks; ++i)
    {
        void *buffer = text.data(i);
        parent()->writeBlock(_firstBlock + i, buffer);
        
    }
    
    _modification = Date::Today();
    
    setFileSize(blocks * 512);
    
    parent()->writeEntry(this);
    parent()->sync();
    
    return blocks * 512;
}

int FileEntry::write(const uint8_t *buffer, unsigned size, unsigned offset)
{
#undef __METHOD__
#define __METHOD__ "FileEntry::write"
    
    if (parent()->readOnly())
    {
        errno = EROFS;
        return -1;
    }
    
    if (fileKind() == kTextFile)
    {
        errno = EINVAL;
        return -1;
    }

    unsigned currentSize = fileSize();
    
    unsigned newSize = std::max(offset + size, currentSize);
    

    if (newSize > _maxFileSize)
    {
        errno = ENOSPC;
        return -1;
    }
    
    if (offset > currentSize)
    {
        if (truncateCommon(offset) != 0) return -1;
    }
    
    // now write the data...
    
    unsigned block = firstBlock() +  offset / 512;
    unsigned start = offset % 512;
    unsigned remainder = size;
    
    if (start)
    {
        unsigned count = std::min(512 - start, remainder);
        uint8_t *address = (uint8_t *)parent()->loadBlock(block);
                
        std::memcpy(address + start, buffer, count);
        parent()->unloadBlock(block, true);
        
        remainder -= count;
        buffer += count;
        block++;
    }
    
    while (remainder)
    {
        uint8_t *address = (uint8_t *)parent()->loadBlock(block);

        unsigned count = std::min(512u, size);
        
        std::memcpy(address, buffer, count);
        parent()->unloadBlock(block, true);
        
        remainder -= count;
        buffer += count;
        block++;
    }
    
    
    if (newSize > currentSize) setFileSize(newSize);

    
    _modification = Date::Today();
    parent()->writeEntry(this);
    
    return size;
}


/* 
 * private
 * set the file size.  Does not check if > _maxFileSize.
 * 
 */
void FileEntry::setFileSize(unsigned size)
{
    if (size == 0)
    {
        // TODO -- verify how 0 byte files are handled.
        _lastBlock = _firstBlock + 1;
        _lastByte = 0;
        return;
    }

    _lastBlock = 1 + _firstBlock + size / 512;
    _lastByte = size % 512;
    if (_lastByte == 0) _lastByte = 512;
}

unsigned FileEntry::dataFileSize()
{
    return blocks() * 512 - 512 + _lastByte;
}

unsigned FileEntry::textFileSize()
{
    if (!_pageSize) textInit();
    return _fileSize;  
}






int FileEntry::dataRead(uint8_t *buffer, unsigned size, unsigned offset)
{
    uint8_t tmp[512];
    
    unsigned count = 0;
    unsigned block = 0;
        
    block = _firstBlock + (offset / 512);
    
    // returned value (count) is equal to size at this point.
    // (no partial reads).
    
    /*
     * 1.  Block align everything
     */
    
    if (offset % 512)
    {
        unsigned bytes = std::min(offset % 512, size);
        
        parent()->readBlock(block++, tmp);
        
        std::memcpy(buffer, tmp + 512 - bytes, bytes);
        
        buffer += bytes;
        count += bytes;
        size -= bytes;
    }
    
    /*
     * 2. read full blocks into the buffer.
     */
     
     while (size >= 512)
     {
        parent()->readBlock(block++, buffer);
        
        buffer += 512;
        count += 512;
        size -= 512;
     }
     
    /*
     * 3. Read any trailing blocks.
     */
    if (size)
    {
        parent()->readBlock(block, tmp);
        std::memcpy(buffer, tmp, size);
        
        count += size; 
    
    }

    
    return count;
}



int FileEntry::textRead(uint8_t *buffer, unsigned size, unsigned offset)
{
    unsigned page = 0;
    unsigned to = 0;
    unsigned block;
    unsigned l;
    unsigned count = 0;
    
    ProFUSE::auto_array<uint8_t> tmp;
    unsigned tmpSize = 0;
    
    if (!_pageSize) textInit();
    
    l = _pageSize->size();


    // find the first page.    
    for (page = 0; page < l; ++page)
    {
        unsigned pageSize = (*_pageSize)[page];
        if (to + pageSize > offset)
        {
            break;
        }
        
        to += pageSize;
    }
    
    // first 2 pages are spare, for editor use, not actually text.
    block = _firstBlock + 2 + (page * 2);
    
    
    // offset not needed anymore,
    // convert to offset from *this* page.
    offset -= to;
    
    while (size)
    {
        unsigned pageSize = (*_pageSize)[page];
        unsigned bytes = std::min(size, pageSize - offset);
        
        if (pageSize > tmpSize)
        {
            tmp.reset(new uint8_t[pageSize]);
            tmpSize = pageSize;            
        }
        
        
        // can decode straight to buffer if size >= bytes && offset = 0.
        textDecodePage(block, tmp.get());
        
        
        std::memcpy(buffer, tmp.get() + offset, bytes);
        
        
        block += 2;
        page += 1;
        
        size -= bytes;
        buffer += bytes;
        count += bytes;
        
        offset = 0;
    }

    return count;
}



unsigned FileEntry::textDecodePage(unsigned block, uint8_t *out)
{
    uint8_t buffer[1024];
    unsigned size = 0;
    bool dle = false;
    
    unsigned bytes = textReadPage(block, buffer);
    
    for (unsigned i = 0; i < bytes; ++i)
    {
        uint8_t c = buffer[i];
        
        if (!c) continue;
        
        
        if (dle)
        {
            if (c > 32)
            {
                unsigned x = c - 32;
                size += x;
                if (out) for (unsigned j = 0; j < x; ++j)
                    *out++ = ' ';
            }
            dle = false;
            continue;
        }
        if (c == kDLE) { dle = true; continue; }
        
        //if (c & 0x80) continue; // ascii only.
        

        if (c == 0x0d) c = 0x0a; // convert to unix format.
        if (out) *out++ = c;
        size += 1;

    }
    
    return size;
}

unsigned FileEntry::textReadPage(unsigned block, uint8_t *in)
{
    // reads up to 2 blocks.
    // assumes block within _startBlock ... _lastBlock - 1

    parent()->readBlock(block, in); 
    if (block + 1 == _lastBlock)
    {
        return _lastByte;
    }
    
    parent()->readBlock(block + 1, in + 512);
    if (block +2 == _lastBlock)
    {
        return 512 + _lastByte;
    }
    
    return 1024;
} 



void FileEntry::textInit()
{
    // calculate the file size and page offsets.
    _pageSize = new std::vector<unsigned>;
    _pageSize->reserve((_lastBlock - _firstBlock + 1 - 2) / 2);

    _fileSize = 0;
    for (unsigned block = _firstBlock + 2; block < _lastBlock; block += 2)
    {
        unsigned size = textDecodePage(block, NULL);
        //printf("%u: %u\n", block, size);
        _fileSize += size;
        _pageSize->push_back(size);
    }
}


/*
 * compress white space into a dle.
 * returns true if altered.
 * nb -- only leading white space is compressed.
 */
bool FileEntry::Compress(std::string& text)
{
    std::string out;
    
    size_t pos;
    size_t count;
    
    
    if (text.length() < 3) return false;
    if (text[0] != ' ') return false;
    
    pos = text.find_first_not_of(' ');
    
    if (pos == std::string::npos)
        count = text.length();
    else count = pos;
    
    if (count < 3) return false;
    
    count = std::min((int)count, 255 - 32);
    
    out.push_back(kDLE);
    out.push_back(32 + count);
    out.append(text.begin() + count, text.end());
    
    text.swap(out);
    
    return true;
}

/*
 * dle will only occur at start.
 *
 */
bool FileEntry::Uncompress(std::string& text)
{
    std::string out;

    unsigned c;
    
    if (text.length() < 2) return false;
    
    if (text[0] != kDLE) return false;
    
    c = text[1];
    if (c < 32) c = 32;
    
    out.append(c - 32, ' ');
    out.append(text.begin() + 2, text.end());
    
    text.swap(out);
    return true;
}

