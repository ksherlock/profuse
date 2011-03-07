//
//  SDKImage.h
//  profuse
//
//  Created by Kelvin Sherlock on 3/6/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <Device/BlockDevice.h>
#include <Device/DiskImage.h>

namespace Device {
    
    class SDKImage : public DiskImage
    {
    public:
        
        static BlockDevicePointer Open(const char *name);
        
        
        static bool Validate(MappedFile *, const std::nothrow_t &);
        static bool Validate(MappedFile *);        
        
    private:
        SDKImage();
        SDKImage(const SDKImage &);
        ~SDKImage();
        SDKImage & operator=(const SDKImage &);
    };
}