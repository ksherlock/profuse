#include "Date.h"
#include <cstring>
#include <stdint.h>

using namespace Pascal;



Date::Date(unsigned val)
{
    // yyyy yyym mmmm dddd
    _month = val & 0xf;
    _day = (val >> 4) & 0x1f;
    _year = (val >> 9) & 0x7f;
}

Date::operator std::time_t() const {
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

Date::operator uint16_t() const {
    // year must be 0 .. 127
    return (_year << 9) | (_day << 4) | _month;
}

Date Date::Today()
{
    struct tm tm;
    std::time_t t = std::time(NULL); 

    ::localtime_r(&t, &tm);

    return Date(tm.tm_year, tm.tm_mon, tm.tm_mday);
}