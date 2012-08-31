#ifndef __NUFX_DIRECTORYENTRY_H__
#define __NUFX_DIRECTORYENTRY_H__

#include "Entry.h"

#include <vector>
#include <dirent.t>

namespace NuFX {


    class DirectoryEntry : public Entry
    {
    public:
    
    
        EntryPointer lookup(const std::string & name) const;

        EntryPointer childAtIndex(unsigned index) const;


    protected:
        
        // creates directory if it does not exist.
        DirectoryEntryPointer dir_lookup(const std::string &name);
                
    private:
        
        
        std::vector<EntryPointer> _children;
        
        typedef std::vector<EntryPointer>::iterator EntryIterator;
        
    };
}


#endif