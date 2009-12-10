#ifndef __DATEREC_H__
#define __DATEREC_H__

#include <ctime>

namespace Pascal {

class  DateRec {
public:

    DateRec();
    DateRec(unsigned yy, unsigned mm, unsigned dd);
    DateRec(unsigned);
    
    operator std::time_t() const;
    
    unsigned month() const { return _month; }
    unsigned day() const { return _day; }
    unsigned year() const { return _year; }
    
private:
    unsigned _year;
    unsigned _month;
    unsigned _day;
};


inline DateRec::DateRec()
{
    _year = 0;
    _month = 0;
    _day = 0;
}

inline DateRec::DateRec(unsigned yy, unsigned mm, unsigned dd)
{
    _year = yy;
    _month = mm;
    _day = dd;
}

}

#endif
