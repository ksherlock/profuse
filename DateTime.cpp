#include "DateTime.h"

#include <strings.h>

namespace ProDOS {

/*
 *  date is a 16 bit value:
 *
 *  7 6 5 4 3 2 1 0   7 6 5 4 3 2 1 0 
 * +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ 
 * |    Year     |  Month  |   Day   |  
 * +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ 
 *
 * time is a 16 bit value:
 *
 *  7 6 5 4 3 2 1 0   7 6 5 4 3 2 1 0 
 * +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ 
 * |0 0 0|  Hour   | |0 0|  Minute   | 
 * +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ 
 *
 */
DateTime::DateTime() 
{
    DateTime(time(NULL));
}

DateTime::DateTime(uint32_t dtm) :
    _yymmdd((dtm >> 16) & 0xff), _hhmm(dtm & 0xff)
{
}

DateTime::DateTime(unsigned yymmdd, unsigned hhmm) :
    _yymmdd(yymmdd), _hhmm(hhmm)
{
}

DateTime::DateTime(time_t time)
{
    tm t;

    localtime_r(&time, &t);

    DateTime(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min);

}


DateTime::DateTime(unsigned year, unsigned month, unsigned day,
        unsigned hour, unsigned minute)
{
  
    // 1940 is the earliest year, so clamp to 1940-01-01 00:00
    if (year < 1940)
    {
        _yymmdd = (40 << 9) | (1 << 5) | 1;
        _hhmm = 0;
        return;
    }
    // 2039 is the latest year, so clamp to 2039-12-31 23:59
    if (year > 2039)
    {
        _yymmdd = (39 << 9) | (12 << 5) | 31;
        _hhmm = (23 << 8) | 59;
        return;
    }

    if (year >= 2000) year -= 2000;
    else year -= 1900;


    _hhmm = minute | (hour << 8);

    _yymmdd = day | (month << 5) | (year << 9);
}

/*
 * ProDOS technote 28
 *
 * The following definition allows the same range of years that the Apple IIgs
 * Control Panel CDA currently does:
 *
 * o  A seven-bit ProDOS year value is in the range 0 to 99
 * (100 through 127 are invalid)
 * o  Year values from 40 to 99 represent 1940 through 1999
 * o  Year values from 0 to 39 represent 2000 through 2039
 */


unsigned DateTime::year() const
{
    unsigned tmp = _yymmdd >> 9;
    if (tmp == 0 || tmp > 100) return 0;
  
    if (tmp <= 39) tmp += 2000;
    else tmp += 1900;

    return tmp;
}

/*
 * A positive or 0 value for tm_isdst causes mktime() to presume initially
 * that Daylight Savings Time, respectively, is or is not in effect for
 * the specified time. A negative value for tm_isdst causes mktime() to
 * attempt to determine whether Daylight Saving Time is in effect for the
 * specified time.
 */

time_t DateTime::toUnix() const
{
    tm t;

    if (_yymmdd == 0) return 0;

    bzero(&t, sizeof(tm));

    t.tm_min = minute();
    // tm_hour is 0-23, but is_dst -1 compensates
    t.tm_hour = hour() - 1;
    t.tm_isdst = -1;

    t.tm_mday = day();
    t.tm_mon = month() - 1;
    t.tm_year = year() - 1900;

    return mktime(&t);
    // convert back via locatime & fudge for dst?
}

} // namespace
