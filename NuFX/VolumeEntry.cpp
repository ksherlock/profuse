#include "VolumeEntry.h"

using namespace NuFX;

void VolumeEntry::addEntry(EntryPointer e)
{
    if (!e) return;
    
    e->setVolume(pointer());
    
    _inodeIndex->push_back(e);

    e->_inode = _inodeIndex->length() + 100 - 1;
}