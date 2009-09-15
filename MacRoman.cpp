
#include "MacRoman.h"

/*
 * mapping of MacRoman characters (0x80-0xff) to unicode.
 */
static unsigned m2u[] = {
    0x00C4,
    0x00C5,
    0x00C7,
    0x00C9,
    0x00D1,
    0x00D6,
    0x00DC,
    0x00E1,
    0x00E0,
    0x00E2,
    0x00E4,
    0x00E3,
    0x00E5,
    0x00E7,
    0x00E9,
    0x00E8,
    0x00EA,
    0x00EB,
    0x00ED,
    0x00EC,
    0x00EE,
    0x00EF,
    0x00F1,
    0x00F3,
    0x00F2,
    0x00F4,
    0x00F6,
    0x00F5,
    0x00FA,
    0x00F9,
    0x00FB,
    0x00FC,
    0x2020,
    0x00B0,
    0x00A2,
    0x00A3,
    0x00A7,
    0x2022,
    0x00B6,
    0x00DF,
    0x00AE,
    0x00A9,
    0x2122,
    0x00B4,
    0x00A8,
    0x2260,
    0x00C6,
    0x00D8,
    0x221E,
    0x00B1,
    0x2264,
    0x2265,
    0x00A5,
    0x00B5,
    0x2202,
    0x2211,
    0x220F,
    0x03C0,
    0x222B,
    0x00AA,
    0x00BA,
    0x03A9,
    0x00E6,
    0x00F8,
    0x00BF,
    0x00A1,
    0x00AC,
    0x221A,
    0x0192,
    0x2248,
    0x2206,
    0x00AB,
    0x00BB,
    0x2026,
    0x00A0,
    0x00C0,
    0x00C3,
    0x00D5,
    0x0152,
    0x0153,
    0x2013,
    0x2014,
    0x201C,
    0x201D,
    0x2018,
    0x2019,
    0x00F7,
    0x25CA,
    0x00FF,
    0x0178,
    0x2044,
    0x20AC,
    0x2039,
    0x203A,
    0xFB01,
    0xFB02,
    0x2021,
    0x00B7,
    0x201A,
    0x201E,
    0x2030,
    0x00C2,
    0x00CA,
    0x00C1,
    0x00CB,
    0x00C8,
    0x00CD,
    0x00CE,
    0x00CF,
    0x00CC,
    0x00D3,
    0x00D4,
    0xF8FF,
    0x00D2,
    0x00DA,
    0x00DB,
    0x00D9,
    0x0131,
    0x02C6,
    0x02DC,
    0x00AF,
    0x02D8,
    0x02D9,
    0x02DA,
    0x00B8,
    0x02DD,
    0x02DB,
    0x02C7
};



bool MacRoman::isASCII() const
{
    for (std::string::const_iterator iter = _string.begin(); iter != _string.end(); ++iter)
    {
        if (*iter & 0x80) return false;
    }
    return true;
}

std::string MacRoman::toUTF8() const
{
    std::string out;
    out.reserve(_string.length()); 

    for (std::string::const_iterator iter = _string.begin(); iter != _string.end();  ++iter)
    {
        char c = *iter;
        if (c & 0x80 == 0) out.push_back(c);
        else
        {
            unsigned uc = m2u[c & 0x7f];
            if (uc <= 0x7ff)
            {
                out.push_back(0xC0 | (uc >> 6));
                out.push_back(0x80 | (uc & 0x3f));
            }
            else // nothing larger than 0xffff
            {
                out.push_back(0xe0 | (uc >> 12));
                out.push_back(0x80 | ((uc >> 6) & 0x3f));
                out.push_back(0x80 | (uc & 0x3f));
            }
        }
    }

    return out;
}

std::wstring MacRoman::toWString() const
{
    std::wstring out;
    out.reserve(_string.length());

    for (std::string::const_iterator iter = _string.begin(); iter != _string.end(); ++iter)
    {
        char c = *iter;
        if (c & 0x80 == 0) out.push_back(c);
        else out.push_back(m2u[c & 0x7f]);
    }
  
    return out;
}

