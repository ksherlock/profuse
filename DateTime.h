#ifndef __PRODOS_DATE_TIME__
#define __PRODOS_DATE_TIME__

#include <ctime>
#include <stdint.h>

namespace ProDOS {

class DateTime
{
public:
    DateTime(); 
    DateTime(time_t);
    DateTime(uint32_t);
    DateTime(unsigned, unsigned);

    DateTime(unsigned year, unsigned month, unsigned day, 
        unsigned hour, unsigned minute);

    time_t toUnix() const;
   

    operator time_t() const;
    unsigned minute() const;
    unsigned hour() const;
    unsigned day() const;
    unsigned month() const;
    unsigned year() const;

private:
    unsigned _hhmm;
    unsigned _yymmdd;
};


DateTime::operator time_t() const
{
    return toUnix();
}


unsigned DateTime::minute() const 
{
    return _hhmm & 0x3f;
}

unsigned DateTime::hour() const
{
    return (_hhmm >> 8) & 0x1f;
}


unsigned DateTime::day() const
{
    return _yymmdd & 0x1f;
}

unsigned DateTime::month() const
{
    return (_yymmdd >> 5) & 0x1f;
}

/*
unsigned DateTime::year() const
{
    unsigned tmp = _yymmdd >> 9;
    if (tmp <= 39) tmp += 100;
    return tmp + 1900;
}
*/

} // namespace

#endif

