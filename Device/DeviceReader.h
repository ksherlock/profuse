#ifndef __DEVICEREADER_H__
#define __DEVICEREADER_H__

#include <Device/TrackSector.h>

namespace Device {


class DeviceReader {
    public:
    
    virtual ~DeviceReader();
    
    virtual void read(unsigned block, void *bp) = 0;
    virtual void read(TrackSector ts, void *bp) = 0;
    
    virtual void write(unsigned block, const void *bp) = 0;
    virtual void write(TrackSector ts, const void *bp) = 0;
    
    // direct access -- not always available.
    
    virtual void *read(unsigned block);
    virtual void *read(TrackSector ts);
    
    virtual bool readOnly() = 0;
    virtual bool mapped();

};

class FileDeviceReader : public DeviceReader {

    // does not assume ownership of fd.
    FileDeviceReader(int fd, unsigned blocks, bool readOnly);
    //virtual ~FileDeviceReader();
    
public:

    virtual bool readOnly();


    virtual void read(unsigned block, void *bp);
    virtual void read(TrackSector ts, void *bp);
    
    virtual void write(unsigned block, const void *bp);
    virtual void write(TrackSector ts, const void *bp);  
    
    private:
    
    int _fd;
    unsigned _blocks;
    bool _readOnly;
}

class MappedFileDeviceReader : public DeviceReader {

    protected:
    
    MappedFileDeviceReader(MappedFile *f, unsigned offset);
    
    void *_data;
    
    private:
    MappedFile *_file
};

class ProDOSOrderDeviceReader : public MappedFileDeviceReader {
    public:

    virtual void read(unsigned block, void *bp);
    virtual void read(TrackSector ts, void *bp);
    
    virtual void write(unsigned block, const void *bp);
    virtual void write(TrackSector ts, const void *bp);  

    virtual void *read(unsigned block);
    virtual void *read(TrackSector ts);

    private:
    unsigned blocks;
};

// 16 sectors only.
class DOSOrderDeviceReader : public MappedFileDeviceReader {
    public:

    virtual void read(unsigned block, void *bp);
    virtual void read(TrackSector ts, void *bp);
    
    virtual void write(unsigned block, const void *bp);
    virtual void write(TrackSector ts, const void *bp);  

    virtual void *read(TrackSector ts);

    private:
    unsigned _tracks;
};



class NibbleDeviceReader : public MappedFileDeviceReader {

    private:
    
    std::vector<unsigned> _map;
};

}

#endif