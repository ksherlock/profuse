
#ifndef __MACROMAN_H__
#define __MACROMAN_H__

#include <string>

class MacRoman {
public:
    MacRoman(const std::string& string);

    bool isASCII() const;
    std::string toUTF8() const;
    std::wstring toWString() const;


private:
    std::string _string;
};

inline MacRoman::MacRoman(const std::string& string) :
    _string(string)
{
}

#endif

