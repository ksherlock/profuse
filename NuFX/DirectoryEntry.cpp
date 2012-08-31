#include "DirectoryEntry.h"

#include <cstring>

using namespace NuFX;

EntryPointer DirectoryEntry::lookup(const std::string& name) const
{
    EntryIterator iter;
    
    for (iter = _children.begin(); iter != _children.end(); ++iter)
    {
    
        EntryPointer e = *iter;
        
        if (e->name() == name) return e;
    }

    return EntryPointer(); // empty.
}

DirectoryEntryPointer DirectoryEntry::dir_lookup(const std::string &name)
{
    EntryIterator iter;

    for (iter = _children.begin(); iter != _children.end(); ++iter)
    {
    
        EntryPointer e = *iter;
        
        if (e->name() == name)
        {
            // dynamic cast, will return as empty pointer if
            // not a directory.
            
            return DYNAMIC_POINTER_CAST(DirectoryEntryPointer, e);
        }    
    }
    
    // not found, insert it.. 

    DirectoryEntryPointer e(new DirectoryEntryPointer(name));
    VolumeEntryPointer v = volume().lock();
    
    _children.add(e);
    
    if (v)
    {
        v->addEntry(e);
    }
    
    return e;
}

#pragma mark -
#pragma mark fuse-support

EntryPointer DirectoryEntry::childAtIndex(unsigned index) const
{
    if (index >= _children.size()) return EntryPointer();
    
    return _children[index];
}
