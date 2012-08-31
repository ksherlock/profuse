#ifndef __NUFX_VOLUMEENTRY_H__
#define __NUFX_VOLUMEENTRY_H__

#include "DirectoryEntry.h"

#include <Common/unordered_map.h>

namespace NuFX {


    class VolumeEntry : public DirectoryEntry
    {
    public:
    
        void addEntry(EntryPointer);
        
    private:
    
        VolumeEntryPointer pointer() const
        {
            return STATIC_POINTER_CAST(VolumeEntryPointer, shared_from_this());
        }
    
        void parse();
    
        NuArchive *_archive;
        
        //unsigned _inodeGenerator;
        
        std::vector<WeakPointer> _inodeIndex;
    };
}


#endif