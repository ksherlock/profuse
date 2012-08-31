#ifndef __NUFX_ENTRY_H__
#define __NUFX_ENTRY_H__


#include <Common/smart_pointers.h>

#include <string>
#include <stdint.h>


#include <NufxLib.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <dirent.h>

namespace NuFX {

  class DirectoryEntry;
  class Entry;
  class FileEntry;
  class VolumeEntry;

    typedef SHARED_PTR(DirectoryEntry) DirectoryEntryPointer;
    typedef SHARED_PTR(Entry) EntryPointer;
    typedef SHARED_PTR(FileEntry) FileEntryPointer;
    typedef SHARED_PTR(VolumeEntry) VolumeEntryPointer;

    typedef WEAK_PTR(Entry) EntryWeakPointer;
    typedef WEAK_PTR(VolumeEntry) VolumeEntryWeakPointer;

    class Entry : public ENABLE_SHARED_FROM_THIS(Entry) {

    public:

        virtual ~Entry();

        virtual unsigned inode() const;
        virtual const std::string& name() const;
        
        
        // operations...
        virtual int stat(VolumeEntryPointer, struct stat *) const;
        virtual ssize_t read(VolumeEntryPointer, size_t size, off_t offset) const;
        virtual ssize_t listxattr(VolumeEntryPointer, char *namebuf, size_t size, int options) const;
        virtual ssize_t getxattr(VolumeEntryPointer, const std::string &name, void *value, size_t size, u_int32_t position, int options) const;

        virtual int open(VolumeEntryPointer, int flags);
        virtual int close(VolumeEntryPointer);
        

                
    protected:
        Entry();
        Entry(const std::string& name);
        
        void setName(const std::string&);

    private:
        Entry(const Entry&);
        Entry& operator=(const Entry&);

        friend VolumeEntry;

        std::string _name;
        unsigned _inode;
    };
}

#endif