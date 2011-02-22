//
//  Device.h
//  profuse
//
//  Created by Kelvin Sherlock on 2/19/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef __DEVICE_DEVICE_H__
#define __DEVICE_DEVICE_H__

#include <tr1/memory>

namespace Device {
    
    class BlockDevice;
    class BlockCache;
    
    
    typedef std::tr1::shared_ptr<BlockDevice> BlockDevicePointer;
    typedef std::tr1::shared_ptr<BlockCache> BlockCachePointer;

    
}


#endif
