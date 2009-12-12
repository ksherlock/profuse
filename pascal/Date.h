#ifndef __PASCAL_DATE_H__
#define __PASCAL_DATE_H__

#include <ctime>

namespace Pascal {

class  Date {
public:

    static Date Today();

    Date();
    Date(unsigned yy, unsigned mm, unsigned dd);
    Date(unsigned);
    
    operator std::time_t() const;
    operator unsigned() const;
    
    unsigned month() const { return _month; }
    unsigned day() const { return _day; }
    unsigned year() const { return _year; }
    
private:
    unsigned _year;
    unsigned _month;
    unsigned _day;
};


inline Date::Date()
{
    _year = 0;
    _month = 0;
    _day = 0;
}

inline Date::Date(unsigned yy, unsigned mm, unsigned dd)
{
    _year = yy;
    _month = mm;
    _day = dd;
}

}

#endif
