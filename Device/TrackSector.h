#ifndef __TRACKSECTOR_H__
#define __TRACKSECTOR_H__

namespace Device {

    struct TrackSector {
        TrackSector(unsigned, unsigned);
        unsigned track;
        unsigned sector;
    };

    inline TrackSector::TrackSector(unsigned t, unsigned s)
    {
        track = t;
        sector = s;
    }
}

#endif