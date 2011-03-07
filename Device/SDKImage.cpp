//
//  SDKImage.cpp
//  profuse
//
//  Created by Kelvin Sherlock on 3/6/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "SDKImage.h"

#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cerrno>

#include <NufxLib.h>


#include <File/File.h>
#include <File/MappedFile.h>

#include <ProFUSE/Exception.h>

    
using ProFUSE::Exception;
using ProFUSE::POSIXException;


class NuFXException : public Exception
{
public:
    
    NuFXException(const char *cp, NuError error);
    NuFXException(const std::string& string, NuError error);
    
    virtual const char *errorString();

};


inline NuFXException::NuFXException(const char *cp, NuError error) :
Exception(cp, error)
{
}

inline NuFXException::NuFXException(const std::string& string, NuError error) :
Exception(string, error)
{
}

const char *NuFXException::errorString()
{
    return ::NuStrError((NuError)error());
}


using namespace Device;

struct record_thread
{
    NuRecordIdx record_index;
    NuThreadIdx thread_index;
};

/*
 * callback function to 
 *
 *
 */
static NuResult ContentFunction(NuArchive *archive, void *vp)
{
    const NuRecord *record = (const NuRecord *)vp;
    
    /*
     * The application must not attempt to retain a copy of "pRecord" 
     * after the callback returns, as the structure may be freed.  
     * Anything of interest should be copied out.
     */
    
    
    for (unsigned i = 0; i < NuRecordGetNumThreads(record); ++i)
    {
        const NuThread *thread = NuGetThread(record, i);
        
        
        printf("%ld, %ld\n", (long)record->recordIdx, (long)thread->threadIdx);
        
        if (NuGetThreadID(thread) == kNuThreadIDDiskImage)
        {
            record_thread *rt;
            
            NuGetExtraData(archive, (void **)&rt);
            if (rt)
            {
                rt->record_index = record->recordIdx;
                rt->thread_index = thread->threadIdx;
            }
            
            return kNuAbort;
        }
        
    }
    
    
    return kNuOK;
}

static NuThreadIdx FindDiskThread(const NuRecord *record)
{
#undef __METHOD__
#define __METHOD__ "SDKImage::FindDiskThread"
    
    for (unsigned i = 0; i < NuRecordGetNumThreads(record); ++i)
    {
        const NuThread *thread = NuGetThread(record, i);
        
        if (NuGetThreadID(thread) == kNuThreadIDDiskImage)
            return thread->threadIdx;
    }    

    throw Exception(__METHOD__ ": not a disk image");
}

/*
 * helper function to extract SDK image to /tmp and return a 
 * ProDOSDiskImage of the /tmp file.
 *
 */
BlockDevicePointer SDKImage::Open(const char *name)
{
#undef __METHOD__
#define __METHOD__ "SDKImage::Open"
    
    
    char tmp[] = "/tmp/pfuse.XXXXXXXX";
    
    int fd = -1;
    FILE *fp = NULL;
    NuArchive *archive = NULL;
    //const NuThread *thread = NULL;
    const NuRecord *record = NULL;
    NuDataSink *sink = NULL;
    NuRecordIdx rIndex;
    NuThreadIdx tIndex;
    
    NuError e;


    record_thread rt = {0, 0};
    
    try {

    
        e = NuOpenRO(name, &archive);
        if (e)
        {
            throw NuFXException(__METHOD__ ": NuOpenRO", e);
        }
        NuSetExtraData(archive, (void *)&rt);
        
        // NuContents screws everything up???
        /*
        e = NuContents(archive, ContentFunction);
        if (e == kNuErrAborted)
        {
            // ok!
        }
        else if (e == kNuErrNone)
        {
            throw Exception(__METHOD__ ": NuContents: not a disk image");
        }
        else
        {
            throw NuFXException(__METHOD__ ": NuContents", e);
        }

        
        printf("%ld, %ld\n", (long)rt.record_index, (long)rt.thread_index);
        */
        
        e = NuGetRecordIdxByPosition(archive, 0, &rIndex);
        if (e)
        {
            throw NuFXException(__METHOD__ ": NuGetRecordIdxByPosition", e);
        }
        printf("%ld\n", rIndex);
        
        /* load up the record.
         * I assume this is necessary since thread ids are only unique
         * within a record, not across the entire archive
         */
        
        e = NuGetRecord(archive, rIndex, &record);
        if (e)
        {
            
            throw NuFXException(__METHOD__ ": NuGetRecord", e);
        }
        
        tIndex = FindDiskThread(record); // throws on error.
        printf("%ld\n", tIndex);

        
        fd = mkstemp(tmp);
        if (fd < 0)
        {
            throw POSIXException(__METHOD__ ": mkstemp", errno);
        }
        
        fp = fdopen(fd, "w");
        if (!fp)
        {
            ::close(fd);
            throw POSIXException(__METHOD__ ": fdopen", errno);            
        }
        
        e = NuCreateDataSinkForFP(true, kNuConvertOff, fp, &sink);
        if (e)
        {
            throw NuFXException(__METHOD__ ": NuCreateDataSinkForFP", e);
        }
        

        e = NuExtractThread(archive, tIndex, sink);
        if (e)
        {
            throw NuFXException(__METHOD__ ": NuExtractThread", e);
        }
        

        fclose(fp);
        NuClose(archive);
        NuFreeDataSink(sink);
        fp = NULL;
        archive = NULL;
        sink = NULL;
    }
    catch(...)
    {
        if (fp) fclose(fp);
        if (archive) NuClose(archive);
        if (sink) NuFreeDataSink(sink);
        
        throw;
    }

    // todo -- maybe SDKImage should extend ProDOSOrderDiskImage, have destructor
    // that unklinks the temp file.
    
    MappedFile file(tmp, File::ReadOnly);

    return ProDOSOrderDiskImage::Open(&file);

}
