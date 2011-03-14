//
//  Device.h
//  profuse
//
//  Created by Kelvin Sherlock on 2/19/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef __DEVICE_DEVICE_H__
#define __DEVICE_DEVICE_H__

#include <Common/smart_pointers.h>

namespace Device {
    
    class BlockDevice;
    class BlockCache;
    
    typedef SHARED_PTR(BlockDevice) BlockDevicePointer;
    typedef SHARED_PTR(BlockCache) BlockCachePointer;    
}


#endif
