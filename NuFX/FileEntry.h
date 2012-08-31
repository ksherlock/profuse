#ifndef __NUFX_FILEENTRY_H__
#define __NUFX_FILEENTRY_H__

#include "Entry.h"
#include <NufxLib.h>

namespace NuFX {


    class FileEntry : public Entry
    {
    public:
    
    private:
    
        NuRecordIdx _recordID;
        unsigned _flags; // threads
        size_t _size; // data size
    };
}


#endif