#include "DateRec.h"
#include <cstring>

using namespace Pascal;



DateRec::DateRec(unsigned val)
{
    // yyyy yyym mmmm dddd
    _day = val & 0xf;
    _month = (val >> 4) & 0x1f;
    _year = (val >> 9) & 0x7f;
}

DateRec::operator std::time_t() const {
    struct tm tm;
    
    if (_day == 0 || _month == 0) return (std::time_t)-1;
    
    std::memset(&tm, 0, sizeof(tm));
    tm.tm_hour = 12;
    tm.tm_mday = _day;
    tm.tm_mon = _month;
    tm.tm_year = _year; 
    tm.tm_isdst = -1;
    
    return std::mktime(&tm);
}