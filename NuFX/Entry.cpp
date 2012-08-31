#include "Entry.h"
#include "Exception.h"

using namespace NuFX;


Entry::Entry() :
    _inode(0)
{
}

Entry::Entry(const std::string& name) :
    _name(name), _inode(0)
{
}


Entry::~Entry()
{
}


unsigned Entry::inode() const
{
    return _inode;
}

const std::string& Entry::name() const
{
    return _name;
}

void Entry::setName(const std::string& name)
{
    _name = name;
}

VolumeEntryWeakPointer Entry::volume() const
{
    return _volume;
}

void Entry::setVolume(VolumeEntryWeakPointer volume)
{
    _volume = volume;
}

int Entry::stat(struct stat *st) const
{
    return -1;
}

ssize_t Entry::read(size_t size, off_t offset) const
{
    return -1;
}

ssize_t Entry::listxattr(char *namebuf, size_t size, int options) const
{
    return -1;
}

ssize_t Entry::getxattr(const std::string &name, void *value, size_t size, u_int32_t position, int options) const
{
    return -1;
}
