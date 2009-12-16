#ifndef __PRODOS_DATE_TIME__
#define __PRODOS_DATE_TIME__

#include <ctime>
#include <stdint.h>

namespace ProDOS {

class DateTime
{
public:
    DateTime(); 
    DateTime(std::time_t);
    DateTime(uint32_t);
    DateTime(unsigned, unsigned);

    DateTime(unsigned year, unsigned month, unsigned day, 
        unsigned hour, unsigned minute);

    std::time_t toUnix() const;
   

    operator std::time_t() const;
    operator uint32_t() const;

    unsigned date() const;
    unsigned time() const;

    unsigned minute() const;
    unsigned hour() const;
    unsigned day() const;
    unsigned month() const;
    unsigned year() const;

private:
    void init(std::time_t);
    void init(unsigned, unsigned, unsigned, unsigned, unsigned);

    unsigned _yymmdd;
    unsigned _hhmm;
};


inline DateTime::operator std::time_t() const
{
    return toUnix();
}

inline DateTime::operator uint32_t() const
{
    return (_yymmdd << 16) | _hhmm;
}

inline unsigned DateTime::date() const 
{
    return _yymmdd;
}

inline unsigned DateTime::time() const
{
    return _hhmm;
}

inline unsigned DateTime::minute() const 
{
    return _hhmm & 0x3f;
}

inline unsigned DateTime::hour() const
{
    return (_hhmm >> 8) & 0x1f;
}

inline unsigned DateTime::day() const
{
    return _yymmdd & 0x1f;
}

inline unsigned DateTime::month() const
{
    return (_yymmdd >> 5) & 0x0f;
}

/*
inline unsigned DateTime::year() const
{
    unsigned tmp = _yymmdd >> 9;
    if (tmp <= 39) tmp += 100;
    return tmp + 1900;
}
*/

} // namespace

#endif

